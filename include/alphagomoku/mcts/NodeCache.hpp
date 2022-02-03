/*
 * NodeCache.hpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_NODECACHE_HPP_
#define ALPHAGOMOKU_MCTS_NODECACHE_HPP_

#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/statistics.hpp>

namespace ag
{
	struct NodeCacheStats
	{
			uint64_t hits = 0;
			uint64_t calls = 0;
			uint64_t collisions = 0;

			TimedStat seek;
			TimedStat insert;
			TimedStat remove;
			TimedStat resize;

			NodeCacheStats();

			std::string toString() const;
			/**
			 * @brief Used to sum statistics over several instances of this class.
			 */
			NodeCacheStats& operator+=(const NodeCacheStats &other) noexcept;
			/**
			 * @brief Used to average statistics over several instances of this class.
			 */
			NodeCacheStats& operator/=(int i) noexcept;
	};

	class NodeCache
	{
		private:
			struct Entry
			{
					Node node;
					uint64_t hash = 0;
					Entry *next_entry = nullptr; // non-owning
			};

			std::vector<Entry*> bins; // non-owning
			Entry *buffer = nullptr; // non-owning
			ZobristHashing hashing;
			uint64_t bin_index_mask;
			int64_t allocated_entries = 0;
			int64_t stored_entries = 0;
			int64_t buffered_entries = 0;

			mutable NodeCacheStats stats;
		public:
			/**
			 * @brief Creates cache with 2^size initial bins.
			 */
			NodeCache(GameConfig gameConfig, size_t size = 10);
			~NodeCache();

			void clearStats() noexcept;
			NodeCacheStats getStats() const noexcept;

			uint64_t getMemory() const noexcept;
			int allocatedElements() const noexcept;
			int storedElements() const noexcept;
			int bufferedElements() const noexcept;
			int numberOfBins() const noexcept;
			double loadFactor() const noexcept;

			/**
			 * @brief Ensures that at least n entries are allocated and ready to use.
			 */
			void reserve(size_t n);
			/**
			 * @brief Clears the cache.
			 * All entries are moved to the temporary buffer to be used again.
			 */
			void clear() noexcept;
			/**
			 * @brief If given board is in the cache, returns pointer to node.
			 * If board is not in cache a null pointer is returned.
			 */
			Node* seek(const matrix<Sign> &board, Sign signToMove) const noexcept;
			/**
			 * @brief Inserts new entry to the cache.
			 * The inserted entry must not be in cache.
			 */
			Node* insert(const matrix<Sign> &board, Sign signToMove) noexcept;
			/**
			 * @brief Removes given board state from the cache, if it exists in the cache.
			 * If not, the function does nothing.
			 */
			void remove(const matrix<Sign> &board, Sign signToMove) noexcept;
			/**
			 * @brief Changes the number of bins to 2^newSize.
			 */
			void resize(size_t newSize);
			/**
			 * @brief Deallocates all buffered entries.
			 */
			void freeUnusedMemory() noexcept;

			NodeCache(const NodeCache &other) = delete;
			NodeCache& operator=(const NodeCache &other) = delete;
			NodeCache(NodeCache &&other) = delete;
			NodeCache& operator=(NodeCache &&other) = delete;

		private:
			void link(Entry *&prev, Entry *next) noexcept;
			NodeCache::Entry* unlink(Entry *&prev) noexcept;
			NodeCache::Entry* get_new_entry();
			void move_to_buffer(NodeCache::Entry *entry) noexcept;
			bool is_hash_collision(const Entry *entry, const matrix<Sign> &board, Sign signToMove) const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NODECACHE_HPP_ */
