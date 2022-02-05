/*
 * NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>

namespace ag
{
	NodeCacheStats::NodeCacheStats() :
			seek("seek    "),
			insert("insert  "),
			remove("remove  "),
			resize("resize  ")
	{
	}
	std::string NodeCacheStats::toString() const
	{
		std::string result = "----NodeCacheStats----\n";
		if (calls == 0)
			result += "hit rate = N/A\n";
		else
			result += "hit rate = " + std::to_string(static_cast<int>(1000.0 * static_cast<double>(hits) / calls) / 10.0) + "%\n";
		result += seek.toString() + '\n';
		result += insert.toString() + '\n';
		result += remove.toString() + '\n';
		result += resize.toString() + '\n';
//		result += "entries = " + std::to_string(stored_entries) + " : " + std::to_string(buffered_entries) + " : " + std::to_string(allocated_entries)
//				+ " (stored:buffered:allocated)\n";
		return result;
	}
	NodeCacheStats& NodeCacheStats::operator+=(const NodeCacheStats &other) noexcept
	{
		this->hits += other.hits;
		this->calls += other.calls;
		this->collisions += other.collisions;

		this->seek += other.seek;
		this->insert += other.insert;
		this->remove += other.remove;
		this->resize += other.resize;
		return *this;
	}
	NodeCacheStats& NodeCacheStats::operator/=(int i) noexcept
	{
		this->hits /= i;
		this->calls /= i;
		this->collisions /= i;

		this->seek /= i;
		this->insert /= i;
		this->remove /= i;
		this->resize /= i;
		return *this;
	}

	NodeCache::NodeCache(int boardHeight, int boardWidth, size_t initialCacheSize) :
			bins(1u << initialCacheSize, nullptr),
			hashing(boardHeight, boardWidth),
			bin_index_mask(bins.size() - 1u)
	{
	}
	NodeCache::NodeCache(NodeCache &&other) :
			bins(std::move(other.bins)),
			buffer(std::move(other.buffer)),
			hashing(std::move(other.hashing)),
			bin_index_mask(std::move(other.bin_index_mask)),
			allocated_entries(std::move(other.allocated_entries)),
			stored_entries(std::move(other.stored_entries)),
			buffered_entries(std::move(other.buffered_entries)),
			stats(std::move(other.stats))
	{
		other.buffer = nullptr;
		other.allocated_entries = other.stored_entries = other.buffered_entries = 0;
	}
	NodeCache& NodeCache::operator =(NodeCache &&other)
	{
		std::swap(this->bins, other.bins);
		std::swap(this->buffer, other.buffer);
		std::swap(this->hashing, other.hashing);
		std::swap(this->bin_index_mask, other.bin_index_mask);
		std::swap(this->allocated_entries, other.allocated_entries);
		std::swap(this->stored_entries, other.stored_entries);
		std::swap(this->buffered_entries, other.buffered_entries);
		std::swap(this->stats, other.stats);
		return *this;
	}
	NodeCache::~NodeCache()
	{
		clear();
		freeUnusedMemory();
	}

	void NodeCache::clearStats() noexcept
	{
		stats.hits = stats.calls = 0;
		stats.seek.reset();
		stats.insert.reset();
		stats.remove.reset();
		stats.resize.reset();
	}
	NodeCacheStats NodeCache::getStats() const noexcept
	{
		return stats;
	}

	uint64_t NodeCache::getMemory() const noexcept
	{
		return sizeof(Entry) * allocated_entries + sizeof(Entry*) * bins.size();
	}
	int NodeCache::allocatedElements() const noexcept
	{
		return allocated_entries;
	}
	int NodeCache::storedElements() const noexcept
	{
		return stored_entries;
	}
	int NodeCache::bufferedElements() const noexcept
	{
		return buffered_entries;
	}
	int NodeCache::numberOfBins() const noexcept
	{
		return bins.size();
	}
	double NodeCache::loadFactor() const noexcept
	{
		return static_cast<double>(stored_entries) / bins.size();
	}

	void NodeCache::reserve(size_t n)
	{
		for (size_t i = buffered_entries; i < n; i++)
			move_to_buffer(new Entry());
	}
	void NodeCache::clear() noexcept
	{
		if (stored_entries == 0)
			return;
		for (size_t i = 0; i < bins.size(); i++)
		{
			while (bins[i] != nullptr)
			{
				Entry *tmp = unlink(bins[i]);
				move_to_buffer(tmp);
			}
			assert(bins[i] == nullptr);
		}
		assert(stored_entries + buffered_entries == allocated_entries);
	}
	Node* NodeCache::seek(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		TimerGuard timer(stats.seek);
		const uint64_t hash = hashing.getHash(board, signToMove);

		stats.calls++; // statistics
		Entry *current = bins[hash & bin_index_mask];
		while (current != nullptr)
		{
			if (current->hash == hash)
			{
				assert(is_hash_collision(current, board, signToMove) == false);
				stats.hits++; // statistics
				return &(current->node);
			}
			current = current->next_entry;
		}
		return nullptr;
	}
	Node* NodeCache::insert(const matrix<Sign> &board, Sign signToMove) noexcept
	{
		assert(seek(board, signToMove) == nullptr);
		TimerGuard timer(stats.insert);
		const uint64_t hash = hashing.getHash(board, signToMove);

		Entry *new_entry = get_new_entry();
		new_entry->hash = hash;

		link(bins[hash & bin_index_mask], new_entry);
		stored_entries++;
		assert(stored_entries + buffered_entries == allocated_entries);

		new_entry->node.setDepth(Board::numberOfMoves(board));
		new_entry->node.setSignToMove(signToMove);
		new_entry->node.markAsUsed();
		return &(new_entry->node);
	}
	void NodeCache::remove(const matrix<Sign> &board, Sign signToMove) noexcept
	{
		TimerGuard timer(stats.remove);
		const uint64_t hash = hashing.getHash(board, signToMove);

		size_t bin_index = hash & bin_index_mask;
		Entry *&current = bins[bin_index];
		while (current != nullptr)
		{
			if (current->hash == hash)
			{
				assert(is_hash_collision(current, board, signToMove) == false);
				Entry *tmp = unlink(current);
				move_to_buffer(tmp);
				break;
			}
			else
				current = current->next_entry;
		}
		assert(stored_entries + buffered_entries == allocated_entries);
	}
	void NodeCache::resize(size_t newSize)
	{
		assert(newSize < 64ull);

		TimerGuard timer(stats.resize);

		newSize = 1ull << newSize;
		bin_index_mask = newSize - 1ull;
		if (bins.size() == newSize)
			return;
		if (stored_entries == 0)
		{
			bins.resize(newSize, nullptr);
			return;
		}

		Entry *storage = nullptr; // all currently stored elements will be appended here
		for (size_t i = 0; i < bins.size(); i++)
		{
			while (bins[i] != nullptr)
			{
				Entry *tmp = unlink(bins[i]);
				link(storage, tmp);
			}
		}

		bins.resize(newSize, nullptr);

		while (storage != nullptr)
		{
			Entry *tmp = unlink(storage);
			link(bins[tmp->hash & bin_index_mask], tmp);
		}
		assert(stored_entries + buffered_entries == allocated_entries);
	}
	void NodeCache::freeUnusedMemory() noexcept
	{
		while (buffer != nullptr)
		{
			Entry *tmp = unlink(buffer);
			assert(tmp->node.isUsed() == false);
			delete tmp;
		}
		buffered_entries = 0;
	}
	/*
	 * private
	 */
	void NodeCache::link(Entry *&prev, Entry *next) noexcept
	{
		// changes : prev = &entry1{...}
		// into    : prev = &next{next_entry = &entry1{...}}
		next->next_entry = prev;
		prev = next;
	}
	NodeCache::Entry* NodeCache::unlink(Entry *&prev) noexcept
	{
		// changes : prev = &entry1{next_entry = &entry2{...}}
		// into    : prev = &entry2{...} and returns pointer to entry1{next_entry = nullptr}
		Entry *result = prev;
		prev = result->next_entry;
		result->next_entry = nullptr;
		return result;
	}
	NodeCache::Entry* NodeCache::get_new_entry()
	{
		if (buffer == nullptr)
		{
			assert(buffered_entries == 0);
			allocated_entries++;
			return new Entry();
		}
		else
		{
			assert(buffered_entries > 0);
			buffered_entries--;
			return unlink(buffer);
		}
	}
	void NodeCache::move_to_buffer(NodeCache::Entry *entry) noexcept
	{
		entry->node.markAsUnused();
		link(buffer, entry);
		assert(stored_entries > 0);
		stored_entries--;
		buffered_entries++;
	}
	bool NodeCache::is_hash_collision(const Entry *entry, const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		const Node &node = entry->node;
		if (node.getSignToMove() != signToMove)
			return true; // sign to move mismatch
		if (node.getDepth() != Board::numberOfMoves(board))
			return true; // depth mismatch
		for (int i = 0; i < node.numberOfEdges(); i++)
		{
			Move move = node.getEdge(i).getMove();
			if (board.at(move.row, move.col) != Sign::NONE)
				return true; // we found an edge representing move that is invalid in this position
		}
		return false; // unfortunately the above checks do not give 100% confidence, but there is nothing more that could be done
	}

} /* namespace ag */

