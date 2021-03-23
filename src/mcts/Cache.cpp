/*
 * Cache.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>

#include <math.h>
#include <iostream>

namespace ag
{
	Cache::Entry::Entry(int boardSize) :
			data(std::make_unique<uint16_t[]>(boardSize))
	{
		clearTranspositions();
	}
	void Cache::Entry::copyTo(EvaluationRequest &request) const noexcept
	{
		const float tmp = 1.0f / 16383;
		request.setReady();
		request.setValue(value);
		request.setProvenValue(proven_value);
		for (int i = 0; i < request.getPolicy().size(); i++)
			request.getPolicy().data()[i] = (data[i] >> 2) * tmp;
	}
	void Cache::Entry::copyFrom(const EvaluationRequest &request, uint64_t boardHash) noexcept
	{
		clearTranspositions();
		addTransposition(request.getNode());
		hash = boardHash;
		proven_value = ProvenValue::UNKNOWN;
		value = request.getValue();
		for (int i = 0; i < request.getPolicy().size(); i++)
			data[i] = static_cast<int>(request.getBoard().data()[i]) | (static_cast<int>(request.getPolicy().data()[i] * 16383.0f) << 2);
	}
	void Cache::Entry::addTransposition(Node *node) noexcept
	{
		if (stored_transpositions >= max_number_of_transpositions || node == nullptr)
			return;
		for (int i = 0; i < stored_transpositions; i++)
			if (transpositions[i] == node)
				return;
		transpositions[stored_transpositions] = node;
		stored_transpositions++;
		if (proven_value == ProvenValue::UNKNOWN)
			proven_value = node->getProvenValue(); // TODO potentially unsafe (lack of synchronization with the node)
	}
	bool Cache::Entry::isPossible(const matrix<Sign> &newBoard) const noexcept
	{
		for (int i = 0; i < newBoard.size(); i++)
			if (static_cast<Sign>(data[i] & 3) != newBoard.data()[i] && newBoard.data()[i] != Sign::NONE)
				return false;
		return true;
	}
	void Cache::Entry::update(int visitTreshold, matrix<int> &workspace)
	{
		if (stored_transpositions == 0)
			return;

		int total_visits = 0;
		double total_wins = 0.0;
		for (int i = 0; i < stored_transpositions; i++)
		{
			assert(transpositions[i] != nullptr);
			total_visits += transpositions[i]->getVisits();
			total_wins += static_cast<double>(transpositions[i]->getValue()) * transpositions[i]->getVisits();
			if (proven_value == ProvenValue::UNKNOWN)
				proven_value = transpositions[i]->getProvenValue();
		}
		this->value = total_wins / total_visits;

		if (total_visits >= visitTreshold)
		{
			total_visits = 0;
			workspace.clear();
			for (int i = 0; i < stored_transpositions; i++)
				for (int j = 0; j < transpositions[i]->numberOfChildren(); j++)
				{
					uint16_t move = transpositions[i]->getChild(j).getMove();
					workspace.at(Move::getRow(move), Move::getCol(move)) += transpositions[i]->getChild(j).getVisits();
					total_visits += transpositions[i]->getChild(j).getVisits();
				}

			const float weight = (total_visits > 1024 * visitTreshold) ? 0.0f : std::pow(0.5f, static_cast<float>(total_visits) / visitTreshold);
			const float scale = 16383.0f / total_visits;
			for (int i = 0; i < workspace.size(); i++)
			{
				float new_policy = weight * (data[i] >> 2) + (1.0f - weight) * (workspace.data()[i] * scale);
				data[i] = static_cast<int>(data[i] & 3) | (static_cast<int>(new_policy) << 2);
			}
		}
	}
	void Cache::Entry::clearTranspositions() noexcept
	{
		stored_transpositions = 0;
		std::memset(transpositions, 0, sizeof(transpositions));
	}

	Cache::Cache(GameConfig gameOptions, CacheConfig cacheOptions) :
			hashing(gameOptions.rows * gameOptions.cols),
			bins(cacheOptions.min_cache_size, nullptr),
			game_config(gameOptions),
			cache_config(cacheOptions)
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
		return (sizeof(Entry) + sizeof(uint16_t) * game_config.rows * game_config.cols) * allocated_entries + sizeof(Entry*) * bins.size();
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
	int Cache::transpositionCount() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		int result = 0;
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			while (current != nullptr)
			{
				result += current->stored_transpositions;
				current = current->next_entry;
			}
		}
		return result;
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
		assert(stored_entries + buffered_entries == allocated_entries);
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
	void Cache::cleanup(const matrix<Sign> &newBoard) noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		if (stored_entries == 0)
			return;

		matrix<int> workspace(game_config.rows, game_config.cols);
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			bins[i] = nullptr;
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				if (current->isPossible(newBoard))
				{
					if (cache_config.update_from_search)
						current->update(cache_config.update_visit_treshold, workspace);
					current->clearTranspositions();
					current->next_entry = bins[i];
					bins[i] = current;
				}
				else
					add_to_buffer(current);
				current = next;
			}
		}
		assert(stored_entries + buffered_entries == allocated_entries);
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
		assert(stored_entries + buffered_entries == allocated_entries);
	}
	void Cache::rehash(int newNumberOfBins) noexcept
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
		bins.resize(newNumberOfBins, nullptr);

		while (tmp != nullptr)
		{
			Entry *next = tmp->next_entry;
			size_t bin_index = tmp->hash % bins.size();
			tmp->next_entry = bins[bin_index];
			bins[bin_index] = tmp;
			tmp = next;
		}
		assert(stored_entries + buffered_entries == allocated_entries);
	}
// private
	Cache::Entry* Cache::get_new_entry()
	{
		if (buffer == nullptr)
		{
			allocated_entries++;
			return new Entry(game_config.rows * game_config.cols);
		}
		else
		{
			buffered_entries--;
			Entry *result = buffer;
			buffer = result->next_entry;
			result->stored_transpositions = 0;
			result->next_entry = nullptr;

			assert(buffered_entries >= 0);
			return result;
		}
	}
	void Cache::add_to_buffer(Cache::Entry *entry) noexcept
	{
		entry->next_entry = buffer;
		buffer = entry;
		stored_entries--;
		buffered_entries++;
		assert(stored_entries >= 0);
	}
}

