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
//		for (int i = 0; i < prefix; i++)
//			std::cout << "|     ";
//		if (is_last)
//			std::cout << "└──" << edge.toString() << '\n';
//		else
//			std::cout << "├──" << edge.toString() << '\n';
//
//		if (edge.getNode() != nullptr)
//		{
//			for (int i = 0; i < prefix; i++)
//				std::cout << "|     ";
//			std::cout << "|  └──";
//			print_node(edge.getNode(), max_depth, sort, top_n, prefix + 1);
//		}
	}
	void print_node(const Node *node, const int max_depth, bool sort, int top_n, int prefix)
	{
//		if (node == nullptr)
//			return;
//		if (max_depth != -1 and node->getDepth() <= max_depth)
//		{
//			if (sort)
//				node->sortEdges();
//
//			std::cout << node->toString() << '\n';
//			if (top_n == -1 || 2 * top_n >= node->numberOfEdges())
//			{
//				for (int i = 0; i < node->numberOfEdges(); i++)
//					print_edge(node->getEdge(i), max_depth, sort, top_n, i == (node->numberOfEdges() - 1), prefix);
//			}
//			else
//			{
//				for (int i = 0; i < top_n; i++)
//					print_edge(node->getEdge(i), max_depth, sort, top_n, false, prefix);
//				for (int i = 0; i < prefix; i++)
//					std::cout << "|     ";
//				std::cout << "├──... (" << (node->numberOfEdges() - 2 * top_n) << " edges skipped)\n";
//				for (int i = node->numberOfEdges() - top_n; i < node->numberOfEdges(); i++)
//					print_edge(node->getEdge(i), max_depth, sort, top_n, i == (node->numberOfEdges() - 1), prefix);
//			}
//		}
//		else
//		{
//			std::cout << "... (subtree skipped)\n";
//			for (int i = 0; i < prefix; i++)
//				std::cout << "|     ";
//			std::cout << '\n';
//			return;
//		}
	}

	bool has_information_leak(const Edge *edge, const Node *node, float leak_threshold) noexcept
	{
		assert(edge != nullptr);
		assert(leak_threshold >= 0.0f);
		if (node == nullptr or leak_threshold >= 1.0f)
			return false;
		if (edge->getScore() != invert_up(node->getScore()))
			return true;
		const Value diff = edge->getValue() - node->getValue().getInverted();
		return diff.abs() > leak_threshold;
	}

	void update_score(Edge *edge, const Node *node) noexcept
	{
		assert(edge != nullptr);
		if (node != nullptr)
			edge->setScore(invert_up(node->getScore()));
	}
	void update_score(Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);

		Score result = Score::min_value();
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
			result = std::max(result, edge->getScore());

		if (node->isFullyExpanded() or result.isWin() or result.isUnproven())
			node->setScore(result);
	}
	Node* get_next_node(const SearchTask &task, int idx) noexcept
	{
		if (idx == (task.visitedPathLength() - 1))
			return task.getFinalNode();
		else
			return task.getPair(idx + 1).node;
	}
}

