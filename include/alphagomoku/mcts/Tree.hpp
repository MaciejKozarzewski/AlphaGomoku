/*
 * Tree.hpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_TREE_HPP_
#define ALPHAGOMOKU_MCTS_TREE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/tss/SharedHashTable.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <cinttypes>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace ag
{
	class SearchTask;
	class Node;
} /* namespace ag */

namespace ag
{
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
			std::shared_ptr<tss::SharedHashTable<4>> shared_hash_table;
			Node *root_node = nullptr; // non-owning

			std::unique_ptr<EdgeSelector> edge_selector;
			std::unique_ptr<EdgeGenerator> edge_generator;
			matrix<Sign> base_board;
			int base_depth = 0;
			int max_depth = 0;
			Sign sign_to_move = Sign::NONE;

			TreeConfig tree_config;
		public:
			Tree(TreeConfig treeOptions);
			int64_t getMemory() const noexcept;

			void setBoard(const matrix<Sign> &newBoard, Sign signToMove, bool forceRemoveRootNode = false);
			void setEdgeSelector(const EdgeSelector &selector);
			void setEdgeGenerator(const EdgeGenerator &generator);

			int getNodeCount() const noexcept;
			int getSimulationCount() const noexcept;
			int getMaximumDepth() const noexcept;
			bool isProven() const noexcept;
			bool hasAllMovesProven() const noexcept;
			bool hasSingleMove() const noexcept;
			bool hasSingleNonLosingMove() const noexcept;

			SelectOutcome select(SearchTask &task);
			void generateEdges(SearchTask &task) const;
			ExpandOutcome expand(SearchTask &task);
			void backup(const SearchTask &task);
			void correctInformationLeak(const SearchTask &task);
			void cancelVirtualLoss(const SearchTask &task) noexcept;
			void printSubtree(int depth = -1, bool sort = false, int top_n = -1) const;

			std::shared_ptr<tss::SharedHashTable<4>> getSharedHashTable() noexcept;
			const matrix<Sign>& getBoard() const noexcept;
			Sign getSignToMove() const noexcept;

			Node getInfo(const std::vector<Move> &moves) const;
			void clearNodeCacheStats() noexcept;
			NodeCacheStats getNodeCacheStats() const noexcept;
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

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_TREE_HPP_ */
