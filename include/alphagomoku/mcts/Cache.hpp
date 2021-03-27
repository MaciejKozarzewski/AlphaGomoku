/*
 * Cache.hpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_MCTS_CACHE_HPP_
#define ALPHAGOMOKU_MCTS_CACHE_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/ZobristHashing.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <inttypes.h>
#include <memory>
#include <vector>
#include <mutex>

namespace ag
{
	class EvaluationRequest;
} /* namespace ag */

namespace ag
{
	struct CacheStats
	{
			uint64_t hits = 0;
			uint64_t calls = 0;
			uint64_t allocated_entries = 0;
			uint64_t stored_entries = 0;
			uint64_t buffered_entries = 0;

			uint64_t nb_seek = 0;
			uint64_t nb_insert = 0;
			uint64_t nb_cleanup = 0;

			double time_seek = 0.0;
			double time_insert = 0.0;
			double time_cleanup = 0.0;

			std::string toString() const;
			CacheStats& operator+=(const CacheStats &other) noexcept;
			CacheStats& operator/=(int i) noexcept;
	};

	class Cache
	{
		private:
			struct Entry
			{
					static const int max_number_of_transpositions = 10;

					Node *transpositions[max_number_of_transpositions]; // non-owning
					Entry *next_entry = nullptr; // non-owning
					std::unique_ptr<uint16_t[]> data;
					uint64_t hash = 0;
					Value value;
					ProvenValue proven_value = ProvenValue::UNKNOWN;
					int stored_transpositions = 0;

					Entry(int boardSize);
					void copyTo(EvaluationRequest &request) const noexcept;
					void copyFrom(const EvaluationRequest &request, uint64_t boardHash) noexcept;
					void addTransposition(Node *node) noexcept;
					bool isPossible(const matrix<Sign> &newBoard) const noexcept;
					void update(int visitTreshold, matrix<int> &workspace);
					void clearTranspositions() noexcept;
			};

			ZobristHashing hashing;
			std::vector<Entry*> bins; // non-owning
			mutable std::mutex cache_mutex;
			Entry *buffer = nullptr; // non-owning

			mutable CacheStats stats;

			GameConfig game_config;
			CacheConfig cache_config;
		public:

			Cache(GameConfig gameOptions, CacheConfig cacheOptions);
			Cache(const Cache &other) = delete;
			Cache& operator=(const Cache &other) = delete;
			Cache(Cache &&other) = delete;
			Cache& operator=(Cache &&other) = delete;
			~Cache();

			void clearStats() noexcept;
			CacheStats getStats() const noexcept;

			uint64_t getMemory() const noexcept;
			int allocatedElements() const noexcept;
			int storedElements() const noexcept;
			int bufferedElements() const noexcept;
			int numberOfBins() const noexcept;
			double loadFactor() const noexcept;
			int transpositionCount() const noexcept;
			void clear() noexcept;

			bool seek(EvaluationRequest &request) const noexcept;
			void insert(const EvaluationRequest &request);
			void cleanup(const matrix<Sign> &newBoard) noexcept;
			void remove(const matrix<Sign> &board, Sign signToMove) noexcept;
			void rehash(int newNumberOfBins) noexcept;
		private:
			Cache::Entry* get_new_entry();
			void add_to_buffer(Cache::Entry *entry) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_CACHE_HPP_ */
