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
			uint64_t allocated_entries = 0;
			uint64_t stored_entries = 0;
			uint64_t buffered_entries = 0;

			TimedStat contains;
			TimedStat insert;
			TimedStat remove;
			TimedStat rehash;

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
				private:
					Entry *next_entry = nullptr; // non-owning
				public:
					Node node;
					uint64_t hash = 0;

					/**
					 * @brief Creates a link between entries.
					 *
					 *
					 * @param prev reference to a pointer that
					 */
					void link(Entry *&prev) noexcept;
					Entry* unlink(Entry *&prev) noexcept;
			};

			ZobristHashing hashing;
			std::vector<Entry*> bins; // non-owning
			Entry *buffer = nullptr; // non-owning

			mutable NodeCacheStats stats;
		public:
			NodeCache(GameConfig gameOptions);
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
			 * @brief Clears the cache.
			 * All entries are moved to the temporary buffer to be used again.
			 */
			void clear() noexcept;
			/**
			 * @brief Checks if the given board state is in the cache.
			 */
			bool contains(const Board &board) const noexcept;
			/**
			 * @brief Adds new board state to the cache and returns its node.
			 */
			Node* insert(const Board &board) noexcept;
			/**
			 * @brief Removes given board state from the cache, if it exists in the cache.
			 */
			void remove(const Board &board) noexcept;
			/**
			 * @brief Changes the number of bins.
			 */
			void rehash(int newNumberOfBins);
			/**
			 * @brief Deallocates all buffered entries.
			 */
			void freeUnusedMemory();

			NodeCache(const NodeCache &other) = delete;
			NodeCache& operator=(const NodeCache &other) = delete;
			NodeCache(NodeCache &&other) = delete;
			NodeCache& operator=(NodeCache &&other) = delete;

		private:
			NodeCache::Entry* get_new_entry();
			void add_to_buffer(NodeCache::Entry *entry) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_NODECACHE_HPP_ */
