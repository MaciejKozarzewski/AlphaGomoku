/*
 * AlignedStorage.hpp
 *
 *  Created on: Oct 21, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_ALIGNEDSTORAGE_HPP_
#define ALPHAGOMOKU_UTILS_ALIGNEDSTORAGE_HPP_

#include <cstring>
#include <algorithm>
#include <cassert>

namespace ag
{
	template<typename T, int Alignment = 16>
	class AlignedStorage
	{
			size_t m_size = 0;
			T *m_data = nullptr;

			static void* aligned_new(size_t size, size_t alignment)
			{
				if (size == 0)
					return nullptr;
				else
					return ::operator new[](size, std::align_val_t(alignment));
			}
			static void aligned_delete(void *ptr, size_t alignment)
			{
				if (ptr != nullptr)
					::operator delete[](ptr, std::align_val_t(alignment));
			}
		public:
			AlignedStorage() noexcept = default;
			AlignedStorage(size_t size) :
					m_size(size),
					m_data(reinterpret_cast<T*>(aligned_new(sizeInBytes(), Alignment)))
			{
			}
			AlignedStorage(const AlignedStorage &other) :
					AlignedStorage(other.size())
			{
				if (sizeInBytes() > 0)
					std::memcpy(this->data(), other.data(), sizeInBytes());
			}
			AlignedStorage(AlignedStorage &&other) noexcept :
					m_size(other.m_size),
					m_data(other.m_data)
			{
				other.m_data = nullptr;
			}
			AlignedStorage& operator=(const AlignedStorage &other)
			{
				if (this != &other)
				{
					if (this->size() != other.size())
					{
						aligned_delete(m_data, Alignment);
						m_size = other.size();
						m_data = reinterpret_cast<T*>(aligned_new(sizeInBytes(), Alignment));
					}
					if (sizeInBytes() > 0)
						std::memcpy(this->data(), other.data(), sizeInBytes());
				}
				return *this;
			}
			AlignedStorage& operator=(AlignedStorage &&other) noexcept
			{
				std::swap(this->m_data, other.m_data);
				std::swap(this->m_size, other.m_size);
				return *this;
			}
			~AlignedStorage() noexcept
			{
				aligned_delete(m_data, Alignment);
			}

			size_t size() const noexcept
			{
				return m_size;
			}
			size_t sizeInBytes() const noexcept
			{
				return sizeof(T) * size();
			}
			const T* data() const noexcept
			{
				return m_data;
			}
			T* data() noexcept
			{
				return m_data;
			}
			const T* begin() const noexcept
			{
				return data();
			}
			T* begin() noexcept
			{
				return data();
			}
			const T* end() const noexcept
			{
				return begin() + size();
			}
			T* end() noexcept
			{
				return begin() + size();
			}
			const T& operator[](size_t index) const noexcept
			{
				assert(index < size());
				return m_data[index];
			}
			T& operator[](size_t index) noexcept
			{
				assert(index < size());
				return m_data[index];
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_ALIGNEDSTORAGE_HPP_ */
