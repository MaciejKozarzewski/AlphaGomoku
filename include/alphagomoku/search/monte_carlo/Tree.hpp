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
#include <alphagomoku/utils/PriorityMutex.hpp>

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
			mutable PriorityMutex tree_mutex;

			NodeCache node_cache;
			Node *root_node = nullptr; // non-owning

			std::unique_ptr<EdgeSelector> edge_selector;
			std::unique_ptr<EdgeGenerator> edge_generator;
			matrix<Sign> base_board;
			int move_number = 0;
			float evaluation = 0.0f;
			float moves_left = 0.0f;
			int max_depth = 0;
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

			LowPriorityLock low_priority_lock() const;
			HighPriorityLock high_priority_lock() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_TREE_HPP_ */
