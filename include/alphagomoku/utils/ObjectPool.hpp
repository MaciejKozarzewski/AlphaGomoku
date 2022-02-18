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
#include <map>
#include <cassert>
#include <iostream>

namespace ag
{
	template<typename T>
	class ObjectPool
	{
		private:
			class ListOfBlocks
			{
					std::vector<std::unique_ptr<T[]>> m_blocks;
				public:
					std::unique_ptr<T[]> getNewBlock(size_t size)
					{
						if (m_blocks.empty())
							return std::make_unique<T[]>(size);
						else
						{
							std::unique_ptr<T[]> result = std::move(m_blocks.back());
							m_blocks.pop_back();
							return result;
						}
					}
					void addBlock(std::unique_ptr<T[]> &block)
					{
						m_blocks.push_back(std::move(block));
					}
					size_t size() const noexcept
					{
						return m_blocks.size();
					}
			};
			std::vector<ListOfBlocks> m_pools;
		public:
			ObjectPool(size_t maxBlockSize = 0) :
					m_pools(maxBlockSize)
			{
			}
			std::unique_ptr<T[]> getNewBlockOfSize(size_t size)
			{
				assert(size < m_pools.size());
				return m_pools[size].getNewBlock(size);
			}
			void releaseBlock(std::unique_ptr<T[]> &block, size_t size)
			{
				assert(size < m_pools.size());
				m_pools[size].addBlock(block);
			}
			size_t numberOfObjects() const noexcept
			{
				size_t result = 0;
				for (size_t i = 0; i < m_pools.size(); i++)
					result += i * m_pools[i].size();
				return result;
			}
			uint64_t getMemory() const noexcept
			{
				uint64_t result = 0;
				for (size_t i = 0; i < m_pools.size(); i++)
					result += sizeof(T*) * m_pools[i].size();
				return result;
			}
	};

	template<typename T>
	class ObjectPool_v1
	{
		private:
			/**
			 * @brief Class describing block of contiguous memory but not owning it.
			 */
			class BlockDescriptor
			{
				private:
					T *m_block_start = nullptr; // non-owning
					size_t m_block_size = 0;
				public:
					BlockDescriptor() = default;
					BlockDescriptor(T *start, size_t size) noexcept :
							m_block_start(start),
							m_block_size(size)
					{
					}
					/**
					 * \brief Extract elements from the block, effectively reducing its size
					 */
					T* getElements(size_t size) noexcept
					{
						assert(m_block_start != nullptr);
						assert(size <= m_block_size);
						T *result = m_block_start;
						m_block_start += size;
						m_block_size -= size;
						return result;
					}
					size_t size() const noexcept
					{
						return m_block_size;
					}
					bool isInRange(const T *begin, const T *end) const noexcept
					{
						return begin <= m_block_start and (m_block_start + m_block_size) <= end;
					}
			};
			/**
			 * @brief Class that collects multiple block descriptors.
			 */
			class PoolOfBlocks
			{
				private:
					std::vector<BlockDescriptor> m_list_of_blocks;
				public:
					[[nodiscard]] BlockDescriptor pop() noexcept
					{
						assert(m_list_of_blocks.size() > 0);
						BlockDescriptor result = m_list_of_blocks.back();
						m_list_of_blocks.pop_back();
						return result;
					}
					const BlockDescriptor& peek() const noexcept
					{
						assert(m_list_of_blocks.size() > 0);
						return m_list_of_blocks.back();
					}
					void push(const BlockDescriptor &bd)
					{
						assert(bd.size() > 0);
						m_list_of_blocks.push_back(bd);
					}
					size_t size() const noexcept
					{
						return m_list_of_blocks.size();
					}
					void sort()
					{
						std::sort(m_list_of_blocks.begin(), m_list_of_blocks.end(), [](const BlockDescriptor &lhs, const BlockDescriptor &rhs)
						{	return lhs.size() < rhs.size();});
					}
			};
			/**
			 * @brief Class that owns a block of memory for objects.
			 */
			class BlockOfObjects
			{
				private:
					std::unique_ptr<T[]> m_objects;
					std::unique_ptr<bool[]> m_is_free;
					size_t m_elements;
				public:
					BlockOfObjects(size_t elements) :
							m_objects(std::make_unique<T[]>(elements)),
							m_is_free(std::make_unique<bool[]>(elements)),
							m_elements(elements)
					{
					}
					bool contains(const BlockDescriptor &desc) const noexcept
					{
						return desc.isInRange(m_objects.get(), m_objects.get() + m_elements);
					}
					BlockDescriptor asSingleBlock() noexcept
					{
						return BlockDescriptor(m_objects.get(), m_elements);
					}
					void markAsFree(const T *blockPointer, size_t blockSize) noexcept
					{
						assert(m_objects.get() <= blockPointer && (blockPointer + blockSize) < (m_objects.get() + m_elements));
						size_t offset = std::distance(m_objects.get(), blockPointer);
						std::fill(m_is_free.get() + offset, m_is_free.get() + offset + blockSize, true);
					}
					void markAsUsed(const T *blockPointer, size_t blockSize) noexcept
					{
						assert(m_objects.get() <= blockPointer && (blockPointer + blockSize) < (m_objects.get() + m_elements));
						size_t offset = std::distance(m_objects.get(), blockPointer);
						std::fill(m_is_free.get() + offset, m_is_free.get() + offset + blockSize, false);
					}
					std::vector<BlockDescriptor> getFreeBlocks() const
					{
						std::vector<BlockDescriptor> result;
						T *block_start = nullptr;
						size_t block_size = 0;
						for (size_t i = 0; i < m_elements; i++)
						{
							if (m_is_free[i])
							{
								if (block_start == nullptr)
									block_start = m_objects.get() + i; // is the element is free and there is no currently scanned block, it is a start of a new block
								else
									block_size++; // if the element is free and there is a currently scanned block, increase the size of it
							}
							else
							{
								if (block_start != nullptr)
								{ // if the element is not free but there is a currently scanned block, it just ended so we can append it to the result and clear temporary variables
									result.push_back(BlockDescriptor(block_start, block_size));
									block_start = nullptr;
									block_size = 0;
								}
							}
						}
						return result;
					}
			};

