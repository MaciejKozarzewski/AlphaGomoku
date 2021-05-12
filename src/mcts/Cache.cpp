/*
 * Cache.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <math.h>
#include <iostream>

namespace ag
{
	std::string CacheStats::toString() const
	{
		std::string result = "----CacheStats----\n";
		if (calls == 0)
			result += "hit rate = 0%\n";
		else
			result += "hit rate = " + std::to_string(static_cast<int>(1000.0 * static_cast<double>(hits) / calls) / 10.0) + "%\n";
		result += printStatistics("time_seek   ", nb_seek, time_seek);
		result += printStatistics("time_insert ", nb_insert, time_insert);
		result += printStatistics("time_cleanup", nb_cleanup, time_cleanup);
		result += "entries = " + std::to_string(stored_entries) + " : " + std::to_string(buffered_entries) + " : " + std::to_string(allocated_entries)
				+ " (stored:buffered:allocated)\n";
		return result;
	}
	CacheStats& CacheStats::operator+=(const CacheStats &other) noexcept
	{
		this->hits += other.hits;
		this->calls += other.calls;
		this->allocated_entries += other.allocated_entries;
		this->stored_entries += other.stored_entries;
		this->buffered_entries += other.buffered_entries;

		this->nb_seek += other.nb_seek;
		this->nb_insert += other.nb_insert;
		this->nb_cleanup += other.nb_cleanup;

		this->time_seek += other.time_seek;
		this->time_insert += other.time_insert;
		this->time_cleanup += other.time_cleanup;
		return *this;
	}
	CacheStats& CacheStats::operator/=(int i) noexcept
	{
		this->hits /= i;
		this->calls /= i;
		this->allocated_entries /= i;
		this->stored_entries /= i;
		this->buffered_entries /= i;

		this->nb_seek /= i;
		this->nb_insert /= i;
		this->nb_cleanup /= i;

		this->time_seek /= i;
		this->time_insert /= i;
		this->time_cleanup /= i;
		return *this;
	}

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
		{
			assert(request.getBoard().data()[i] == static_cast<Sign>(data[i] & 3)); // check for board correctness
			request.getPolicy().data()[i] = (data[i] >> 2) * tmp;
		}
	}
	void Cache::Entry::copyFrom(const EvaluationRequest &request, uint64_t boardHash) noexcept
	{
		hash = boardHash;
		proven_value = ProvenValue::UNKNOWN;
		value = request.getValue();
		for (int i = 0; i < request.getPolicy().size(); i++)
			data[i] = static_cast<int>(request.getBoard().data()[i]) | (static_cast<int>(request.getPolicy().data()[i] * 16383.0f) << 2);
		clearTranspositions();
		addTransposition(request.getNode());
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
		//if (proven_value == ProvenValue::UNKNOWN)
		//	proven_value = node->getProvenValue(); // TODO potentially unsafe (lack of synchronization with the node)
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
		double total_wins = 0.0, total_draws = 0.0, total_losses = 0.0;
		for (int i = 0; i < stored_transpositions; i++)
		{
			assert(transpositions[i] != nullptr);
			total_visits += transpositions[i]->getVisits();
			total_wins += static_cast<double>(transpositions[i]->getValue().win) * transpositions[i]->getVisits();
			total_draws += static_cast<double>(transpositions[i]->getValue().draw) * transpositions[i]->getVisits();
			total_losses += static_cast<double>(transpositions[i]->getValue().loss) * transpositions[i]->getVisits();
			if (proven_value == ProvenValue::UNKNOWN)
				proven_value = transpositions[i]->getProvenValue();
		}
		assert(total_visits > 0);
		this->value = Value(total_wins / total_visits, total_draws / total_visits, total_losses / total_visits);

		if (total_visits >= visitTreshold and visitTreshold != -1)
		{
			total_visits = 0;
			workspace.clear();
			for (int i = 0; i < stored_transpositions; i++)
				for (auto iter = transpositions[i]->begin(); iter < transpositions[i]->end(); iter++)
				{
					uint16_t move = iter->getMove();
					workspace.at(Move::getRow(move), Move::getCol(move)) += iter->getVisits();
					total_visits += iter->getVisits();
				}

			const float scale = 16383.0f / total_visits;
			for (int i = 0; i < workspace.size(); i++)
				data[i] = static_cast<int>(data[i] & 3) | (static_cast<int>(workspace.data()[i] * scale) << 2);
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
	void Cache::clearStats() noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		stats.hits = stats.calls = 0;
		stats.nb_seek = stats.nb_insert = stats.nb_cleanup = 0;
		stats.time_seek = stats.time_insert = stats.time_cleanup = 0.0;
	}
	CacheStats Cache::getStats() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return stats;
	}
	uint64_t Cache::getMemory() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return (sizeof(Entry) + sizeof(uint16_t) * game_config.rows * game_config.cols) * stats.stored_entries + sizeof(Entry*) * bins.size();
//		return (sizeof(Entry) + sizeof(uint16_t) * game_config.rows * game_config.cols) * stats.allocated_entries + sizeof(Entry*) * bins.size();
	}
	int Cache::allocatedElements() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return stats.allocated_entries;
	}
	int Cache::storedElements() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return stats.stored_entries;
	}
	int Cache::bufferedElements() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return stats.buffered_entries;
	}
	int Cache::numberOfBins() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return bins.size();
	}
	double Cache::loadFactor() const noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		return static_cast<double>(stats.stored_entries) / bins.size();
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
		assert(stats.stored_entries + stats.buffered_entries == stats.allocated_entries);
	}
	bool Cache::seek(EvaluationRequest &request) const noexcept
	{
		double start = getTime(); // statistics
		uint64_t hash = hashing.getHash(request.getBoard(), request.getSignToMove());
		std::lock_guard<std::mutex> lock(cache_mutex);
		stats.calls++; // statistics
		stats.nb_seek++; // statistics

		Entry *current = bins[hash % bins.size()];
		while (current != nullptr)
		{
			if (current->hash == hash)
			{
				current->copyTo(request);
				current->addTransposition(request.getNode());
				stats.hits++; // statistics
				stats.time_seek += getTime() - start; // statistics
				return true;
			}
			current = current->next_entry;
		}
		stats.time_seek += getTime() - start; // statistics
		return false;
	}
	void Cache::insert(const EvaluationRequest &request)
	{
		double start = getTime(); // statistics
		uint64_t hash = hashing.getHash(request.getBoard(), request.getSignToMove());
		std::lock_guard<std::mutex> lock(cache_mutex);
		stats.nb_insert++; // statistics

		Entry *entry = get_new_entry();
		entry->hash = hash;
		entry->copyFrom(request, hash);
		size_t bin_index = hash % bins.size();

		entry->next_entry = bins[bin_index];
		bins[bin_index] = entry;
		stats.stored_entries++;
		stats.time_insert += getTime() - start;
		assert(stats.stored_entries + stats.buffered_entries == stats.allocated_entries);
	}
	void Cache::cleanup(const matrix<Sign> &newBoard) noexcept
	{
		double start = getTime(); // statistics
		std::lock_guard<std::mutex> lock(cache_mutex);
		stats.nb_cleanup++; // statistics
		if (stats.stored_entries == 0)
		{
			stats.time_cleanup += getTime() - start; // statistics
			return;
		}

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
		stats.time_cleanup += getTime() - start; // statistics
		assert(stats.stored_entries + stats.buffered_entries == stats.allocated_entries);
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
		assert(stats.stored_entries + stats.buffered_entries == stats.allocated_entries);
	}
	void Cache::rehash(int newNumberOfBins) noexcept
	{
		std::lock_guard<std::mutex> lock(cache_mutex);
		if (stats.stored_entries == 0 or static_cast<int>(bins.size()) == newNumberOfBins)
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
		assert(stats.stored_entries + stats.buffered_entries == stats.allocated_entries);
	}
// private
	Cache::Entry* Cache::get_new_entry()
	{
		if (buffer == nullptr)
		{
			stats.allocated_entries++;
			return new Entry(game_config.rows * game_config.cols);
		}
		else
		{
			assert(stats.buffered_entries > 0);
			stats.buffered_entries--;
			Entry *result = buffer;
			buffer = result->next_entry;
			result->stored_transpositions = 0;
			result->next_entry = nullptr;
			return result;
		}
	}
	void Cache::add_to_buffer(Cache::Entry *entry) noexcept
	{
		entry->next_entry = buffer;
		buffer = entry;
		assert(stats.stored_entries > 0);
		stats.stored_entries--;
		stats.buffered_entries++;
	}
}

