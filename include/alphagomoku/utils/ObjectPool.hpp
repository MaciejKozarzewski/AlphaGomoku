/*
 * ObjectPool.hpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_OBJECTPOOL_HPP_
#define ALPHAGOMOKU_UTILS_OBJECTPOOL_HPP_

#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <cassert>
#include <iostream>

namespace ag
{
	template<typename T>
	class ObjectPool;

	/**
	 * \brief Class describing block of contiguous memory but not owning it.
	 */
	template<typename T>
	class BlockDescriptor
	{
			friend class ObjectPool<T> ;
		private:
			T *m_block_start = nullptr; // non-owning
			uint32_t m_block_size = 0;
			uint32_t m_source_block_index = -1;
			BlockDescriptor(T *start, uint32_t size, uint32_t sourceIndex) noexcept :
					m_block_start(start),
					m_block_size(size),
					m_source_block_index(sourceIndex)
			{
			}
		public:
			BlockDescriptor() = default;
			/**
			 * \brief Extract elements from the block, effectively reducing its size
			 */
			BlockDescriptor<T> getSubBlock(uint32_t size) noexcept
			{
				assert(m_block_start != nullptr);
				assert(size <= m_block_size);
				BlockDescriptor<T> result(this->m_block_start, size, this->m_source_block_index);
				this->m_block_start += size;
				this->m_block_size -= size;
				return result;
			}
			const T* get() const noexcept
			{
				return m_block_start;
			}
			T* get() noexcept
			{
				return m_block_start;
			}
			size_t size() const noexcept
			{
				return m_block_size;
			}
			size_t sourceIndex() const noexcept
			{
				return m_source_block_index;
			}
			bool isEmpty() const noexcept
			{
				return m_block_start == nullptr;
			}
	};

	template<typename T>
	class ObjectPool
	{
		private:
			/**
			 * \brief Class that collects multiple block descriptors.
			 */
			class PoolOfBlocks
			{
				private:
					std::vector<BlockDescriptor<T>> m_list_of_blocks;
				public:
					void clear() noexcept
					{
						m_list_of_blocks.clear();
					}
					[[nodiscard]] BlockDescriptor<T> pop() noexcept
					{
						assert(m_list_of_blocks.size() > 0);
						BlockDescriptor<T> result = m_list_of_blocks.back();
						m_list_of_blocks.pop_back();
						return result;
					}
					const BlockDescriptor<T>& peek() const noexcept
					{
						assert(m_list_of_blocks.size() > 0);
						return m_list_of_blocks.back();
					}
					void push(const BlockDescriptor<T> &block)
					{
						assert(block.size() > 0);
						m_list_of_blocks.push_back(block);
					}
					size_t size() const noexcept
					{
						return m_list_of_blocks.size();
					}
					bool isEmpty() const noexcept
					{
						return m_list_of_blocks.empty();
					}
					void sort()
					{
						std::sort(m_list_of_blocks.begin(), m_list_of_blocks.end(), [](const BlockDescriptor<T> &lhs, const BlockDescriptor<T> &rhs)
						{	return lhs.size() < rhs.size();});
					}
			};
			/**
			 * \brief Class that owns a block of memory for objects.
			 */
			class OwningBlockOfObjects
			{
				private:
					std::vector<T> m_objects;
					std::vector<bool> m_is_free;
					size_t m_index;

					size_t get_distance(const BlockDescriptor<T> &block) const noexcept
					{
						assert(block.sourceIndex() == m_index);
						assert(m_objects.data() <= block.get());
						assert((block.get() + block.size()) <= (m_objects.data() + m_objects.size()));
						return std::distance(static_cast<const T*>(m_objects.data()), block.get());
					}
				public:
					OwningBlockOfObjects(size_t elements, size_t index) :
							m_objects(elements),
							m_is_free(elements, true),
							m_index(index)
					{
					}
					BlockDescriptor<T> asSingleBlock() noexcept
					{
						return BlockDescriptor<T>(m_objects.data(), m_objects.size(), m_index);
					}
					void markAsFree(const BlockDescriptor<T> &block) noexcept
					{
						const size_t offset = get_distance(block);
						assert(offset < m_objects.size());
						assert(offset + block.size() <= m_objects.size());
						std::fill(m_is_free.begin() + offset, m_is_free.begin() + offset + block.size(), true);
					}
					void markAsUsed(const BlockDescriptor<T> &block) noexcept
					{
						const size_t offset = get_distance(block);
						assert(offset < m_objects.size());
						assert(offset + block.size() <= m_objects.size());
						std::fill(m_is_free.begin() + offset, m_is_free.begin() + offset + block.size(), false);
					}
					std::vector<BlockDescriptor<T>> getFreeBlocks()
					{
						std::vector<BlockDescriptor<T>> result;
						T *block_start = nullptr;
						size_t block_size = 0;
						for (size_t i = 0; i < m_is_free.size(); i++)
						{
							if (m_is_free[i])
							{
								if (block_start == nullptr)
									block_start = m_objects.data() + i; // if the element is free and there is no currently scanned block, it is a start of a new block
								else
									block_size++; // if the element is free and there is a currently scanned block, increase the size of it
							}
							else
							{
								if (block_start != nullptr)
								{ // if the element is not free but there is a currently scanned block, it just ended so we can append it to the result and clear temporary variables
									result.push_back(BlockDescriptor<T>(block_start, block_size, m_index));
									block_start = nullptr;
									block_size = 0;
								}
							}
						}
						if (block_start != nullptr)
						{ // if the element is not free but there is a currently scanned block, it just ended so we can append it to the result and clear temporary variables
							result.push_back(BlockDescriptor<T>(block_start, block_size + 1, m_index)); // adding one to make the block point to the end of array
						}
						return result;
					}
					size_t countUsed() const noexcept
					{
						size_t result = 0;
						for (size_t i = 0; i < m_is_free.size(); i++)
							result += static_cast<size_t>(m_is_free[i]);
						return result;
					}
			};

			std::vector<PoolOfBlocks> m_list_of_small_blocks;
			PoolOfBlocks m_list_of_large_blocks;

			std::vector<OwningBlockOfObjects> m_owning_blocks;
			size_t m_bucket_size;

			size_t m_allocated_objects = 0;
			size_t m_used_objects = 0;

		public:
			ObjectPool(size_t bucketSize = 1000, size_t expectedMaxBlockSize = 100) :
					m_list_of_small_blocks(expectedMaxBlockSize + 1),
					m_bucket_size(bucketSize)
			{
			}
			~ObjectPool()
			{
			}
			size_t getUsedObjects() const noexcept
			{
				return m_used_objects;
			}
			size_t getAllocatedObjects() const noexcept
			{
				return m_allocated_objects;
			}

			void consolidate()
			{
				m_list_of_large_blocks.clear();
				for (size_t i = 0; i < m_list_of_small_blocks.size(); i++)
					m_list_of_small_blocks[i].clear();

				for (size_t i = 0; i < m_owning_blocks.size(); i++)
				{
					std::vector<BlockDescriptor<T>> blocks = m_owning_blocks[i].getFreeBlocks();
					for (size_t j = 0; j < blocks.size(); j++)
						store_block(blocks[j]);
				}
				m_list_of_large_blocks.sort();
			}
			[[nodiscard]] BlockDescriptor<T> allocate(size_t elements)
			{
				for (size_t i = elements; i < m_list_of_small_blocks.size(); i++)  // try to find some space in small blocks
				{
					PoolOfBlocks &pool = m_list_of_small_blocks[i];
					if (not pool.isEmpty())
						return get_block(pool, elements);
				}

				// there are no more large blocks or it is too small, must create a new one
				if (m_list_of_large_blocks.isEmpty() or m_list_of_large_blocks.peek().size() < elements)
				{
					m_allocated_objects += m_bucket_size;
					const size_t index = m_owning_blocks.size();
					m_owning_blocks.push_back(OwningBlockOfObjects(m_bucket_size, index));
					m_list_of_large_blocks.push(m_owning_blocks.back().asSingleBlock());
				}

				return get_block(m_list_of_large_blocks, elements);
			}
			void free(BlockDescriptor<T> &block)
			{
				if (not block.isEmpty())
				{
					m_used_objects -= block.size();
					const size_t src_idx = block.sourceIndex();
					m_owning_blocks[src_idx].markAsFree(block);
					store_block(block);
					block = BlockDescriptor<T>();
				}
			}
			int64_t getMemory() const noexcept
			{
				return sizeof(T) * m_bucket_size * m_owning_blocks.size();
			}
		private:
			BlockDescriptor<T> get_block(PoolOfBlocks &pool, size_t elements)
			{
				assert(pool.isEmpty() == false);
				BlockDescriptor<T> block = pool.pop();
				assert(block.size() >= elements);

				const size_t src_idx = block.sourceIndex();
				BlockDescriptor<T> result = block.getSubBlock(elements);
				m_owning_blocks[src_idx].markAsUsed(result);

				store_block(block);
				m_used_objects += result.size();
				return result;
			}
			void store_block(BlockDescriptor<T> &block)
			{
				if (block.size() > 0)
				{
					if (block.size() < m_list_of_small_blocks.size())
						m_list_of_small_blocks[block.size()].push(block); // append remaining block to appropriate pool
					else
						m_list_of_large_blocks.push(block); // append remaining block to the large blocks list
				}
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_OBJECTPOOL_HPP_ */
