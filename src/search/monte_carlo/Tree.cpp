/*
 * Tree.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
	using namespace ag;

	void print_node(const Node *node, const int max_depth, bool sort, int top_n, int prefix);
	void print_edge(const Edge &edge, const int max_depth, bool sort, int top_n, bool is_last, int prefix)
	{
		for (int i = 0; i < prefix; i++)
			std::cout << "|     ";
		if (is_last)
			std::cout << "└──" << edge.toString() << '\n';
		else
			std::cout << "├──" << edge.toString() << '\n';

		if (edge.getNode() != nullptr)
		{
			for (int i = 0; i < prefix; i++)
				std::cout << "|     ";
			std::cout << "|  └──";
			print_node(edge.getNode(), max_depth, sort, top_n, prefix + 1);
		}
	}
	void print_node(const Node *node, const int max_depth, bool sort, int top_n, int prefix)
	{
		if (node == nullptr)
			return;
		if (max_depth != -1 and node->getDepth() <= max_depth)
		{
			if (sort)
				node->sortEdges();

			std::cout << node->toString() << '\n';
			if (top_n == -1 || 2 * top_n >= node->numberOfEdges())
			{
				for (int i = 0; i < node->numberOfEdges(); i++)
					print_edge(node->getEdge(i), max_depth, sort, top_n, i == (node->numberOfEdges() - 1), prefix);
			}
			else
			{
				for (int i = 0; i < top_n; i++)
					print_edge(node->getEdge(i), max_depth, sort, top_n, false, prefix);
				for (int i = 0; i < prefix; i++)
					std::cout << "|     ";
				std::cout << "├──... (" << (node->numberOfEdges() - 2 * top_n) << " edges skipped)\n";
				for (int i = node->numberOfEdges() - top_n; i < node->numberOfEdges(); i++)
					print_edge(node->getEdge(i), max_depth, sort, top_n, i == (node->numberOfEdges() - 1), prefix);
			}
		}
		else
		{
			std::cout << "... (subtree skipped)\n";
			for (int i = 0; i < prefix; i++)
				std::cout << "|     ";
			std::cout << '\n';
			return;
		}
	}

	bool has_information_leak(const Edge *edge) noexcept
	{
		constexpr float leak_threshold = 0.01f;
		assert(edge != nullptr);
		if (edge->isLeaf())
			return false;
		Score tmp = edge->getNode()->getScore();
		tmp.increaseDistanceToWinOrLoss();
		if (edge->getScore() != -tmp)
			return true;
		if ((edge->getValue() - edge->getNode()->getValue().getInverted()).abs() >= leak_threshold)
			return true;
		return false;
	}
	void update_score(Edge *edge) noexcept
	{
		assert(edge != nullptr);
		assert(edge->isLeaf() == false);
		Score tmp = edge->getNode()->getScore();
		tmp.increaseDistanceToWinOrLoss();
		edge->setScore(-tmp);
	}
	void update_score(Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		Score result = Score::min_value();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			result = std::max(result, edge->getScore());
		node->setScore(result);
	}
}

namespace ag
{
	Tree::Tree(TreeConfig treeOptions) :
			tree_config(treeOptions)
	{
	}
	int64_t Tree::getMemory() const noexcept
	{
		int64_t result = (shared_hash_table == nullptr) ? 0 : shared_hash_table->getMemory();
		return result + node_cache.getMemory();
	}
	void Tree::clear()
	{
		node_cache.clear();
		if (shared_hash_table != nullptr)
			shared_hash_table->clear();
	}
	void Tree::setBoard(const matrix<Sign> &newBoard, Sign signToMove, bool forceRemoveRootNode)
	{
		if (not equalSize(base_board, newBoard))
		{
			const GameConfig game_config(GameRules::FREESTYLE, newBoard.rows(), newBoard.cols()); // rules specified here are irrelevant
			node_cache = NodeCache(game_config, tree_config);
			shared_hash_table = std::make_shared<SharedHashTable<4>>(newBoard.rows(), newBoard.cols(), tree_config.solver_hash_table_size);
			edge_selector = nullptr; // must clear selector in case it uses information about board size
			edge_generator = nullptr; // must clear generator in case it uses information about board size
		}
		else
		{
			shared_hash_table->increaseGeneration();
			node_cache.cleanup(newBoard, signToMove);
		}

		base_board = newBoard;
		base_depth = Board::numberOfMoves(newBoard);
		sign_to_move = signToMove;

		if (forceRemoveRootNode and node_cache.seek(newBoard, signToMove) != nullptr)
			node_cache.remove(newBoard, signToMove);
		root_node = node_cache.seek(newBoard, signToMove);
		if (root_node != nullptr)
			root_node->markAsRoot();
		max_depth = 0;
	}
	void Tree::setEdgeSelector(const EdgeSelector &selector)
	{
		edge_selector = selector.clone();
	}
	void Tree::setEdgeGenerator(const EdgeGenerator &generator)
	{
		edge_generator = generator.clone();
	}

	int Tree::getNodeCount() const noexcept
	{
		return node_cache.storedNodes();
	}
	int Tree::getSimulationCount() const noexcept
	{
		if (root_node == nullptr)
			return 0;
		else
			return root_node->getVisits();
	}
	int Tree::getMaximumDepth() const noexcept
	{
		return max_depth;
	}
	bool Tree::isProven() const noexcept
	{
		if (root_node == nullptr)
			return false;
		else
			return root_node->isProven();
	}
	bool Tree::hasAllMovesProven() const noexcept
	{
		if (root_node == nullptr)
			return false;
		else
			return std::all_of(root_node->begin(), root_node->end(), [](const Edge &edge)
			{	return edge.isProven();});
	}
	bool Tree::hasSingleMove() const noexcept
	{
		if (root_node == nullptr)
			return false;
		else
			return root_node->numberOfEdges() == 1;
	}
	bool Tree::hasSingleNonLosingMove() const noexcept
	{
		if (root_node == nullptr)
			return false;
		else
		{
			const int non_losing_edges_count = std::count_if(root_node->begin(), root_node->end(), [](const Edge &edge)
			{	return not edge.getScore().isLoss();});
			return non_losing_edges_count == 1;
		}
	}

	SelectOutcome Tree::select(SearchTask &task)
	{
		assert(edge_selector != nullptr);
		task.set(base_board, sign_to_move);
		Node *node = root_node;
		while (node != nullptr)
		{
			Edge *edge = edge_selector->select(node);
			task.append(node, edge);
			edge->increaseVirtualLoss();

			node = edge->getNode();
			if (node == nullptr)
			{ // edge appears to be a leaf
				node = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find board state in cache
				if (node != nullptr)
					edge->setNode(node); // if found in the cache, link that edge to the found node
				// if not found in the cache it means that the edge is really a leaf
			}

			if (has_information_leak(edge))
				return SelectOutcome::INFORMATION_LEAK;
		}
		max_depth = std::max(max_depth, task.visitedPathLength());
		return SelectOutcome::REACHED_LEAF;
	}
	void Tree::generateEdges(SearchTask &task) const
	{
		assert(edge_generator != nullptr);
		edge_generator->generate(task);
	}
	ExpandOutcome Tree::expand(SearchTask &task)
	{
		assert(task.getEdges().size() > 0);

		Node *node_to_add = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find board state in the cache
		if (node_to_add == nullptr)
		{ // not found in the cache
			const int number_of_edges = task.getEdges().size();
			assert(number_of_edges > 0);

			node_to_add = node_cache.insert(task.getBoard(), task.getSignToMove(), number_of_edges);
			for (int i = 0; i < number_of_edges; i++)
				node_to_add->getEdge(i) = task.getEdges()[i];
			node_to_add->updateValue(task.getValue()); // 'updateValue' is called because it increases visit count from 0 to 1 ('setValue' doesn't do that)
			update_score(node_to_add);

			if (task.visitedPathLength() > 0)
				task.getLastEdge()->setNode(node_to_add); // make last visited edge point to the newly added node
			else
			{
				root_node = node_to_add; // if no pairs were visited, it means that the tree is empty and we have to assign the root node
				root_node->markAsRoot();
			}

			return ExpandOutcome::SUCCESS;
		}
		else
		{ // this can happen if the same state was encountered from different paths
			if (task.visitedPathLength() > 0) // in a rare case it could be that the root node has already been expanded by some other thread
			{ // but if not
				task.getLastEdge()->setNode(node_to_add); // make last visited edge point to the newly added node
				if (has_information_leak(task.getLastEdge()))
					correctInformationLeak(task);
			}
			return ExpandOutcome::ALREADY_EXPANDED;
		}
	}
	void Tree::backup(const SearchTask &task)
	{
		assert(task.wasProcessedByNetwork() or task.wasProcessedBySolver());
		const Value value = task.getValue();
		for (int i = 0; i < task.visitedPathLength(); i++)
		{
			NodeEdgePair pair = task.getPair(i);
			if (pair.edge->getMove().sign == task.getSignToMove())
			{
				pair.node->updateValue(value);
				pair.edge->updateValue(value);
			}
			else
			{
				pair.node->updateValue(value.getInverted());
				pair.edge->updateValue(value.getInverted());
			}
			pair.edge->decreaseVirtualLoss();
		}
		for (int i = task.visitedPathLength() - 1; i >= 0; i--)
		{
			NodeEdgePair pair = task.getPair(i);
			update_score(pair.edge);
			update_score(pair.node);
			if (pair.node->isProven() == false)
				break; // if the node is not proven it will not change the tree above
		}
	}
	void Tree::correctInformationLeak(const SearchTask &task)
	{
		for (int i = task.visitedPathLength() - 1; i >= 0; i--)
		{
			NodeEdgePair pair = task.getPair(i);
			if (has_information_leak(pair.edge))
			{
				const Value current_edge_value = pair.edge->getValue();
				const Value target_edge_value = pair.edge->getNode()->getValue().getInverted(); // edge Q should be equal to (1 - node Q)

				const Value correction = (target_edge_value - current_edge_value) * pair.edge->getVisits() + target_edge_value;

				pair.edge->updateValue(correction);
				pair.node->updateValue(correction);
				update_score(pair.edge);
				update_score(pair.node);
			}
			else
				break;
		}
	}
	void Tree::cancelVirtualLoss(const SearchTask &task) noexcept
	{
		for (int i = 0; i < task.visitedPathLength(); i++)
			task.getPair(i).edge->decreaseVirtualLoss();
	}
	void Tree::printSubtree(int depth, bool sort, int top_n) const
	{
		int max_print_depth;
		if (depth < 0)
			max_print_depth = std::numeric_limits<int>::max();
		else
			max_print_depth = depth + ((root_node == nullptr) ? 0 : root_node->getDepth());
		print_node(root_node, max_print_depth, sort, top_n, 0);
	}

	std::shared_ptr<SharedHashTable<4>> Tree::getSharedHashTable() noexcept
	{
		return shared_hash_table;
	}
	const matrix<Sign>& Tree::getBoard() const noexcept
	{
		return base_board;
	}
	Sign Tree::getSignToMove() const noexcept
	{
		return sign_to_move;
	}
	Node Tree::getInfo(const std::vector<Move> &moves) const
	{
		size_t counter = 0;
		Node *node = root_node;
		while (node != nullptr and counter < moves.size())
		{
			const Move seeked_move = moves[counter];
			auto iter = std::find_if(node->begin(), node->end(), [seeked_move](const Edge &edge)
			{	return edge.getMove().row == seeked_move.row and edge.getMove().col == seeked_move.col;});
			if (iter == node->end())
				return Node(); // no such edge within the tree
			else
				node = iter->getNode();
			counter++;
		}
		if (node == nullptr)
			return Node(); // not even a single node was found
		else
		{
			Node result(*node);
			result.createEdges(node->numberOfEdges());
			for (int i = 0; i < node->numberOfEdges(); i++)
				result.getEdge(i) = node->getEdge(i).copyInfo();
			return result;
		}
	}
	void Tree::clearNodeCacheStats() noexcept
	{
		node_cache.clearStats();
	}
	NodeCacheStats Tree::getNodeCacheStats() const noexcept
	{
		return node_cache.getStats();
	}

} /* namespace ag */
