/*
 * Tree.cpp
 *
 *  Created on: Mar 2, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <alphagomoku/mcts/Tree.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
	using namespace ag;

	template<typename T>
	Node* select_puct(Node *parent, const float exploration_constant, const T &get_value)
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
	Node* select_balanced(Node *parent)
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
	Node* select_by_visit(Node *parent)
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
	Node* select_by_value(Node *parent)
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
	bool update_proven_value(Node &parent)
	{
		if (parent.isLeaf())
			return parent.isProven();

		bool has_draw_child = false;
		int unknown_count = 0;
		for (int i = 0; i < parent.numberOfChildren(); i++)
		{
			const Node &child = parent.getChild(i);
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
	void print_subtree(const Node &node, const int max_depth, bool sort, int top_n, int current_depth, bool is_last_child = false)
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

	Tree::Tree(TreeConfig treeOptions) :
			config(treeOptions)
	{
	}
	uint64_t Tree::getMemory() const noexcept
	{
		return sizeof(Node) * allocatedNodes();
	}
	void Tree::clearStats() noexcept
	{
	}
	TreeStats Tree::getStats() const noexcept
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
	void Tree::clear() noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		current_index = { 0, 0 };
		root_node.clear();
	}
	int Tree::allocatedNodes() const noexcept
	{
		return static_cast<int>(nodes.size()) * config.bucket_size;
	}
	int Tree::usedNodes() const noexcept
	{
		return current_index.first * config.bucket_size + current_index.second;
	}
	bool Tree::isProven() const noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		return root_node.isProven();
	}
	const Node& Tree::getRootNode() const noexcept
	{
		return root_node;
	}
	Node& Tree::getRootNode() noexcept
	{
		return root_node;
	}
	void Tree::setBalancingDepth(int depth) noexcept
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		balancing_depth = depth;
	}
	bool Tree::isRootNode(const Node *node) const noexcept
	{
		return node == &root_node;
	}
	void Tree::select(SearchTrajectory &trajectory, float explorationConstant)
	{
		trajectory.clear();
		std::lock_guard<std::mutex> lock(tree_mutex);

		Node *current = &getRootNode();
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
	bool Tree::expand(Node &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd)
	{
		assert(Move::getSign(parent.getMove()) != Sign::NONE);
		assert(movesToAdd.size() > 0);
		std::lock_guard<std::mutex> lock(tree_mutex);
		if (parent.isLeaf() == false)
			return false; // node was already expanded

		Node *children = reserve_nodes(movesToAdd.size());
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
	void Tree::backup(SearchTrajectory &trajectory, Value value, ProvenValue provenValue)
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
	void Tree::cancelVirtualLoss(SearchTrajectory &trajectory)
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		for (int i = 0; i < trajectory.length(); i++)
			trajectory.getNode(i).cancelVirtualLoss();
	}
	void Tree::getPolicyPriors(const Node &node, matrix<float> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(0.0f);
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getPolicyPrior();
		}
	}
	void Tree::getPlayoutDistribution(const Node &node, matrix<float> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(0.0f);
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getVisits();
		}
	}
	void Tree::getProvenValues(const Node &node, matrix<ProvenValue> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(ProvenValue::UNKNOWN);
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getProvenValue();
		}
	}
	void Tree::getActionValues(const Node &node, matrix<Value> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		result.fill(Value());
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			Move move = iter->getMove();
			result.at(move.row, move.col) = iter->getValue();
		}
	}
	SearchTrajectory Tree::getPrincipalVariation()
	{
		SearchTrajectory result;
		std::lock_guard<std::mutex> lock(tree_mutex);

		Node *current = &getRootNode();
		result.append(current, current->getMove());
		while (not current->isLeaf())
		{
			current = select_by_visit(current);
			result.append(current, current->getMove());
		}
		return result;
	}
	void Tree::printSubtree(const Node &node, int depth, bool sort, int top_n) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);
		print_subtree(node, depth, sort, top_n, 0);
	}

	Node* Tree::reserve_nodes(int number)
	{
		if (usedNodes() + number > config.max_number_of_nodes)
			return nullptr;

		if (usedNodes() + number > allocatedNodes())
			nodes.push_back(std::make_unique<Node[]>(config.bucket_size));

		if (current_index.second + number > config.bucket_size)
		{
			current_index.first++;
			current_index.second = 0;
		}
		Node *result = nodes.at(current_index.first).get() + current_index.second;
		current_index.second += number;
		return result;
	}
}

