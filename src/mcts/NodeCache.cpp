/*
 * NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <iostream>

namespace
{
	size_t round_to_power_of_2(size_t x)
	{
		size_t result = 1;
		while (result <= x)
			result *= 2;
		return result / 2;
	}
}

namespace ag
{
	NodeCacheStats::NodeCacheStats() :
			seek("seek    "),
			insert("insert  "),
			remove("remove  "),
			resize("resize  "),
			cleanup("cleanup ")
	{
	}
	std::string NodeCacheStats::toString() const
	{
		std::string result = "----NodeCacheStats----\n";
		if (calls == 0)
			result += "hit rate = N/A\n";
		else
			result += "hit rate = " + std::to_string(static_cast<int>(1000.0 * static_cast<double>(hits) / calls) / 10.0) + "%\n";
		result += "load factor = " + std::to_string(static_cast<int>(1000.0 * load_factor) / 1000.0) + "\n";

		result += "stored nodes    = " + std::to_string(stored_nodes) + '\n';
		result += "allocated nodes = " + std::to_string(allocated_nodes) + '\n';
		result += "stored edges    = " + std::to_string(stored_edges) + '\n';
		result += "allocated edges = " + std::to_string(allocated_edges) + '\n';

		result += seek.toString() + '\n';
		result += insert.toString() + '\n';
		result += remove.toString() + '\n';
		result += resize.toString() + '\n';
		result += cleanup.toString() + '\n';
		return result;
	}
	NodeCacheStats& NodeCacheStats::operator+=(const NodeCacheStats &other) noexcept
	{
		this->hits += other.hits;
		this->calls += other.calls;
		this->collisions += other.collisions;

		this->stored_nodes += other.stored_nodes;
		this->allocated_nodes += other.allocated_nodes;
		this->stored_edges += other.stored_edges;
		this->allocated_edges += other.allocated_edges;

		this->seek += other.seek;
		this->insert += other.insert;
		this->remove += other.remove;
		this->resize += other.resize;
		this->cleanup += other.cleanup;
		return *this;
	}
	NodeCacheStats& NodeCacheStats::operator/=(int i) noexcept
	{
		this->hits /= i;
		this->calls /= i;
		this->collisions /= i;

		this->stored_nodes /= i;
		this->allocated_nodes /= i;
		this->stored_edges /= i;
		this->allocated_edges /= i;

		this->seek /= i;
		this->insert /= i;
		this->remove /= i;
		this->resize /= i;
		this->cleanup /= i;
		return *this;
	}

	NodeCache::NodeCache(int boardHeight, int boardWidth, size_t initialCacheSize) :
			bins(round_to_power_of_2(initialCacheSize), nullptr),
			hashing(boardHeight, boardWidth),
			bin_index_mask(bins.size() - 1u)
	{
	}
	NodeCache::NodeCache(NodeCache &&other) :
			bins(std::move(other.bins)),
			buffer(std::move(other.buffer)),
			hashing(std::move(other.hashing)),
			bin_index_mask(std::move(other.bin_index_mask)),
			buffered_nodes(std::move(other.buffered_nodes)),
			stats(std::move(other.stats))
	{
		other.buffer = nullptr;
	}
	NodeCache& NodeCache::operator =(NodeCache &&other)
	{
		std::swap(this->bins, other.bins);
		std::swap(this->buffer, other.buffer);
		std::swap(this->hashing, other.hashing);
		std::swap(this->bin_index_mask, other.bin_index_mask);
		std::swap(this->buffered_nodes, other.buffered_nodes);
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
		NodeCacheStats result = stats;
		result.load_factor = this->loadFactor();
		return result;
	}

	uint64_t NodeCache::getMemory() const noexcept
	{
		return entry_size_in_bytes * storedNodes() + sizeof(Entry*) * bins.size() + sizeof(Edge) * storedEdges();
	}
	int NodeCache::allocatedEdges() const noexcept
	{
		return stats.allocated_edges;
	}
	int NodeCache::storedEdges() const noexcept
	{
		return stats.stored_edges;
	}
	int NodeCache::allocatedNodes() const noexcept
	{
		return stats.allocated_nodes;
	}
	int NodeCache::storedNodes() const noexcept
	{
		return stats.stored_nodes;
	}
	int NodeCache::bufferedNodes() const noexcept
	{
		return buffered_nodes;
	}
	int NodeCache::numberOfBins() const noexcept
	{
		return bins.size();
	}
	double NodeCache::loadFactor() const noexcept
	{
		return static_cast<double>(stats.stored_nodes) / bins.size();
	}

//	void NodeCache::reserve(size_t n)
//	{
//		for (size_t i = buffered_nodes; i < n; i++)
//			move_to_buffer(new Entry());
//	}
	void NodeCache::clear() noexcept
	{
		if (stats.stored_nodes == 0)
			return;
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				move_to_buffer(current);
				current = next;
			}
			bins[i] = nullptr;
		}
//		for (size_t i = 0; i < bins.size(); i++)
//		{
//			while (bins[i] != nullptr)
//			{
//				Entry *tmp = unlink(&(bins[i]));
//				move_to_buffer(tmp);
//			}
//			assert(bins[i] == nullptr);
//		}
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	void NodeCache::cleanup(const matrix<Sign> &newBoard, Sign signToMove) noexcept
	{
		TimerGuard timer(stats.cleanup);

		if (stats.stored_nodes == 0)
			return;
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			bins[i] = nullptr;
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				if (Board::isTransitionPossible(newBoard, current->board))
				{
					current->next_entry = bins[i];
					bins[i] = current;
				}
				else
					move_to_buffer(current);
				current = next;
			}
		}
//		for (size_t i = 0; i < bins.size(); i++)
//		{
//			Entry **current = &(bins[i]);
//			while (current != nullptr)
//			{
//				if (Board::isTransitionPossible(newBoard, (*current)->board))
//					current = &((*current)->next_entry);
//				else
//				{
//					Entry *tmp = unlink(current);
//					move_to_buffer(tmp);
//				}
//			}
//		}
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	Node* NodeCache::seek(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		TimerGuard timer(stats.seek);
		const uint64_t hash = hashing.getHash(board, signToMove);

		stats.calls++; // statistics
		Entry *current = bins[hash & bin_index_mask];
		while (current != nullptr)
		{
			if (current->hash == hash and current->board == board)
			{
				stats.hits++; // statistics
				return &(current->node);
			}
			current = current->next_entry;
		}
		return nullptr;
	}
	Node* NodeCache::insert(const matrix<Sign> &board, Sign signToMove, int numberOfEdges) noexcept
	{
		assert(seek(board, signToMove) == nullptr); // the board state must not be in the cache
		if (loadFactor() >= 1.0)
			resize(2 * numberOfBins());

		TimerGuard timer(stats.insert);

		Entry *new_entry = get_new_entry();
		new_entry->board = board;
		if (entry_size_in_bytes == 0)
			entry_size_in_bytes = new_entry->sizeInBytes();

		const uint64_t hash = hashing.getHash(board, signToMove);
		new_entry->hash = hash;

		size_t bin_index = hash & bin_index_mask;
		new_entry->next_entry = bins[bin_index];
		bins[bin_index] = new_entry;

//		link(&(bins[hash & bin_index_mask]), new_entry);
		stats.stored_nodes++;
		stats.allocated_edges += numberOfEdges - new_entry->node.numberOfEdges();
		stats.stored_edges += numberOfEdges;
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);

		new_entry->node.clear();
		new_entry->node.setEdges(numberOfEdges);
		new_entry->node.setDepth(Board::numberOfMoves(board));
		new_entry->node.setSignToMove(signToMove);
		new_entry->node.markAsUsed();

		return &(new_entry->node);
	}
	void NodeCache::remove(const matrix<Sign> &board, Sign signToMove) noexcept
	{
		assert(seek(board, signToMove) != nullptr); // the board state must be in the cache

		TimerGuard timer(stats.remove);
		const uint64_t hash = hashing.getHash(board, signToMove);

		size_t bin_index = hash & bin_index_mask;
		Entry *current = bins[bin_index];
		bins[bin_index] = nullptr;
		while (current != nullptr)
		{
			Entry *next = current->next_entry;
			if (current->hash == hash and current->board == board)
				move_to_buffer(current);
			else
			{
				current->next_entry = bins[bin_index];
				bins[bin_index] = current;
			}
			current = next;
		}

//		Entry **current = &(bins[bin_index]);
//		while ((*current) != nullptr)
//		{
//			if ((*current)->hash == hash)
//			{
//				assert(is_hash_collision(*current, board, signToMove) == false);
//				Entry *tmp = unlink(current);
//				move_to_buffer(tmp);
//				break;
//			}
//			else
//				current = &((*current)->next_entry);
//		}
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	void NodeCache::resize(size_t newSize)
	{
		TimerGuard timer(stats.resize);

		newSize = round_to_power_of_2(newSize);
		bin_index_mask = newSize - 1ull;
		if (bins.size() == newSize)
			return;
		if (stats.stored_nodes == 0)
		{
			bins.resize(newSize, nullptr);
			return;
		}

		Entry *storage = nullptr;
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			bins[i] = nullptr;
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				current->next_entry = storage;
				storage = current;
				current = next;
			}
		}

//		Entry *storage = nullptr; // all currently stored elements will be appended here
//		for (size_t i = 0; i < bins.size(); i++)
//		{
//			while (bins[i] != nullptr)
//			{
//				Entry *tmp = unlink(&(bins[i]));
//				link(&storage, tmp);
//			}
//		}

		bins.resize(newSize, nullptr);

		while (storage != nullptr)
		{
			Entry *next = storage->next_entry;
			size_t bin_index = storage->hash % bins.size();
			storage->next_entry = bins[bin_index];
			bins[bin_index] = storage;
			storage = next;
		}

//		while (storage != nullptr)
//		{
//			Entry *tmp = unlink(&storage);
//			link(&(bins[tmp->hash & bin_index_mask]), tmp);
//		}
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	void NodeCache::freeUnusedMemory() noexcept
	{
		while (buffer != nullptr)
		{
			Entry *next = buffer->next_entry;
			delete buffer;
			buffer = next;
		}
//		while (buffer != nullptr)
//		{
//			Entry *tmp = unlink(&buffer);
//			assert(tmp->node.isUsed() == false);
//			delete tmp;
//		}
		buffered_nodes = 0;
		stats.allocated_nodes = stats.stored_nodes;
		stats.allocated_edges = stats.stored_edges;
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	/*
	 * private
	 */
