/*
 * Spinlock.hpp
 *
 *  Created on: Mar 2, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_SPINLOCK_HPP_
#define ALPHAGOMOKU_UTILS_SPINLOCK_HPP_

#include <atomic>

namespace ag
{

	class alignas(64) SpinLock
	{
			std::atomic<bool> lock_ = { 0 };
			uint8_t padding[64 - sizeof(std::atomic<bool>)];
		public:
			void lock() noexcept
			{
				for (;;)
				{
					if (not lock_.exchange(true, std::memory_order_acquire))
						return;
					while (lock_.load(std::memory_order_relaxed))
						__builtin_ia32_pause();
				}
			}
			bool try_lock() noexcept
			{
				return not lock_.load(std::memory_order_relaxed) and not lock_.exchange(true, std::memory_order_acquire);
			}
			void unlock() noexcept
			{
				lock_.store(false, std::memory_order_release);
			}
	};

	class SpinLockGuard
	{
			SpinLock &ref;
		public:
			SpinLockGuard(SpinLock &lock) noexcept :
					ref(lock)
			{
				ref.lock();
			}
			SpinLockGuard(const SpinLockGuard &other) noexcept = delete;
			SpinLockGuard& operator=(const SpinLockGuard &other) noexcept = delete;
			~SpinLockGuard() noexcept
			{
				ref.unlock();
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_SPINLOCK_HPP_ */
