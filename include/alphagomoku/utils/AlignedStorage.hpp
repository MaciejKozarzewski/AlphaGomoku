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
	template<typename T, int Alignment = 64>
	class AlignedStorage
	{
			T *m_data = nullptr;
			size_t m_size = 0;
		public:
			AlignedStorage() noexcept = default;
			AlignedStorage(size_t size) :
					m_data(new (std::align_val_t(Alignment)) T[size]),
					m_size(size)
			{
			}
			AlignedStorage(const AlignedStorage &other) :
					AlignedStorage(other.size())
			{
				std::memcpy(this->data(), other.data(), sizeof(T) * size());
			}
			AlignedStorage(AlignedStorage &&other) noexcept :
					m_data(other.m_data),
					m_size(other.m_size)
			{
				other.m_data = nullptr;
			}
			AlignedStorage& operator=(const AlignedStorage &other)
			{
				if (this != &other)
				{
					if (this->size() != other.size())
					{
						m_size = other.size();
						m_data = new (std::align_val_t(Alignment)) T[size()];
					}
					std::memcpy(this->data(), other.data(), sizeof(T) * size());
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
				::operator delete[](m_data, std::align_val_t(Alignment));
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
			const T& operator[](int index) const noexcept
			{
				assert(0 <= index && index < size());
				return m_data[index];
			}
			T& operator[](int index) noexcept
			{
				assert(0 <= index && index < size());
				return m_data[index];
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_ALIGNEDSTORAGE_HPP_ */
