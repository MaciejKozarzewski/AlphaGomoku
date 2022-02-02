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
#include <cassert>

namespace ag
{
	template<typename T>
	class ObjectPool
	{
		private:
			/**
			 * @brief Class describing block of continuous memory but not owning it.
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
					T* getEdges(size_t size) noexcept
					{
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
						m_list_of_blocks.push_back(bd);
					}
					size_t size() const noexcept
					{
						return m_list_of_blocks.size();
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
			};
		public:
			[[nodiscard]] T* allocate(size_t elements)
			{
				return new T[elements]; // TODO this needs to be rewritten
			}
			void free(T *ptr, size_t elements)
			{
				delete[] ptr; // TODO this needs to be rewritten
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_OBJECTPOOL_HPP_ */
