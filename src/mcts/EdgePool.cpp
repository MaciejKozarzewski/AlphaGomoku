/*
 * EdgePool.cpp
 *
 *  Created on: Sep 12, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/EdgePool.hpp>
#include <alphagomoku/mcts/Edge.hpp>

namespace ag
{
	EdgePool::BlockDescriptor::BlockDescriptor(Edge *start, size_t size) noexcept :
			block_start(start),
			block_size(size)
	{
	}
	Edge* EdgePool::BlockDescriptor::getEdges(size_t size) noexcept
	{
		assert(size <= block_size);
		Edge *result = block_start;
		block_start += size;
		block_size -= size;
		return result;
	}
	size_t EdgePool::BlockDescriptor::size() const noexcept
	{
		return block_size;
	}

	EdgePool::BlockDescriptor EdgePool::BlockPool::pop() noexcept
	{
		assert(list_of_blocks.size() > 0);
		BlockDescriptor result = list_of_blocks.back();
		list_of_blocks.pop_back();
		return result;
	}
	const EdgePool::BlockDescriptor& EdgePool::BlockPool::peek() const noexcept
	{
		assert(list_of_blocks.size() > 0);
		return list_of_blocks.back();
	}
	void EdgePool::BlockPool::push(const BlockDescriptor &bd)
	{
		list_of_blocks.push_back(bd);
	}
	size_t EdgePool::BlockPool::size() const noexcept
	{
		return list_of_blocks.size();
	}

	EdgePool::BlockOfEdges::BlockOfEdges(size_t size) :
			edges(std::make_unique<Edge[]>(size)),
			is_edge_free(std::make_unique<bool[]>(size)),
			size(size)
	{
	}

	Edge* EdgePool::allocate(size_t size)
	{
		return new Edge[size]; // TODO this needs to be rewritten
	}
	void EdgePool::free(Edge *ptr, size_t size)
	{
		delete[] ptr; // TODO this needs to be rewritten
	}
} /* namespace ag */

