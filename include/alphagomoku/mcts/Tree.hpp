/*
 * Tree.hpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_TREE_HPP_
#define ALPHAGOMOKU_MCTS_TREE_HPP_

#include <alphagomoku/game/Move.hpp>
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

	class TreeLock
	{
		private:
			std::mutex& tree_mutex;
		public:
			TreeLock(std::mutex &t) :
					tree_mutex(t)
			{
			}
			TreeLock(TreeLock &&other) = default;
			TreeLock& operator=(TreeLock &&other) = default;
			~TreeLock()
			{
				tree_mutex.unlock();
			}
			TreeLock(const TreeLock &other) = delete;
			TreeLock& operator=(const TreeLock &other) = delete;
	};

	class Tree
	{
		private:
			std::vector<std::unique_ptr<Node_old[]>> nodes;
			std::pair<int, int> current_index { 0, 0 };
			mutable std::mutex tree_mutex;
			Node_old root_node;

			TreeConfig config;
			int balancing_depth = -1;
		public:
			Tree(TreeConfig treeOptions);
			[[nodiscard]] TreeLock lock() const noexcept;

			uint64_t getMemory() const noexcept;
			void clearStats() noexcept;
			TreeStats getStats() const noexcept;

			void clear() noexcept;
			int allocatedNodes() const noexcept;
			int usedNodes() const noexcept;

			bool isProven() const noexcept;
			const Node_old& getRootNode() const noexcept;
			Node_old& getRootNode() noexcept;
			void setBalancingDepth(int depth) noexcept;
			bool isRootNode(const Node_old *node) const noexcept;
			void select(SearchTrajectory &trajectory, float explorationConstant = 1.25f);
			bool expand(Node_old &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd);
			void backup(SearchTrajectory &trajectory, Value value, ProvenValue provenValue);
			void cancelVirtualLoss(SearchTrajectory &trajectory);

			void getPolicyPriors(const Node_old &node, matrix<float> &result) const;
			void getPlayoutDistribution(const Node_old &node, matrix<float> &result) const;
			void getProvenValues(const Node_old &node, matrix<ProvenValue> &result) const;
			void getActionValues(const Node_old &node, matrix<Value> &result) const;
			SearchTrajectory getPrincipalVariation();
			void printSubtree(const Node_old &node, int depth = -1, bool sort = false, int top_n = -1) const;
		private:
			Node_old* reserve_nodes(int number);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_TREE_HPP_ */
