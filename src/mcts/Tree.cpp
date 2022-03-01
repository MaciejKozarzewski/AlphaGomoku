/*
 * Tree.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>

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

	bool is_terminal(const Edge *edge) noexcept
	{
		return edge->isProven() and edge->isLeaf();
	}
	bool is_information_leak(const Edge *edge, const Node *node) noexcept
	{
		constexpr float leak_threshold = 0.01f;
		assert(edge != nullptr);
		assert(edge->getNode() == node); // ensure that they are linked
		if (node == nullptr)
			return false; // the edge is a leaf so there is no information leak
		if (edge->getProvenValue() != invert(node->getProvenValue()))
			return true;
		if ((edge->getValue() - node->getValue().getInverted()).abs() >= leak_threshold)
			return true;
		return false;
	}
	void update_proven_value(Edge *edge) noexcept
	{
		assert(edge != nullptr);
		assert(edge->isLeaf() == false);
		edge->setProvenValue(invert(edge->getNode()->getProvenValue()));
	}
	bool update_proven_value(Node *node) noexcept
	{
		assert(node != nullptr);
		assert(node->isLeaf() == false);
		if (node->isProven())
			return true; // if node is already proven there is no need to analyze its edges

		bool has_draw_child = false;
		int unknown_count = 0;
		for (Edge *edge = node->begin(); edge < node->end(); edge++)
		{
			unknown_count += static_cast<int>(edge->getProvenValue() == ProvenValue::UNKNOWN);
			if (edge->getProvenValue() == ProvenValue::WIN)
			{
				node->setProvenValue(ProvenValue::WIN);
				return true;
			}
			if (edge->getProvenValue() == ProvenValue::DRAW)
				has_draw_child = true;
		}
		if (unknown_count == 0)
		{
			if (has_draw_child)
				node->setProvenValue(ProvenValue::DRAW);
			else
				node->setProvenValue(ProvenValue::LOSS);
		}
		else
			return false;
		return true;
	}
}

namespace ag
{
	Tree::Tree(TreeConfig treeOptions) :
			config(treeOptions)
	{
	}
	int64_t Tree::getMemory() const noexcept
	{
		return node_cache.getMemory();
	}
	void Tree::setBoard(const matrix<Sign> &newBoard, Sign signToMove, bool forceRemoveRootNode)
	{
		if (forceRemoveRootNode and node_cache.seek(newBoard, signToMove) != nullptr)
			node_cache.remove(newBoard, signToMove);

		if (not equalSize(base_board, newBoard))
		{
			node_cache = NodeCache(newBoard.rows(), newBoard.cols(), config.initial_cache_size, config.bucket_size);
			edge_selector = nullptr; // must clear selector in case it uses information about board size
			edge_generator = nullptr; // must clear generator in case it uses information about board size
		}
		else
			node_cache.cleanup(newBoard, signToMove);

		base_board = newBoard;
		base_depth = Board::numberOfMoves(newBoard);
		sign_to_move = signToMove;

		root_node = node_cache.seek(newBoard, signToMove);
		max_depth = 0;
	}
	void Tree::setEdgeSelector(const EdgeSelector &selector)
	{
		edge_selector = std::unique_ptr<EdgeSelector>(selector.clone());
	}
	void Tree::setEdgeGenerator(const EdgeGenerator &generator)
	{
		edge_generator = std::unique_ptr<EdgeGenerator>(generator.clone());
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
	bool Tree::hasSingleNonLosingMove() const noexcept
	{
		if (root_node == nullptr)
			return false;
		else
		{
			int non_losing_edges = std::count_if(root_node->begin(), root_node->end(), [](const Edge &edge)
			{	return edge.getProvenValue() != ProvenValue::LOSS;});
			return non_losing_edges == 1;
		}
	}

	SelectOutcome Tree::select(SearchTask &task)
	{
		task.reset(base_board, sign_to_move);
		Node *node = root_node;
		while (node != nullptr)
		{
			assert(edge_selector != nullptr);
			Edge *edge = edge_selector->select(node);
			task.append(node, edge);
			edge->increaseVirtualLoss();

			node = edge->getNode();
			if (node == nullptr) // edge appears to be a leaf
			{
				node = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find board state in cache
				if (node != nullptr) // if found in the cache
					edge->setNode(node); // link that edge to the found node
			}

			if (is_information_leak(edge, node))
			{
				correct_information_leak(task);
				return SelectOutcome::INFORMATION_LEAK;
			}
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
		assert(task.isReady());

		Node *node_to_add = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find board state in the cache
		if (node_to_add == nullptr) // not found in the cache
		{
			const int64_t number_of_moves = static_cast<int64_t>(task.getEdges().size());
			assert(number_of_moves > 0);

			node_to_add = node_cache.insert(task.getBoard(), task.getSignToMove(), number_of_moves);
			for (int64_t i = 0; i < number_of_moves; i++)
				node_to_add->getEdge(i) = task.getEdges()[i];
			node_to_add->updateValue(task.getValue());
			update_proven_value(node_to_add);

			if (task.visitedPathLength() > 0)
				task.getLastEdge()->setNode(node_to_add); // make last visited edge point to the newly added node
			else
				root_node = node_to_add; // if no pairs were visited, it means that the tree is empty and we have to assign the root node

			return ExpandOutcome::SUCCESS;
		}
		else
		{
			// this can happen if the same state was encountered from different paths within the same batch
			if (task.visitedPathLength() > 0) // in a rare case root node was expanded by some other thread
			{
				task.getLastEdge()->setNode(node_to_add); // make last visited edge point to the newly added node
				if (is_information_leak(task.getLastEdge(), node_to_add))
					correct_information_leak(task);
			}
			return ExpandOutcome::ALREADY_EXPANDED;
		}
	}
	void Tree::backup(const SearchTask &task)
	{
		assert(task.isReady());
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
			update_proven_value(pair.edge);
			bool continue_updating = update_proven_value(pair.node);
			if (continue_updating == false)
				break;
		}
	}
	void Tree::cancelVirtualLoss(SearchTask &task) noexcept
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
			Move seeked_move = moves[counter];
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
	void Tree::freeMemory()
	{
		node_cache.freeUnusedMemory();
	}
	/*
	 * private
	 */
	void Tree::correct_information_leak(SearchTask &task) const
	{
		assert(task.visitedPathLength() > 0);

		Edge *edge = task.getLastEdge();
		Node *node = edge->getNode();
		assert(node != nullptr);
		Value edgeQ = edge->getValue();
		Value nodeQ = node->getValue().getInverted(); // edgeQ should be equal to 1 - nodeQ

//		Value v = (nodeQ - edgeQ) * edge->getVisits() + nodeQ;
		Value v = (nodeQ - edgeQ) * std::min(Edge::update_threshold, edge->getVisits()) + nodeQ; // TODO
		task.setValue(v.getInverted()); // the leak is between the current node and the edge that leads to it
		task.getLastEdge()->setProvenValue(invert(node->getProvenValue()));
		task.setReady();
	}

} /* namespace ag */