namespace ag
{
	Tree::Tree(const TreeConfig &treeConfig) :
			config(treeConfig)
	{
	}
	int64_t Tree::getMemory() const noexcept
	{
		return node_cache.getMemory();
	}
	void Tree::clear()
	{
		node_cache.clear();
	}
	void Tree::setBoard(const matrix<Sign> &newBoard, Sign signToMove, bool forceRemoveRootNode)
	{
		if (not equal_shape(base_board, newBoard))
		{
			const GameConfig game_config(GameRules::FREESTYLE, newBoard.rows(), newBoard.cols()); // game rule specified here is irrelevant
			node_cache = NodeCache(game_config, config);
			edge_selector = nullptr; // must clear selector in case it uses information about board size
			edge_generator = nullptr; // must clear generator in case it uses information about board size
		}
		else
			node_cache.cleanup(newBoard, signToMove);

		base_board = newBoard;
		move_number = Board::numberOfMoves(newBoard);
		evaluation = 0.0f;
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

	int Tree::getMoveNumber() const noexcept
	{
		return move_number;
	}
	float Tree::getEvaluation() const noexcept
	{
		return evaluation;
	}
	float Tree::getMovesLeft() const noexcept
	{
		return moves_left;
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
	bool Tree::isRootProven() const noexcept
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
			node->increaseVirtualLoss();
			edge->increaseVirtualLoss();

			if (edge->isProven())
				return SelectOutcome::REACHED_PROVEN_EDGE;

			node = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find board state in cache
			task.setFinalNode(node);
			if (node == nullptr)
				edge->markAsBeingExpanded();

			if (has_information_leak(edge, node, config.information_leak_threshold))
				return SelectOutcome::INFORMATION_LEAK;
		}
		max_depth = std::max((int) max_depth, task.visitedPathLength());
		return SelectOutcome::REACHED_LEAF;
	}
	void Tree::generateEdges(SearchTask &task) const
	{
		assert(edge_generator != nullptr);
		edge_generator->generate(task);
	}
	ExpandOutcome Tree::expand(SearchTask &task)
	{
		if (task.getEdges().size() == 0)
			return ExpandOutcome::SKIPPED_EXPANSION;

		Node *node_to_add = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find board state in the cache
		if (node_to_add == nullptr)
		{ // not found in the cache
			const int number_of_edges = task.getEdges().size();
			assert(number_of_edges > 0);

			node_to_add = node_cache.insert(task.getBoard(), task.getSignToMove(), number_of_edges);
			for (int i = 0; i < number_of_edges; i++)
				node_to_add->getEdge(i) = task.getEdges()[i];
			node_to_add->updateValue(task.getValue()); // 'updateValue' is called because it increases visit count from 0 to 1 ('setValue' doesn't do that)
			node_to_add->updateMovesLeft(task.getMovesLeft());

			if (task.mustDefend() or (node_to_add->numberOfEdges() + node_to_add->getDepth()) == task.getBoard().size())
				node_to_add->markAsFullyExpanded();
			update_score(node_to_add);
			task.setFinalNode(node_to_add);

			if (task.visitedPathLength() == 0)
			{
				root_node = node_to_add; // if no pairs were visited, it means that the tree is empty and we have to assign the root node
				root_node->markAsRoot();
			}

			return ExpandOutcome::SUCCESS;
		}
		else
		{ // this can happen if the same state was encountered from different paths
			task.setFinalNode(node_to_add);
			if (task.visitedPathLength() > 0) // in a rare case it could be that the root node has already been expanded by some other thread
			{ // but if not
				if (has_information_leak(task.getLastEdge(), node_to_add, config.information_leak_threshold))
					correctInformationLeak(task);
			}
			return ExpandOutcome::ALREADY_EXPANDED;
		}
	}
	void Tree::backup(const SearchTask &task)
	{
		assert(task.isReady());

//		const float weight = config.weight_c / std::max(config.min_uncertainty, task.getUncertainty());
		float moves_left = task.getMovesLeft();
		Value value = task.getValue();
		for (int i = task.visitedPathLength() - 1; i >= 0; i--)
		{
			NodeEdgePair pair = task.getPair(i);
			Node *next_node = get_next_node(task, i);
//			if (next_node != nullptr)
//			if (has_value_leak(pair.edge, next_node, 1.0e-6f))
//			{
//				const Value current_edge_value = pair.edge->getValue();
//				const Value target_edge_value = next_node->getValue().getInverted(); // edge Q should be equal to (1 - node Q)
//
//				assert(pair.node->getVisits() != 0);
//				const float scale = static_cast<float>(pair.edge->getVisits()) / static_cast<float>(pair.node->getVisits());
//				const Value target_node_value = pair.node->getValue() + (target_edge_value - current_edge_value) * scale;
//
//				pair.edge->setValue(target_edge_value);
//				pair.node->setValue(target_node_value);
//
////				value = (target_edge_value - current_edge_value) * pair.edge->getVisits() + target_edge_value;
////
////				const float scale = 1.0f / std::max(1.0f, std::abs(value.win_rate) + std::abs(value.draw_rate));
////				value *= scale;
//			}
			if (pair.node->getSignToMove() == task.getSignToMove())
				value = task.getValue();
			else
				value = task.getValue().getInverted();

			pair.node->updateValue(value);
			pair.edge->updateValue(value);

			pair.node->updateMovesLeft(moves_left);
			moves_left += 1.0f;

//			pair.edge->updateDistribution(value);
			update_score(pair.edge, next_node);
			update_score(pair.node);

//			value = value.getInverted();
			pair.node->decreaseVirtualLoss();
			pair.edge->decreaseVirtualLoss();
			pair.edge->clearFlags();
		}

		this->evaluation = root_node->getExpectation();
		this->moves_left = root_node->getMovesLeft();
	}
	void Tree::correctInformationLeak(const SearchTask &task)
	{
//		std::cout << "\n\nFound leak\n";
		for (int i = task.visitedPathLength() - 1; i >= 0; i--)
		{
			NodeEdgePair pair = task.getPair(i);
			Node *next_node = get_next_node(task, i);
			const Value current_edge_value = pair.edge->getValue();
			const Value target_edge_value = next_node->getValue().getInverted(); // edge Q should be equal to (1 - node Q)

			assert(pair.node->getVisits() != 0);
			const float scale = static_cast<float>(pair.edge->getVisits()) / static_cast<float>(pair.node->getVisits());
			const Value target_node_value = pair.node->getValue() + (target_edge_value - current_edge_value) * scale;

//			std::cout << pair.edge->toString() << " vs " << next_node->toString() << '\n';
//			std::cout << "Edge " << current_edge_value.toString() << " -> " << target_edge_value.toString() << '\n';
//			std::cout << "scale = " << scale << '\n';
//			std::cout << "Node " << pair.node->toString() << " -> " << target_node_value.toString() << '\n' << '\n';

			pair.edge->setValue(target_edge_value);
			pair.node->setValue(target_node_value);
			update_score(pair.edge, next_node);
			update_score(pair.node);
		}
	}
	void Tree::cancelVirtualLoss(const SearchTask &task) noexcept
	{
		for (int i = 0; i < task.visitedPathLength(); i++)
		{
			task.getPair(i).node->decreaseVirtualLoss();
			task.getPair(i).edge->decreaseVirtualLoss();
		}
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
		matrix<Sign> tmp_board = getBoard();
		Sign s = sign_to_move;
		for (auto move = moves.begin(); move < moves.end(); move++)
		{
			if (tmp_board.at(move->row, move->col) == Sign::NONE)
				tmp_board.at(move->row, move->col) = s;
			else
				return Node();
			s = invertSign(s);
		}
		Node *node = node_cache.seek(tmp_board, s); // try to find board state in cache
		if (node == nullptr)
			return Node();
		else
		{
			Node result(*node);
			copyEdgeInfo(result, *node);
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

	LowPriorityLock Tree::low_priority_lock() const
	{
		return LowPriorityLock(tree_mutex);
	}
	HighPriorityLock Tree::high_priority_lock() const
	{
		return HighPriorityLock(tree_mutex);
	}

} /* namespace ag */

