/*
 * NodeCache.cpp
 *
 *  Created on: Sep 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/NodeCache.hpp>
#include <alphagomoku/utils/misc.hpp>

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

	NodeCache::NodeCache(int boardHeight, int boardWidth, size_t initialCacheSize, size_t bucketSize) :
			edge_pool(bucketSize, boardHeight * boardWidth),
			bins(roundToPowerOf2(initialCacheSize), nullptr),
			hashing(boardHeight, boardWidth),
			bin_index_mask(bins.size() - 1u)
	{
	}
	NodeCache::NodeCache(NodeCache &&other) :
			edge_pool(std::move(other.edge_pool)),
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
		std::swap(this->edge_pool, other.edge_pool);
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
		stats.seek.reset();
		stats.insert.reset();
		stats.remove.reset();
		stats.resize.reset();
	}
	NodeCacheStats NodeCache::getStats() const noexcept
	{
		NodeCacheStats result = stats;
		result.load_factor = this->loadFactor();
		result.allocated_edges = allocatedEdges();
		result.stored_edges = storedEdges();
		return result;
	}

	NodeCache::CompressedBoard::CompressedBoard(const matrix<Sign> &board) noexcept
	{
		assert(board.rows() <= static_cast<int>(data.size()) && board.cols() < 32);
		for (int i = 0; i < board.rows(); i++)
		{
			uint64_t tmp = 0ull;
			for (int j = 0; j < board.cols(); j++)
				tmp = tmp | (static_cast<uint64_t>(board.at(i, j)) << (2 * j));
			data[i] = tmp;
		}
		for (size_t i = board.rows(); i < data.size(); i++)
			data[i] = 0ull;
	}
	bool NodeCache::CompressedBoard::operator==(const matrix<Sign> &board) const noexcept
	{
		assert(board.rows() <= static_cast<int>(data.size()) && board.cols() < 32);
		for (int i = 0; i < board.rows(); i++)
		{
			uint64_t tmp = data[i];
			for (int j = 0; j < board.cols(); j++, tmp = tmp >> 2)
				if ((tmp & 3) != static_cast<uint64_t>(board.at(i, j)))
					return false;
		}
		return true;
	}
	bool NodeCache::CompressedBoard::isTransitionPossibleFrom(const NodeCache::CompressedBoard &board) const noexcept
	{
		/*                    transition from
		 *               | NONE | CROSS | CIRCLE |
		 *               |  00  |  01   |  10    |
		 * to    NONE  00|  yes |  no   |  no    |
		 * to   CROSS  01|  yes |  yes  |  no    |
		 * to  CIRCLE  10|  yes |  no   |  yes   |
		 *
		 * The required logical function is AND(XOR(from, to), from) == 0
		 */
		uint64_t result = 0ull;
		for (size_t i = 0; i < data.size(); i++)
		{
			uint64_t from = board.data[i];
			uint64_t to = this->data[i];
			result = result | ((from ^ to) & from);
		}
		return result == 0;
	}

	uint64_t NodeCache::getMemory() const noexcept
	{
		return edge_pool.getMemory() + sizeof(Entry) * storedNodes() + sizeof(Entry*) * bins.size() + sizeof(Edge) * storedEdges();
	}
	int NodeCache::allocatedEdges() const noexcept
	{
		return edge_pool.getAllocatedObjects();
	}
	int NodeCache::storedEdges() const noexcept
	{
		return edge_pool.getUsedObjects();
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
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	void NodeCache::cleanup(const matrix<Sign> &newBoard, Sign signToMove) noexcept
	{
		TimerGuard timer(stats.cleanup);

		if (stats.stored_nodes == 0)
			return;

		CompressedBoard new_board(newBoard);
		for (size_t i = 0; i < bins.size(); i++)
		{
			Entry *current = bins[i];
			bins[i] = nullptr;
			while (current != nullptr)
			{
				Entry *next = current->next_entry;
				if (current->board.isTransitionPossibleFrom(new_board))
				{
					current->next_entry = bins[i];
					bins[i] = current;
				}
				else
					move_to_buffer(current);
				current = next;
			}
		}
		edge_pool.consolidate();
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	Node* NodeCache::seek(const matrix<Sign> &board, Sign signToMove) const noexcept
	{
		TimerGuard timer(stats.seek);
		const uint64_t hash = hashing.getHash(board, signToMove);

		Entry *current = bins[hash & bin_index_mask];
		while (current != nullptr)
		{
			if (current->hash == hash and current->board == board and current->node.getSignToMove() == signToMove)
			{
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
		new_entry->board = CompressedBoard(board);

		const uint64_t hash = hashing.getHash(board, signToMove);
		new_entry->hash = hash;

		size_t bin_index = hash & bin_index_mask;
		new_entry->next_entry = bins[bin_index];
		bins[bin_index] = new_entry;

		stats.stored_nodes++;
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);

		new_entry->node.clear();
		new_entry->edge_block = edge_pool.allocate(numberOfEdges);
		new_entry->node.setEdges(new_entry->edge_block.get(), numberOfEdges);
		new_entry->node.setDepth(Board::numberOfMoves(board));
		new_entry->node.setSignToMove(signToMove);

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
			if (current->hash == hash and current->board == board and current->node.getSignToMove() == signToMove)
				move_to_buffer(current);
			else
			{
				current->next_entry = bins[bin_index];
				bins[bin_index] = current;
			}
			current = next;
		}
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	void NodeCache::resize(size_t newSize)
	{
		TimerGuard timer(stats.resize);

		newSize = roundToPowerOf2(newSize);
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

		bins.resize(newSize, nullptr);

		while (storage != nullptr)
		{
			Entry *next = storage->next_entry;
			size_t bin_index = storage->hash % bins.size();
			storage->next_entry = bins[bin_index];
			bins[bin_index] = storage;
			storage = next;
		}
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
		buffered_nodes = 0;
		stats.allocated_nodes = stats.stored_nodes;
		assert(stats.stored_nodes + buffered_nodes == stats.allocated_nodes);
	}
	/*
	 * private
	 */
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
		}
	}
	void NodeCache::move_to_buffer(NodeCache::Entry *entry) noexcept
	{
		entry->node.freeEdges();
		edge_pool.free(entry->edge_block);

		entry->next_entry = buffer;
		buffer = entry;
		assert(stats.stored_nodes > 0);
		stats.stored_nodes--;
		buffered_nodes++;
	}

} /* namespace ag */

