/*
 * AlignedAllocator.hpp
 *
 *  Created on: Feb 19, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_ALIGNEDALLOCATOR_HPP_
#define ALPHAGOMOKU_UTILS_ALIGNEDALLOCATOR_HPP_

#include <cstddef>
#include <limits>
#include <new>

namespace ag
{
	template<typename T, std::size_t Alignment = 16>
	class AlignedAllocator
	{
			static_assert(Alignment >= alignof(T), "Alignment must be at least alignof(T)");
			static_assert(Alignment % alignof(T) == 0, "Alignment must be a multiple of alignof(T)");
		public:
			using value_type = T;

			/*
			 * https://stackoverflow.com/a/48062758/2191065
			 */
			template<class U>
			struct rebind
			{
					using other = AlignedAllocator<U, Alignment>;
			};

			constexpr AlignedAllocator() noexcept = default;
			constexpr AlignedAllocator(const AlignedAllocator &other) noexcept = default;
			template<typename U>
			constexpr AlignedAllocator(const AlignedAllocator<U, Alignment> &other) noexcept
			{
			}
			constexpr std::align_val_t get_alignment() const noexcept
			{
				return std::align_val_t(Alignment);
			}
			[[nodiscard]] T* allocate(std::size_t n)
			{
				if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
					throw std::bad_array_new_length();
				return reinterpret_cast<T*>(::operator new(n * sizeof(T), get_alignment()));
			}
			void deallocate(T *p, [[maybe_unused]] std::size_t n)
			{
				::operator delete(p, get_alignment());
			}
	};
	template<class T, std::size_t Alignment>
	bool operator==(const AlignedAllocator<T, Alignment> &lhs, const AlignedAllocator<T, Alignment> &rhs)
	{
		return true;
	}
	template<class T, std::size_t Alignment>
	bool operator!=(const AlignedAllocator<T, Alignment> &lhs, const AlignedAllocator<T, Alignment> &rhs)
	{
		return not (lhs == rhs);
	}

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_ALIGNEDALLOCATOR_HPP_ */
