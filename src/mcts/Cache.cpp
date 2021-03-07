/*
 * Cache.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>

namespace ag
{
	Cache::Entry::Entry(int board_size) :
			data(std::make_unique<uint16_t[]>(board_size))
	{
	}
	void Cache::Entry::copyTo(EvaluationRequest &request) const noexcept
	{
		static const float tmp = 1.0f / 16383;
		request.setValue(this->value);
		for (int i = 0; i < request.getPolicy().size(); i++)
			request.getPolicy().data()[i] = (data[i] >> 2) * tmp;
	}
	void Cache::Entry::copyFrom(const EvaluationRequest &request, uint64_t board_hash) noexcept
	{
		this->hash = board_hash;
		this->value = request.getValue();
		for (int i = 0; i < request.getPolicy().size(); i++)
			data[i] = static_cast<int>(request.getBoard().data()[i]) + (static_cast<int>(request.getPolicy().data()[i] * 16383.0f) << 2);
	}
	void Cache::Entry::addTransposition(Node *node) noexcept
	{
		if (stored_transpositions < max_number_of_transpositions)
		{
			bool is_already_stored = false;
			for (int i = 0; i < stored_transpositions; i++)
				if (transpositions[i] == node)
				{
					is_already_stored = true;
					break;
				}
			if (is_already_stored == false)
			{
				transpositions[stored_transpositions] = node;
				stored_transpositions++;
			}
		}
	}
	bool Cache::Entry::isPossible(const matrix<Sign> &new_board) const noexcept
	{
		const int length = new_board.size();
		for (int i = 0; i < length; i++)
			if (static_cast<Sign>(data[i] & 3) != new_board.data()[i] && new_board.data()[i] != Sign::NONE)
				return false;
		return true;
	}

	Cache::Cache(int boardSize, int nb_of_bins) :
			hashing(boardSize),
			bins(nb_of_bins, nullptr),
			board_size(boardSize)
	{
	}
	Cache::~Cache()
	{
		clear();
		std::lock_guard<std::mutex> lock(cache_mutex);
		while (buffer != nullptr)
		{
			Entry *next = buffer->next_entry;
			delete buffer;
			buffer = next;
		}
	}
	uint64_t Cache::getMemory() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return (sizeof(Entry) + sizeof(uint16_t) * board_size) * allocated_entries + sizeof(Entry*) * bins.size();
	}
	int Cache::allocatedElements() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return allocated_entries;
	}
	int Cache::storedElements() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return stored_entries;
	}
	int Cache::bufferedElements() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return buffered_entries;
	}
	int Cache::numberOfBins() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return bins.size();
	}
	double Cache::loadFactor() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return static_cast<double>(stored_entries) / bins.size();
	}
	void Cache::clear() noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				add_to_buffer(current);
				current = next;
			}
			bins[i] = nullptr;
		}
	}
	bool Cache::seek(EvaluationRequest &request) const noexcept
	{
		uint64_t hash = hashing.getHash(request.getBoard(), request.getSignToMove());
		std::lock_guard<std::mutex> lock(cache_mutex);

		Entry *current = bins[hash % bins.size()];
		while (current != nullptr)
		{
			if (current->hash == hash)
			{
				current->copyTo(request);
				current->addTransposition(request.getNode());
				request.setReady();
				return true;
			}
			current = current->next_entry;
		}
		return false;
	}
	void Cache::insert(const EvaluationRequest &request)
	{
		uint64_t hash = hashing.getHash(request.getBoard(), request.getSignToMove());
		std::lock_guard<std::mutex> lock(cache_mutex);

		Entry *entry = get_new_entry();
		entry->hash = hash;
		entry->copyFrom(request, hash);
		size_t bin_index = hash % bins.size();

		entry->next_entry = bins[bin_index];
		bins[bin_index] = entry;
		stored_entries++;
		assert(stored_entries + buffered_entries == allocated_entries);
	}
	void Cache::cleanup(const matrix<Sign> &new_board) noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		if (stored_entries == 0)
			return;

		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			bins[i] = nullptr;
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				if (current->isPossible(new_board))
				{
					current->next_entry = bins[i];
					bins[i] = current;
				}
				else
					add_to_buffer(current);
				current = next;
			}
		}
	}
	void Cache::remove(const matrix<Sign> &board, Sign signToMove) noexcept
	{
		uint64_t hash = hashing.getHash(board, signToMove);
		std::lock_guard<std::mutex> lock(cache_mutex);

		size_t bin_index = hash % bins.size();
		Entry *current = bins[bin_index];
		bins[bin_index] = nullptr;
		while (current != nullptr)
		{
			Entry *next = current->next_entry;
			if (current->hash == hash)
				add_to_buffer(current);
			else
			{
				current->next_entry = bins[bin_index];
				bins[bin_index] = current;
			}
			current = next;
		}
	}
	void Cache::rehash(int new_nb_of_bins) noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		if (stored_entries == 0)
			return;

		Entry *tmp = nullptr;
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			bins[i] = nullptr;
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				current->next_entry = tmp;
				tmp = current;
				current = next;
			}
		}
		bins.resize(new_nb_of_bins, nullptr);

		while (tmp != nullptr)
		{
			Entry *next = tmp->next_entry;
			size_t bin_index = tmp->hash % bins.size();
			tmp->next_entry = bins[bin_index];
			bins[bin_index] = tmp;
			tmp = next;
		}
	}
// private
	Cache::Entry* Cache::get_new_entry()
	{
		if (buffer == nullptr)
		{
			allocated_entries++;
			return new Entry(board_size);
		}
		else
		{
			buffered_entries--;
			Entry *result = buffer;
			buffer = result->next_entry;
			result->stored_transpositions = 0;
			result->next_entry = nullptr;

			assert(buffered_entries >= 0);
			assert(stored_entries + buffered_entries == allocated_entries);
			return result;
		}
	}
	void Cache::add_to_bin(Cache::Entry *entry, size_t bin_index) noexcept
	{
		entry->next_entry = bins[bin_index];
		bins[bin_index] = entry;
	}
	void Cache::add_to_buffer(Cache::Entry *entry) noexcept
	{
		entry->next_entry = buffer;
		buffer = entry;
		stored_entries--;
		buffered_entries++;

		assert(stored_entries >= 0);
		assert(stored_entries + buffered_entries == allocated_entries);
	}
}

