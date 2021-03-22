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

namespace
{
	using namespace ag;
	Node* select_puct(Node *parent, const float exploration_constant)
	{
		assert(not parent->isLeaf());
		int selected = -1;
		float bestValue = -1.0f;
		const float sqrt_visit = exploration_constant * sqrt(parent->getVisits());
		for (int i = 0; i < parent->numberOfChildren(); i++)
		{
			const Node &child = parent->getChild(i);
			if (not child.isProven())
			{
				float PUCT = child.getValue() + child.getPolicyPrior() * sqrt_visit / (1.0f + child.getVisits());
				if (PUCT > bestValue)
				{
					selected = i;
					bestValue = PUCT;
				}
			}
		}
		assert(selected != -1);
		return &(parent->getChild(selected));
	}
	Node* select_by_visits(Node *parent)
	{
		assert(not parent->isLeaf());
		int selected = -1;
		int max_visits = -1;
		for (int i = 0; i < parent->numberOfChildren(); i++)
			if (parent->getChild(i).getVisits() > max_visits)
			{
				selected = i;
				max_visits = parent->getChild(i).getVisits();
			}
		assert(selected != -1);
		return &(parent->getChild(selected));
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
		std::string result;
		result += "used nodes = " + std::to_string(used_nodes) + '\n';
		result += "allocated nodes = " + std::to_string(allocated_nodes) + '\n';
		result += "proven nodes = " + std::to_string(proven_nodes) + '\n';
		return result;
	}
	TreeStats& TreeStats::operator+=(const TreeStats &other) noexcept
	{
		this->allocated_nodes += other.allocated_nodes;
		this->used_nodes += other.used_nodes;
		this->proven_nodes += other.proven_nodes;
		return *this;
	}

	Tree::Tree(TreeConfig treeOptions) :
			config(treeOptions)
	{
	}
	TreeStats Tree::getStats() const noexcept
	{
//		std::lock_guard<std::mutex> lock(tree_mutex);
		TreeStats result;
		result.allocated_nodes = allocatedNodes();
		result.used_nodes = usedNodes();
		for (size_t i = 0; i < nodes.size(); i++)
			for (int j = 0; j < config.bucket_size; j++)
			{
				result.proven_nodes += static_cast<int>(nodes[i][j].isProven());
				if (nodes[i][j].isProven())
				{

//					printSubtree(nodes[i][j], -1, true);
				}
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
	const Node& Tree::getRootNode() const noexcept
	{
		return root_node;
	}
	Node& Tree::getRootNode() noexcept
	{
		return root_node;
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
			current = select_puct(current, explorationConstant);
			current->applyVirtualLoss();
			trajectory.append(current, current->getMove());
		}
	}
	void Tree::expand(Node &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd)
	{
		assert(Move::getSign(parent.getMove()) != Sign::NONE);
		assert(movesToAdd.size() > 0);
		std::lock_guard<std::mutex> lock(tree_mutex);

		Node *children = reserve_nodes(movesToAdd.size());
		if (children == nullptr) // there are no nodes left in the tree
			return;

		parent.createChildren(children, movesToAdd.size());
		for (int i = 0; i < parent.numberOfChildren(); i++)
		{
			assert(Move::getSign(movesToAdd[i].first) == invertSign(Move::getSign(parent.getMove())));
			parent.getChild(i).clear();
			parent.getChild(i).setMove(movesToAdd[i].first);
			parent.getChild(i).setPolicyPrior(movesToAdd[i].second);
		}
	}
	void Tree::backup(SearchTrajectory &trajectory, float value, ProvenValue provenValue)
	{
		std::lock_guard<std::mutex> lock(tree_mutex);

		const Sign sign = trajectory.getLastMove().sign;
		for (int i = 0; i < trajectory.length(); i++)
		{
			if (trajectory.getMove(i).sign == sign)
				trajectory.getNode(i).updateValue(value);
			else
				trajectory.getNode(i).updateValue(1.0f - value);
			trajectory.getNode(i).cancelVirtualLoss();
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
	void Tree::getPlayoutDistribution(const Node &node, matrix<float> &result) const
	{
		std::lock_guard<std::mutex> lock(tree_mutex);

		switch (node.getVisits())
		{
			case 0:
				break;
			case 1:
				for (int i = 0; i < node.numberOfChildren(); i++)
				{
					Move move = node.getChild(i).getMove();
					result.at(move.row, move.col) = node.getChild(i).getPolicyPrior();
				}
				break;
			default:
				for (int i = 0; i < node.numberOfChildren(); i++)
				{
					Move move = node.getChild(i).getMove();
					result.at(move.row, move.col) = node.getChild(i).getVisits();
				}
				break;
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
			current = select_by_visits(current);
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

