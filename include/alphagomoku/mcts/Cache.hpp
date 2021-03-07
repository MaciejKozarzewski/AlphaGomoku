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
#include <alphagomoku/utils/matrix.hpp>
#include <inttypes.h>
#include <memory>
#include <vector>
#include <mutex>

namespace ag
{
	class EvaluationRequest;
	class Node;
} /* namespace ag */

namespace ag
{

	class Cache
	{
		private:
			struct Entry
			{
					static const int max_number_of_transpositions = 10;

					Node *transpositions[max_number_of_transpositions];
					int stored_transpositions = 0;
					float value = 0.0f;
					std::unique_ptr<uint16_t[]> data;
					uint64_t hash = 0;
					Entry *next_entry = nullptr;

					Entry(int board_size);
					void copyTo(EvaluationRequest &request) const noexcept;
					void copyFrom(const EvaluationRequest &request, uint64_t board_hash) noexcept;
					void addTransposition(Node *node) noexcept;
					bool isPossible(const matrix<Sign> &new_board) const noexcept;
			};

			ZobristHashing hashing;
			std::vector<Entry*> bins;

			int allocated_entries = 0;
			int stored_entries = 0;
			int buffered_entries = 0;

			int board_size;
			Entry *buffer = nullptr;
			mutable std::mutex cache_mutex;
		public:

			Cache(int boardSize, int nb_of_bins);
			~Cache();
			uint64_t getMemory() const noexcept;
			int allocatedElements() const noexcept;
			int storedElements() const noexcept;
			int bufferedElements() const noexcept;
			int numberOfBins() const noexcept;
			double loadFactor() const noexcept;
			void clear() noexcept;
			bool seek(EvaluationRequest &request) const noexcept;
			void insert(const EvaluationRequest &request);
			void cleanup(const matrix<Sign> &new_board) noexcept;
			void remove(const matrix<Sign> &board, Sign signToMove) noexcept;
			void rehash(int new_nb_of_bins) noexcept;
		private:
			Cache::Entry* get_new_entry();
			void add_to_bin(Cache::Entry *entry, size_t bin_index) noexcept;
			void add_to_buffer(Cache::Entry *entry) noexcept;
	};

}

#endif /* ALPHAGOMOKU_MCTS_CACHE_HPP_ */