			std::vector<PoolOfBlocks> m_list_of_small_blocks;
			PoolOfBlocks m_list_of_large_blocks;

			std::vector<BlockOfObjects> m_owning_blocks;
			size_t m_bucket_size;

			std::map<size_t, std::vector<std::unique_ptr<T[]>>> m_blocks;

			std::map<size_t, size_t> size_stats;
		public:
			ObjectPool_v1(size_t bucketSize = 1000, size_t expectedMaxBlockSize = 100) :
					m_list_of_small_blocks(expectedMaxBlockSize + 1),
					m_bucket_size(bucketSize)
			{
			}
			~ObjectPool_v1()
			{
				for (auto iter = size_stats.begin(); iter != size_stats.end(); iter++)
					std::cout << "block size " << iter->first << " count " << iter->second << '\n';
			}

			void consolidate()
			{

			}
			/**
			 * \param[in] elements Number of elements to allocate
			 */
			[[nodiscard]] T* allocate(size_t elements)
			{
				if (size_stats.find(elements) == size_stats.end())
					size_stats.insert( { elements, 1 });
				else
					size_stats.find(elements)->second++;
//				for (size_t i = elements; i < m_list_of_small_blocks.size(); i++)  // try to find some space in small blocks
//				{
//					PoolOfBlocks &pool = m_list_of_small_blocks[elements];
//					if (pool.size() > 0)
//					{
//						BlockDescriptor block = pool.pop();
//						assert(result.size() >= elements);
//						T *result = block.getElements(elements);
//						if (block.size() > 0)
//						{
//							assert(block.size() <= m_list_of_small_blocks.size());
//							m_list_of_small_blocks[block.size()].push(block); // append remaining block to appropriate pool
//						}
//						return result;
//					}
//				}
//
//				if (m_list_of_large_blocks.size() == 0) // there are no more large blocks, must create a new one
//				{
//					m_owning_blocks.push_back(BlockOfObjects(m_bucket_size));
//					m_list_of_large_blocks.push(m_owning_blocks.back().asSingleBlock());
//				}
//
//				BlockDescriptor block = m_list_of_large_blocks.pop();
//				T *result = block.getElements(elements);
//
//				if (block.size() > 0)
//				{
//					if (block.size() < m_list_of_small_blocks.size())
//						m_list_of_small_blocks[block.size()].push(block); // append remaining block to appropriate pool
//					else
//						m_list_of_large_blocks.push(block); // append remaining block to the large blocks list
//				}
//
//				return result;

//				if (elements == 0)
//					return nullptr;
//				else
//				{
//					auto tmp = m_blocks.find(elements);
//					if (tmp != m_blocks.end())
//					{
//						if (tmp->second.size() > 0)
//						{
//							std::unique_ptr<T[]> result(std::move(tmp->second.back()));
//							tmp->second.pop_back();
//							return result.release();
//						}
//					}
//					return new T[elements];
//				}

				if (elements == 0)
					return nullptr;
				else
					return new T[elements]; // TODO this needs to be rewritten
			}
			/**
			 * \param[in] ptr Pointer to be deallocated
			 * \param[in] elements Number of elements to be deallocated. It is important that this value matches the original number of elements that was allocated.
			 */
			void free(T *ptr, size_t elements)
			{
//				assert(elements > 0);
//				auto tmp = m_blocks.find(elements);
//				if (tmp == m_blocks.end())
//					m_blocks.insert( { elements, std::vector<std::unique_ptr<T[]>>() });
//				tmp->second.push_back(std::unique_ptr<T[]>(ptr));
				delete[] ptr; // TODO this needs to be rewritten
			}
		private:
			BlockOfObjects& get_owning_block(const BlockDescriptor &desc) const
			{
				for (auto iter = m_owning_blocks.begin(); iter < m_owning_blocks.end(); iter++)
					if (iter->contains(desc))
						return *iter;
				throw std::logic_error("block is not a part of a pool");
			}
	}
	;

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_OBJECTPOOL_HPP_ */