//	void NodeCache::link(Entry **prev, Entry *next) noexcept
//	{
//		// changes : prev = &entry1{...}
//		// into    : prev = &next{next_entry = &entry1{...}}
//		next->next_entry = *prev;
//		*prev = next;
//	}
//	NodeCache::Entry* NodeCache::unlink(Entry **prev) noexcept
//	{
//		// changes : prev = &entry1{next_entry = &entry2{...}}
//		// into    : prev = &entry2{...} and returns pointer to entry1{next_entry = nullptr}
//		Entry *result = *prev;
//		*prev = result->next_entry;
//		result->next_entry = nullptr;
//		return result;
//	}
	NodeCache::Entry* NodeCache::get_new_entry()
	{
		if (buffer == nullptr)
		{
			assert(buffered_nodes == 0);
			stats.allocated_nodes++;
			return new Entry();
		}
		else
		{
			assert(buffered_nodes > 0);
			buffered_nodes--;
			Entry *result = buffer;
			buffer = result->next_entry;
			result->next_entry = nullptr;
			return result;
//			return unlink(&buffer);
		}
	}
	void NodeCache::move_to_buffer(NodeCache::Entry *entry) noexcept
	{
		entry->node.markAsUnused();
		entry->next_entry = buffer;
		buffer = entry;
		assert(stats.stored_nodes > 0);
		assert(stats.stored_edges > 0);
		stats.stored_nodes--;
		stats.stored_edges -= entry->node.numberOfEdges();
		buffered_nodes++;
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

