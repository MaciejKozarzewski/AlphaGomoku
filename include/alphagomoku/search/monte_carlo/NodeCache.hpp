/*
 * NodeCache.hpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_NODECACHE_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_NODECACHE_HPP_

#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/ObjectPool.hpp>
#include <alphagomoku/utils/statistics.hpp>

#include <memory>

namespace ag
{
	struct NodeCacheStats
	{
			double load_factor = 0.0;

			int64_t allocated_nodes = 0;
			int64_t stored_nodes = 0;

			int64_t allocated_edges = 0;
			int64_t stored_edges = 0;

			TimedStat seek;
			TimedStat insert;
			TimedStat remove;
			TimedStat resize;
			TimedStat cleanup;

			NodeCacheStats();

			std::string toString() const;
			/**
			 * \brief Used to sum statistics over several instances of this class.
			 */
			NodeCacheStats& operator+=(const NodeCacheStats &other) noexcept;
			/**
			 * \brief Used to average statistics over several instances of this class.
			 */
			NodeCacheStats& operator/=(int i) noexcept;
	};

	class NodeCache
	{
		private:
			class CompressedBoard
			{
					std::array<uint64_t, 13> data; // assuming at most 416 spots on board
				public:
					CompressedBoard() = default;
					CompressedBoard(const matrix<Sign> &board) noexcept;
					bool operator==(const matrix<Sign> &board) const noexcept;
					bool isTransitionPossibleFrom(const CompressedBoard &board) const noexcept;
					int moveCount() const noexcept;
			};
			struct Entry
			{
					HashKey64 hash_key;
					Entry *next_entry = nullptr; // non-owning
					Node node;
					BlockDescriptor<Edge> edge_block;
					CompressedBoard board;
			};

			ObjectPool<Edge> edge_pool;
			std::vector<std::unique_ptr<Entry[]>> node_pool;

			std::vector<Entry*> bins; // non-owning
			Entry *buffer = nullptr; // non-owning
			FullZobristHashing hash_function;
			HashKey64 bin_index_mask = 0;
			size_t node_pool_bucket_size = 10000;

			int64_t buffered_nodes = 0;

			mutable NodeCacheStats stats;
		public:
			NodeCache() = default;
			NodeCache(GameConfig gameConfig, TreeConfig treeConfig);
			NodeCache(const NodeCache &other) = delete;
			NodeCache(NodeCache &&other);
			NodeCache& operator=(const NodeCache &other) = delete;
			NodeCache& operator =(NodeCache &&other);

			void clearStats() noexcept;
			NodeCacheStats getStats() const noexcept;

			uint64_t getMemory() const noexcept;
			int allocatedEdges() const noexcept;
			int storedEdges() const noexcept;
			int allocatedNodes() const noexcept;
			int storedNodes() const noexcept;
			int bufferedNodes() const noexcept;
			int numberOfBins() const noexcept;
			double loadFactor() const noexcept;

			/**
			 * \brief Clears the cache.
			 * All entries are moved to the temporary buffer to be used again.
			 */
			void clear() noexcept;
			/**
			 * \brief Removes entries that can no longer appear given the current board state.
			 * All entries are moved to the temporary buffer to be used again.
			 */
			void cleanup(const matrix<Sign> &newBoard, Sign signToMove) noexcept;
			/**
			 * \brief If given board is in the cache, returns pointer to node.
			 * If board is not in cache a null pointer is returned.
			 */
			Node* seek(const matrix<Sign> &board, Sign signToMove) const noexcept;
			/**
			 * \brief Inserts new entry to the cache.
			 * The inserted entry must not be in cache.
			 */
			Node* insert(const matrix<Sign> &board, Sign signToMove, int numberOfEdges);
			/**
			 * \brief Removes given board state from the cache, if it exists in the cache.
			 * If not, the function does nothing.
			 */
			void remove(const matrix<Sign> &board, Sign signToMove) noexcept;
			/**
			 * \brief Changes the number of bins to 2^newSize.
			 */
			void resize(size_t newSize);

		private:
			void allocate_more_entries();
			NodeCache::Entry* get_new_entry();
			void move_to_buffer(NodeCache::Entry *entry) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_NODECACHE_HPP_ */
