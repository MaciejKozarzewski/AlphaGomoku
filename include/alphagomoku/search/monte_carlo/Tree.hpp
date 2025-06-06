/*
 * Tree.hpp
 *
 *  Created on: Mar 2, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_TREE_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_TREE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/monte_carlo/NodeCache.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <cinttypes>
#include <memory>
#include <atomic>
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
		REACHED_PROVEN_EDGE,
		INFORMATION_LEAK
	};
	enum class ExpandOutcome
	{
		SUCCESS,
		ALREADY_EXPANDED,
		SKIPPED_EXPANSION
	};

	class TreeLock;
	class Tree
	{
		private:
			friend class TreeLock;

			mutable std::mutex tree_mutex;

			NodeCache node_cache;
			Node *root_node = nullptr; // non-owning

			std::unique_ptr<EdgeSelector> edge_selector;
			std::unique_ptr<EdgeGenerator> edge_generator;
			matrix<Sign> base_board;
			std::atomic<int> move_number { 0 };
			std::atomic<float> evaluation { 0.0f };
			std::atomic<float> moves_left { 0.0f };
			std::atomic<int> max_depth { 0 };
			Sign sign_to_move = Sign::NONE;

			TreeConfig config;
		public:
			Tree(const TreeConfig &treeConfig);
			int64_t getMemory() const noexcept;

			void clear();
			void setBoard(const matrix<Sign> &newBoard, Sign signToMove, bool forceRemoveRootNode = false);
			void setEdgeSelector(const EdgeSelector &selector);
			void setEdgeGenerator(const EdgeGenerator &generator);

			int getMoveNumber() const noexcept;
			float getEvaluation() const noexcept;
			float getMovesLeft() const noexcept;
			int getNodeCount() const noexcept;
			int getSimulationCount() const noexcept;
			int getMaximumDepth() const noexcept;
			bool isRootProven() const noexcept;
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

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_TREE_HPP_ */
