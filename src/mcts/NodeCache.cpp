/*
 * NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NodeCache.hpp>

namespace ag
{
	NodeCacheStats::NodeCacheStats() :
			contains("contains"),
			insert("insert"),
			remove("remove"),
			rehash("rehash")
	{
	}
	std::string NodeCacheStats::toString() const
	{
		std::string result = "----NodeCacheStats----\n";
		if (calls == 0)
			result += "hit rate = N/A\n";
		else
			result += "hit rate = " + std::to_string(static_cast<int>(1000.0 * static_cast<double>(hits) / calls) / 10.0) + "%\n";
		result += contains.toString() + '\n';
		result += insert.toString() + '\n';
		result += remove.toString() + '\n';
		result += rehash.toString() + '\n';
		result += "entries = " + std::to_string(stored_entries) + " : " + std::to_string(buffered_entries) + " : " + std::to_string(allocated_entries)
				+ " (stored:buffered:allocated)\n";
		return result;
	}
	NodeCacheStats& NodeCacheStats::operator+=(const NodeCacheStats &other) noexcept
	{
		this->hits += other.hits;
		this->calls += other.calls;
		this->allocated_entries += other.allocated_entries;
		this->stored_entries += other.stored_entries;
		this->buffered_entries += other.buffered_entries;

		this->contains += other.contains;
		this->insert += other.insert;
		this->remove += other.remove;
		this->rehash += other.rehash;
		return *this;
	}
	NodeCacheStats& NodeCacheStats::operator/=(int i) noexcept
	{
		this->hits /= i;
		this->calls /= i;
		this->allocated_entries /= i;
		this->stored_entries /= i;
		this->buffered_entries /= i;

		this->contains /= i;
		this->insert /= i;
		this->remove /= i;
		this->rehash /= i;
		return *this;
	}

	NodeCache::NodeCache(GameConfig gameOptions) :
			hashing(gameOptions)
	{
	}
	NodeCache::~NodeCache()
	{
		clear();
		while (buffer != nullptr)
		{
			Entry *next = buffer->next_entry;
			delete buffer;
			buffer = next;
		}
	}

	void NodeCache::clearStats() noexcept
	{
		stats.hits = stats.calls = 0;
		stats.contains.reset();
		stats.insert.reset();
		stats.remove.reset();
		stats.rehash.reset();
	}
	NodeCacheStats NodeCache::getStats() const noexcept
	{
		return stats;
	}

	uint64_t NodeCache::getMemory() const noexcept
	{
		return sizeof(Entry) * stats.allocated_entries + sizeof(Entry*) * bins.size();
	}
	int NodeCache::allocatedElements() const noexcept
	{
		return stats.allocated_entries;
	}
	int NodeCache::storedElements() const noexcept
	{
		return stats.stored_entries;
	}
	int NodeCache::bufferedElements() const noexcept
	{
		return stats.buffered_entries;
	}
	int NodeCache::numberOfBins() const noexcept
	{
		return bins.size();
	}
	double NodeCache::loadFactor() const noexcept
	{
		return static_cast<double>(stats.stored_entries) / bins.size();
	}

	/**
	 * @brief Clears the cache.
	 * All entries are moved to the temporary buffer to be used again.
	 */
	void NodeCache::clear() noexcept
	{
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
	/**
	 * @brief Checks if the given board state is in the cache.
	 */
	bool NodeCache::contains(const Board &board) const noexcept
	{
		TimerGuard timer(stats.contains);
	}
	/**
	 * @brief Adds new board state to the cache and returns its node.
	 */
	Node* NodeCache::insert(const Board &board) noexcept
	{
		TimerGuard timer(stats.insert);
	}
	/**
	 * @brief Removes given board state from the cache, if it exists in the cache.
	 */
	void NodeCache::remove(const Board &board) noexcept
	{
		TimerGuard timer(stats.remove);
	}
	/**
	 * @brief Changes the number of bins.
	 */
	void NodeCache::rehash(int newNumberOfBins)
	{
		TimerGuard timer(stats.rehash);
	}
	/**
	 * @brief Deallocates all buffered entries.
	 */
	void NodeCache::freeUnusedMemory()
	{
	}

	NodeCache::Entry* NodeCache::get_new_entry()
	{
		if (buffer == nullptr)
		{
			stats.allocated_entries++;
			return new Entry();
		}
		else
		{
			assert(stats.buffered_entries > 0);
			stats.buffered_entries--;
			Entry *result = buffer;
			buffer = result->next_entry;
			result->next_entry = nullptr;
			return result;
		}
	}
	void NodeCache::add_to_buffer(NodeCache::Entry *entry) noexcept
	{
		entry->next_entry = buffer;
		buffer = entry;
		assert(stats.stored_entries > 0);
		stats.stored_entries--;
		stats.buffered_entries++;
	}
} /* namespace ag */

