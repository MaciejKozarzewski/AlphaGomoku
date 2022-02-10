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
#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/utils/ObjectPool.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <inttypes.h>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace ag
{
	class SearchTask;
	class SearchTrajectory_old;
} /* namespace ag */

namespace ag
{
	struct TreeStats
	{
			int64_t allocated_nodes = 0;
			int64_t used_nodes = 0;
			int64_t allocated_edges = 0;
			int64_t used_edges = 0;
			int max_depth = 0;
			int64_t node_proven_loss = 0;
			int64_t node_proven_draw = 0;
			int64_t node_proven_win = 0;
			int64_t edge_proven_loss = 0;
			int64_t edge_proven_draw = 0;
			int64_t edge_proven_win = 0;

			std::string toString() const;
			TreeStats& operator+=(const TreeStats &other) noexcept;
			TreeStats& operator/=(int i) noexcept;
	};

	enum class SelectOutcome
	{
		REACHED_LEAF,
		INFORMATION_LEAK
	};
	enum class ExpandOutcome
	{
		SUCCESS,
		ALREADY_EXPANDED
	};

	class TreeLock;
	class Tree
	{
		private:
			friend class TreeLock;

			mutable std::mutex tree_mutex;

			NodeCache node_cache;
			Node *root_node = nullptr;

			std::unique_ptr<EdgeSelector> edge_selector;
			std::unique_ptr<EdgeGenerator> edge_generator;
			matrix<Sign> base_board;
			int base_depth = 0;
			Sign sign_to_move = Sign::NONE;

			TreeConfig config;
			TreeStats stats;
		public:
			Tree(TreeConfig treeOptions);
			int64_t getMemory() const noexcept;

			void setBoard(const matrix<Sign> &newBoard, Sign signToMove);
			void setEdgeSelector(const EdgeSelector &selector);
			void setEdgeGenerator(const EdgeGenerator &generator);

			int getSimulationCount() const noexcept;
			int getMaximumDepth() const noexcept;
			bool isProven() const noexcept;
			bool hasSingleNonLosingMove() const noexcept;

			SelectOutcome select(SearchTask &task);
			void generateEdges(SearchTask &task) const;
			ExpandOutcome expand(SearchTask &task);
			void backup(const SearchTask &task);
			void cancelVirtualLoss(SearchTask &task) noexcept;
			void printSubtree(int depth = -1, bool sort = false, int top_n = -1) const;

			const matrix<Sign>& getBoard() const noexcept;
			Sign getSignToMove() const noexcept;

			Node getInfo(const std::vector<Move> &moves) const;
			void clearTreeStats() noexcept;
			TreeStats getTreeStats() const noexcept;
			void clearNodeCacheStats() noexcept;
			NodeCacheStats getNodeCacheStats() const noexcept;
		private:
			/*
			 * \brief Recursively checks every node in the subtree and deletes those representing states that can no longer appear in the tree (given current state at root).
			 */
			void prune_subtree(Node *node, matrix<Sign> &tmpBoard);
			/*
			 * \brief Recursively deletes all nodes in the subtree.
			 */
			void delete_subtree(Node *node, matrix<Sign> &tmpBoard);
			/*
			 * \brief Remove given node from the tree, freeing its edges and removing from cache.
			 */
			void remove_from_tree(Node *node, const matrix<Sign> &tmpBoard);
			/*
			 * \brief Calculates proper value if an information leak was found
			 */
			void correct_information_leak(SearchTask &task) const;
	};

	class TreeLock
	{
		private:
			const Tree &tree;
		public:
			TreeLock(const Tree &t) :
					tree(t)
			{
				t.tree_mutex.lock();
			}
			~TreeLock()
			{
				tree.tree_mutex.unlock();
			}
			TreeLock(const TreeLock &other) = delete;
			TreeLock(TreeLock &&other) = delete;
			TreeLock& operator=(const TreeLock &other) = delete;
			TreeLock& operator=(TreeLock &&other) = delete;
	};

	class Tree_old
	{
		private:
			std::vector<std::unique_ptr<Node_old[]>> nodes;
			std::pair<int, int> current_index { 0, 0 };
			mutable std::mutex tree_mutex;
			Node_old root_node;

			TreeConfig config;
			int balancing_depth = -1;
		public:
			Tree_old(TreeConfig treeOptions);

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
			void select(SearchTrajectory_old &trajectory, float explorationConstant = 1.25f);
			bool expand(Node_old &parent, const std::vector<std::pair<uint16_t, float>> &movesToAdd);
			void backup(SearchTrajectory_old &trajectory, Value value, ProvenValue provenValue);
			void cancelVirtualLoss(SearchTrajectory_old &trajectory);

			void getPolicyPriors(const Node_old &node, matrix<float> &result) const;
			void getPlayoutDistribution(const Node_old &node, matrix<float> &result) const;
			void getProvenValues(const Node_old &node, matrix<ProvenValue> &result) const;
			void getActionValues(const Node_old &node, matrix<Value> &result) const;
			SearchTrajectory_old getPrincipalVariation();
			void printSubtree(const Node_old &node, int depth = -1, bool sort = false, int top_n = -1) const;
		private:
			Node_old* reserve_nodes(int number);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_TREE_HPP_ */
