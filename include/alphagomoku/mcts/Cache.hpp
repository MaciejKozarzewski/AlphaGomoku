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
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/configs.hpp>
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
					float value = 0.0f;
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

			int allocated_entries = 0;
			int stored_entries = 0;
			int buffered_entries = 0;

			GameConfig game_config;
			CacheConfig cache_config;
		public:

			Cache(GameConfig gameOptions, CacheConfig cacheOptions);
			Cache(const Cache &other) = delete;
			Cache& operator=(const Cache &other) = delete;
			Cache(Cache &&other) = delete;
			Cache& operator=(Cache &&other) = delete;
			~Cache();

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
