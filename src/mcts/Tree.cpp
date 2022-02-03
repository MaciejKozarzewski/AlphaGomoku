/*
 * Tree.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
	using namespace ag;

	template<typename T>
	Node_old* select_puct(Node_old *parent, const float exploration_constant, const T &get_value)
	{
		assert(parent->getVisits() > 0);
		const float my_value = 1.0f - get_value(parent);
		const float sqrt_visit = exploration_constant * sqrtf(parent->getVisits());
		auto selected = parent->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (auto iter = parent->begin(); iter < parent->end(); iter++)
			if (not iter->isProven())
			{
				float Q = (iter->getVisits() == 0) ? my_value : get_value(iter);
				float U = iter->getPolicyPrior() * sqrt_visit / (1.0f + iter->getVisits()); // classical PUCT formula

				if (Q + U > bestValue)
				{
					selected = iter;
					bestValue = Q + U;
				}
			}
		assert(selected != parent->end());
		return selected;
	}
	Node_old* select_balanced(Node_old *parent)
	{
		MaxBalance get_value;
		auto selected = parent->end();
		float bestValue = std::numeric_limits<float>::lowest();
		for (auto iter = parent->begin(); iter < parent->end(); iter++)
			if (get_value(iter) > bestValue)
			{
				selected = iter;
				bestValue = get_value(iter);
			}
		assert(selected != parent->end());
		return selected;
	}
	Node_old* select_by_visit(Node_old *parent)
	{
		auto selected = parent->end();
		double bestValue = std::numeric_limits<float>::lowest();
		for (auto iter = parent->begin(); iter < parent->end(); iter++)
		{
			double value = iter->getVisits() + MaxExpectation()(iter) + 0.001 * iter->getPolicyPrior()
					+ ((int) (iter->getProvenValue() == ProvenValue::WIN) - (int) (iter->getProvenValue() == ProvenValue::LOSS)) * 1e9;
			if (value > bestValue)
			{
				selected = iter;
				bestValue = value;
			}
		}
		assert(selected != parent->end());
		return selected;
	}
	Node_old* select_by_value(Node_old *parent)
	{
		auto selected = parent->end();
//		float max_value = std::numeric_limits<float>::lowest();
//		for (auto iter = parent->begin(); iter < parent->end(); iter++)
//			switch (iter->getProvenValue())
//			{
//				case ProvenValue::UNKNOWN:
//					if (iter->getValue() > max_value)
//					{
//						selected = iter;
//						max_value = iter->getValue();
//					}
//					break;
//				case ProvenValue::LOSS:
//					if (iter->getValue() > max_value)
//					{
//						selected = iter;
//						max_value = -1.0f + iter->getValue();
//					}
//					break;
//				case ProvenValue::DRAW:
//					if (0.5f > max_value)
//					{
//						selected = iter;
//						max_value = 0.5f;
//					}
//					break;
//				case ProvenValue::WIN:
//					return iter;
//			}
//		assert(selected != parent->end());
		return selected;
	}
	bool update_proven_value(Node_old &parent)
	{
		// the node can be proven as leaf if it represents terminal state
		// or it can be proven as non-leaf if VCF solver has been used
		if (parent.isLeaf() or parent.isProven())
			return parent.isProven();

		bool has_draw_child = false;
		int unknown_count = 0;
		for (int i = 0; i < parent.numberOfChildren(); i++)
		{
			const Node_old &child = parent.getChild(i);
			unknown_count += static_cast<int>(child.getProvenValue() == ProvenValue::UNKNOWN);
			if (child.getProvenValue() == ProvenValue::WIN)
			{
				parent.setProvenValue(ProvenValue::LOSS);
				return true;
			}
			if (child.getProvenValue() == ProvenValue::DRAW)
				has_draw_child = true;
		}
		if (unknown_count == 0)
		{
			if (has_draw_child)
				parent.setProvenValue(ProvenValue::DRAW);
			else
				parent.setProvenValue(ProvenValue::WIN);
		}
		else
			return false;
		return true;
	}
	void print_subtree(const Node_old &node, const int max_depth, bool sort, int top_n, int current_depth, bool is_last_child = false)
	{
		for (int i = 0; i < current_depth - 1; i++)
			std::cout << "│  ";
		if (current_depth > 0)
		{
			if (is_last_child)
				std::cout << "└──";
			else
				std::cout << "├──";
		}
		std::cout << node.toString() << '\n';
		if (node.isLeaf())
		{
			if (is_last_child)
			{
				for (int i = 0; i < current_depth - 1; i++)
					std::cout << "│  ";
				std::cout << '\n';
			}
			return;
		}
		if (max_depth != -1 && current_depth >= max_depth)
		{
			for (int i = 0; i < current_depth; i++)
				std::cout << "│  ";
			std::cout << "└──... (subtree skipped)\n";
			for (int i = 0; i < current_depth; i++)
				std::cout << "│  ";
			std::cout << '\n';
			return;
		}
		if (sort)
			node.sortChildren();
		if (top_n == -1 || 2 * top_n >= node.numberOfChildren())
		{
			for (int i = 0; i < node.numberOfChildren(); i++)
				print_subtree(node.getChild(i), max_depth, sort, top_n, current_depth + 1, i == node.numberOfChildren() - 1);
		}
		else
		{
			for (int i = 0; i < top_n; i++)
				print_subtree(node.getChild(i), max_depth, sort, top_n, current_depth + 1);
			for (int i = 0; i < current_depth; i++)
				std::cout << "│  ";
			std::cout << "├──... (" << (node.numberOfChildren() - 2 * top_n) << " nodes skipped)\n";
			for (int i = node.numberOfChildren() - top_n; i < node.numberOfChildren(); i++)
				print_subtree(node.getChild(i), max_depth, sort, top_n, current_depth + 1, i == node.numberOfChildren() - 1);
		}
	}

	void print_subtree(const Node *node, const int max_depth, bool sort, int top_n, int current_depth, bool is_last_child = false);
	void print_subtree(const Edge &edge, const int max_depth, bool sort, int top_n, int current_depth, bool is_last_child = false)
	{
		if (current_depth > 0)
		{
			if (is_last_child)
			{
				for (int i = 0; i < current_depth - 1; i++)
					std::cout << "   " << "   ";
				std::cout << "└──";
			}
			else
			{
				for (int i = 0; i < current_depth - 1; i++)
					std::cout << "│  " << "   ";
				std::cout << "├──";
			}
		}
		std::cout << edge.toString() << '\n';
		if (edge.isLeaf() == false)
		{
			for (int i = 0; i < current_depth; i++)
				std::cout << "│  ";
			std::cout << "└──";
			print_subtree(edge.getNode(), max_depth, sort, top_n, current_depth, is_last_child);
		}
	}
	void print_subtree(const Node *node, const int max_depth, bool sort, int top_n, int current_depth, bool is_last_child)
	{
		if (node == nullptr)
			return;
		std::cout << node << " " << node->toString() << '\n';
		if (max_depth != -1 && current_depth >= max_depth)
		{
			for (int i = 0; i < 2 * current_depth; i++)
				std::cout << "│  ";
			std::cout << "└──... (subtree skipped)\n";
			for (int i = 0; i < 2 * current_depth; i++)
				std::cout << "│  ";
			std::cout << '\n';
			return;
		}
		if (sort)
			node->sortEdges();
		if (top_n == -1 || 2 * top_n >= node->numberOfEdges())
		{
			for (int i = 0; i < node->numberOfEdges(); i++)
				print_subtree(node->getEdge(i), max_depth, sort, top_n, current_depth + 1, i == node->numberOfEdges() - 1);
		}
		else
		{
			for (int i = 0; i < top_n; i++)
				print_subtree(node->getEdge(i).getNode(), max_depth, sort, top_n, current_depth + 1);
			for (int i = 0; i < 2 * current_depth; i++)
				std::cout << "│  ";
			std::cout << "├──... (" << (node->numberOfEdges() - 2 * top_n) << " nodes skipped)\n";
			for (int i = node->numberOfEdges() - top_n; i < node->numberOfEdges(); i++)
				print_subtree(node->getEdge(i), max_depth, sort, top_n, current_depth + 1, i == node->numberOfEdges() - 1);
		}
	}

	bool is_information_leak(const Edge *edge, const Node *node) noexcept
	{
		constexpr float leak_threshold = 0.01f;
		assert(edge != nullptr);
		assert(edge->getNode() == node); // ensure that they are linked
		if (node == nullptr)
			return false; // the edge is a leaf so there is no information leak
		if (edge->getProvenValue() != node->getProvenValue())
			return true;
		if ((edge->getValue() - node->getValue()).abs() >= leak_threshold)
			return true;
		return false;
	}
	void update_proven_value(Edge *edge) noexcept
	{
		assert(edge != nullptr);
		assert(edge->isLeaf() == false);
		edge->setProvenValue(edge->getNode()->getProvenValue());
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
				node->setProvenValue(ProvenValue::LOSS);
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
				node->setProvenValue(ProvenValue::WIN);
		}
		else
			return false;
		return true;
	}
}

namespace ag
{
	std::string TreeStats::toString() const
	{
		std::string result = "----TreeStats----\n";
		result += "used nodes      = " + std::to_string(used_nodes) + '\n';
		result += "allocated nodes = " + std::to_string(allocated_nodes) + '\n';
		result += "proven nodes    = " + std::to_string(proven_win) + " : " + std::to_string(proven_draw) + " : " + std::to_string(proven_loss)
				+ " (win:draw:loss)\n";
		return result;
	}
	TreeStats& TreeStats::operator+=(const TreeStats &other) noexcept
	{
		this->allocated_nodes += other.allocated_nodes;
		this->used_nodes += other.used_nodes;
		this->proven_loss += other.proven_loss;
		this->proven_draw += other.proven_draw;
		this->proven_win += other.proven_win;
		return *this;
	}
	TreeStats& TreeStats::operator/=(int i) noexcept
	{
		this->allocated_nodes /= i;
		this->used_nodes /= i;
		this->proven_loss /= i;
		this->proven_draw /= i;
		this->proven_win /= i;
		return *this;
	}

	Tree::Tree(GameConfig gameOptions, TreeConfig treeOptions) :
			node_cache(gameOptions)
	{
	}
	Tree::~Tree()
	{
		delete_subtree(root_node);
	}
	void Tree::setBoard(const matrix<Sign> &newBoard, Sign signToMove)
	{
		if (newBoard == base_board and signToMove == sign_to_move)
			return;

		matrix<Sign> tmp_board = base_board;
		base_board = newBoard;
		sign_to_move = signToMove;

		prune_subtree(root_node, tmp_board);

		root_node = node_cache.seek(base_board, sign_to_move);
	}
	int Tree::getSimulationCount() const noexcept
	{
		if (root_node == nullptr)
			return 0;
		else
			return root_node->getVisits();
	}
	SelectOutcome Tree::select(SearchTask &task, const EdgeSelector &selector)
	{
		task.reset(base_board, sign_to_move);
		Node *node = root_node;
		while (node != nullptr)
		{
			Edge *edge = selector.select(node);
			task.append(node, edge);
			edge->increaseVirtualLoss();

			node = edge->getNode();
			if (node == nullptr) // edge appears to be a leaf
			{
				node = node_cache.seek(task.getBoard(), task.getSignToMove()); // try to find the node in cache
				if (node != nullptr) // if found in the cache
					edge->setNode(node); // link that edge to this node
			}

			if (is_information_leak(edge, node))
				return SelectOutcome::INFORMATION_LEAK;
		}
		return SelectOutcome::REACHED_LEAF;
	}
	ExpandOutcome Tree::expand(const SearchTask &task)
	{
		assert(task.isReady());
		if (task.visitedPathLength() > 0)
		{
			if (task.getLastPair().edge->isLeaf() == false)
				return ExpandOutcome::ALREADY_EXPANDED; // can happen either because of multithreading or minibatch evaluation
		}
		else
		{
			if (root_node != nullptr)
				return ExpandOutcome::ALREADY_EXPANDED; // rare case if the tree has just been initialized but root node was already evaluated
		}

		const size_t number_of_moves = task.getEdges().size();
		assert(number_of_moves > 0);
		Edge *new_edges = edge_pool.allocate(number_of_moves);
		for (size_t i = 0; i < number_of_moves; i++)
			new_edges[i] = task.getEdges()[i];

		Node *node_to_add = node_cache.insert(task.getBoard(), task.getSignToMove());
		node_to_add->clear();
		node_to_add->setEdges(new_edges, number_of_moves);
		node_to_add->updateValue(task.getValue().getInverted());
		update_proven_value(node_to_add);
		node_to_add->setSignToMove(task.getSignToMove());
		node_to_add->markAsUsed();

		if (task.visitedPathLength() > 0)
		{
			task.getLastPair().edge->setNode(node_to_add); // make last visited edge point to the newly added node
			node_to_add->setDepth(root_node->getDepth() + task.visitedPathLength());
		}
		else
		{
			root_node = node_to_add; // if no pairs were visited, it means that the tree is empty
			root_node->setDepth(Board::numberOfMoves(base_board));
		}
		return ExpandOutcome::SUCCESS;
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
				pair.node->updateValue(value.getInverted());
				pair.edge->updateValue(value);
			}
			else
			{
				pair.node->updateValue(value);
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
		print_subtree(root_node, depth, sort, top_n, 0);
	}

// private
	void Tree::prune_subtree(Node *node, matrix<Sign> &tmpBoard)
	{
		if (node == nullptr)
			return;
		std::cout << "tmp board\n" << Board::toString(tmpBoard) << '\n';
		std::cout << "base board\n" << Board::toString(base_board) << '\n';
		if (Board::isTransitionPossible(base_board, tmpBoard))
			return; // if the board state at this node is possible, then all nodes in the subtree below also are
		else
		{
			if (node->isLeaf() == false)
			{
				for (auto edge = node->begin(); edge < node->end(); edge++)
					if (edge->getNode() != nullptr and edge->getNode()->isUsed())
					{
						Board::putMove(tmpBoard, edge->getMove());
						prune_subtree(edge->getNode(), tmpBoard);
						Board::undoMove(tmpBoard, edge->getMove());
					}
			}
			edge_pool.free(node->begin(), node->numberOfEdges());
			node->markAsUnused();
			node_cache.remove(tmpBoard, node->getSignToMove());
		}
	}
	void Tree::delete_subtree(Node *node)
	{
		if (node == nullptr)
			return;
		if (node->isLeaf() == false)
		{
			for (auto edge = node->begin(); edge < node->end(); edge++)
				if (edge->getNode() != nullptr and edge->getNode()->isUsed())
					delete_subtree(edge->getNode());
		}
		edge_pool.free(node->begin(), node->numberOfEdges());
		node->markAsUnused();
//		node_cache.remove(currentBoard, signToMove);
	}

	Tree_old::Tree_old(TreeConfig treeOptions) :
			config(treeOptions)
	{
	}

	uint64_t Tree_old::getMemory() const noexcept
	{
		return sizeof(Node_old) * usedNodes();
//		return sizeof(Node_old) * allocatedNodes();
	}
	void Tree_old::clearStats() noexcept
	{
	}
	TreeStats Tree_old::getStats() const noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		TreeStats result;
		result.allocated_nodes = allocatedNodes();
		result.used_nodes = usedNodes();
		for (size_t i = 0; i < nodes.size(); i++)
			for (int j = 0; j < config.bucket_size; j++)
				switch (nodes[i][j].getProvenValue())
				{
					default:
					case ProvenValue::UNKNOWN:
						break;
					case ProvenValue::LOSS:
						result.proven_loss++;
						break;
					case ProvenValue::DRAW:
						result.proven_draw++;
						break;
					case ProvenValue::WIN:
						result.proven_win++;
						break;
				}
		return result;
	}
	void Tree_old::clear() noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		current_index = { 0, 0 };
		root_node.clear();
	}
	int Tree_old::allocatedNodes() const noexcept
	{
		return static_cast<int>(nodes.size()) * config.bucket_size;
	}
	int Tree_old::usedNodes() const noexcept
	{
		return current_index.first * config.bucket_size + current_index.second;
	}
	bool Tree_old::isProven() const noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		return root_node.isProven();
	}
	const Node_old& Tree_old::getRootNode() const noexcept
	{
		return root_node;
	}
	Node_old& Tree_old::getRootNode() noexcept
	{
		return root_node;
	}
	void Tree_old::setBalancingDepth(int depth) noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		balancing_depth = depth;
	}
	bool Tree_old::isRootNode(const Node_old *node) const noexcept
	{
		return node == &root_node;
	}
	void Tree_old::select(SearchTrajectory_old &trajectory, float explorationConstant)
	{
		trajectory.clear();
		std::lock_guard<std::mutex> lock(tree_mutex);

		Node_old *current = &getRootNode();
		current->applyVirtualLoss();
		trajectory.append(current, current->getMove());
		while (not current->isLeaf())
		{
			if (trajectory.length() <= balancing_depth)
				current = select_balanced(current);
			else
				current = select_puct(current, explorationConstant, MaxExpectation());
			current->applyVirtualLoss();
			trajectory.append(current, current->getMove());
		}
	}
	bool Tree_old::expand(Node_old &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd)
	{
		assert(Move::getSign(parent.getMove()) != Sign::NONE);
		assert(movesToAdd.size() > 0);
		std::lock_guard<std::mutex> lock(tree_mutex);
		if (parent.isLeaf() == false)
			return false; // node has already been expanded

		Node_old *children = reserve_nodes(movesToAdd.size());
		if (children == nullptr) // there are no nodes left in the tree
			return true; // don't expand if there is no memory

		parent.createChildren(children, movesToAdd.size());
		for (int i = 0; i < parent.numberOfChildren(); i++)
		{
			assert(Move::getSign(movesToAdd[i].first) == invertSign(Move::getSign(parent.getMove())));
			parent.getChild(i).clear();
			parent.getChild(i).setMove(movesToAdd[i].first);
			parent.getChild(i).setPolicyPrior(movesToAdd[i].second);
		}
		return true;
	}
	void Tree_old::backup(SearchTrajectory_old &trajectory, Value value, ProvenValue provenValue)
	{
		std::lock_guard<std::mutex> lock(tree_mutex);

		if (trajectory.getMove(0).sign != trajectory.getLastMove().sign)
			value.invert();
		for (int i = 0; i < trajectory.length(); i++)
		{
			trajectory.getNode(i).updateValue(value);
			trajectory.getNode(i).cancelVirtualLoss();
			value.invert();
		}
		trajectory.getLeafNode().setProvenValue(provenValue);
		for (int i = trajectory.length() - 1; i >= 0; i--)
		{
			bool continue_updating = update_proven_value(trajectory.getNode(i));
			if (continue_updating == false)
				break;
		}
	}
	void Tree_old::cancelVirtualLoss(SearchTrajectory_old &trajectory)
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		for (int i = 0; i < trajectory.length(); i++)
			trajectory.getNode(i).cancelVirtualLoss();
	}
	void Tree_old::getPolicyPriors(const Node_old &node, matrix<float> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(0.0f);
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getPolicyPrior();
		}
	}
	void Tree_old::getPlayoutDistribution(const Node_old &node, matrix<float> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(0.0f);
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getVisits();
		}
	}
	void Tree_old::getProvenValues(const Node_old &node, matrix<ProvenValue> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(ProvenValue::UNKNOWN);
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getProvenValue();
		}
	}
	void Tree_old::getActionValues(const Node_old &node, matrix<Value> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(Value());
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getValue();
		}
	}
	SearchTrajectory_old Tree_old::getPrincipalVariation()
	{
		SearchTrajectory_old result;
		std::lock_guard<std::mutex> lock(tree_mutex);

		Node_old *current = &getRootNode();
		result.append(current, current->getMove());
		while (not current->isLeaf())
		{
			current = select_by_visit(current);
			result.append(current, current->getMove());
		}
		return result;
	}
	void Tree_old::printSubtree(const Node_old &node, int depth, bool sort, int top_n) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		print_subtree(node, depth, sort, top_n, 0);
	}

	Node_old* Tree_old::reserve_nodes(int number)
	{
		if (usedNodes() + number > allocatedNodes()) // allocate new bucket
			nodes.push_back(std::make_unique<Node_old[]>(config.bucket_size));

		if (current_index.second + number > config.bucket_size)
		{
			current_index.first++; // switch to next bucket
			current_index.second = 0;
		}
		Node_old *result = nodes.at(current_index.first).get() + current_index.second;
		current_index.second += number;
		return result;
	}
}

