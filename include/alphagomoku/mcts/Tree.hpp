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
#include <alphagomoku/utils/configs.hpp>
#include <inttypes.h>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace ag
{
	class SearchTrajectory;
} /* namespace ag */

namespace ag
{
	struct TreeStats
	{
			uint64_t allocated_nodes = 0;
			uint64_t used_nodes = 0;
			uint64_t proven_loss = 0;
			uint64_t proven_draw = 0;
			uint64_t proven_win = 0;

			std::string toString() const;
			TreeStats& operator+=(const TreeStats &other) noexcept;
			TreeStats& operator/=(int i) noexcept;
	};

	class Tree
	{
		private:
			std::vector<std::unique_ptr<Node[]>> nodes;
			std::pair<int, int> current_index { 0, 0 };
			mutable std::mutex tree_mutex;
			Node root_node;

			TreeConfig config;

		public:
			Tree(TreeConfig treeOptions);
			uint64_t getMemory() const noexcept;
			void clearStats() noexcept;
			TreeStats getStats() const noexcept;

			void clear() noexcept;
			int allocatedNodes() const noexcept;
			int usedNodes() const noexcept;

			bool isProven() const noexcept;
			const Node& getRootNode() const noexcept;
			Node& getRootNode() noexcept;
			bool isRootNode(const Node *node) const noexcept;
			void select(SearchTrajectory &trajectory, float explorationConstant = 1.25f, int balanceDepth = -1);
			void expand(Node &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd);
			void backup(SearchTrajectory &trajectory, Value value, ProvenValue provenValue);
			void cancelVirtualLoss(SearchTrajectory &trajectory);

			void getPolicyPriors(const Node &node, matrix<float> &result) const;
			void getPlayoutDistribution(const Node &node, matrix<float> &result) const;
			void getProvenValues(const Node &node, matrix<ProvenValue> &result) const;
			void getActionValues(const Node &node, matrix<Value> &result) const;
			SearchTrajectory getPrincipalVariation();
			void printSubtree(const Node &node, int depth = -1, bool sort = false, int top_n = -1) const;
		private:
			Node* reserve_nodes(int number);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_TREE_HPP_ */
