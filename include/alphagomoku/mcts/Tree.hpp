/*
 * Tree.hpp
 *
 *  Created on: Mar 2, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_TREE_HPP_
#define ALPHAGOMOKU_MCTS_TREE_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class Json;
namespace ag
{
	class SearchTrajectory;
} /* namespace ag */

namespace ag
{
	struct TreeStats
	{
			int64_t nb_used_nodes = 0;
			int64_t max_used_nodes = 0;
			int64_t nb_free = 0;

			void print() const;
			TreeStats& operator+=(const TreeStats &other) noexcept;
	};

	struct TreeConfig
	{
			uint32_t max_number_of_nodes = (1 << 20);
			uint32_t bucket_size = (1 << 10);
			float exploration_constant = 1.25f;
			float expansion_policy_treshold = 0.0f;
	};

	class Tree
	{
		private:
			std::vector<std::unique_ptr<Node[]>> nodes;
			std::pair<uint32_t, uint32_t> current_index { 0, 0 };
			mutable std::mutex tree_mutex;
			Node root_node;
			TreeConfig config;

		public:
			TreeStats stats;

			Tree();

			void clear() noexcept;
			uint32_t allocatedNodes() const noexcept;
			uint32_t usedNodes() const noexcept;

			const Node& getRootNode() const noexcept;
			Node& getRootNode() noexcept;
			void select(SearchTrajectory &trajectory);
			void expand(Node &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd);
			void backup(SearchTrajectory &trajectory, float value, ProvenValue provenValue);
			void cancelVirtualLoss(SearchTrajectory &trajectory);

			void getPlayoutDistribution(const Node &node, matrix<float> &result) const;
			SearchTrajectory getPrincipalVariation();
			void printSubtree(const Node &node, int depth = -1, bool sort = false, int top_n = -1) const;
		private:
			Node* reserve_nodes(int number);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_TREE_HPP_ */
