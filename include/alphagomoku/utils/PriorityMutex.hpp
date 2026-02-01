/*
 * PriorityMutex.hpp
 *
 *  Created on: Aug 12, 2025
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_UTILS_PRIORITYMUTEX_HPP_
#define INCLUDE_ALPHAGOMOKU_UTILS_PRIORITYMUTEX_HPP_

#include <mutex>

namespace ag
{
	class PriorityMutex
	{
			std::mutex data_mutex;
			std::mutex need_to_access_mutex;
			std::mutex low_priority_mutex;
		public:
			void high_priority_lock()
			{
				std::lock_guard<std::mutex> lock(need_to_access_mutex);
				data_mutex.lock();
			}
			void high_priority_unlock()
			{
				data_mutex.unlock();
			}
			void low_priority_lock()
			{
				low_priority_mutex.lock();
				std::lock_guard<std::mutex> lock(need_to_access_mutex);
				data_mutex.lock();
			}
			void low_priority_unlock()
			{
				data_mutex.unlock();
				low_priority_mutex.unlock();
			}
	};

	class HighPriorityLock
	{
			PriorityMutex &priority_mutex;
		public:
			HighPriorityLock(PriorityMutex &pm) :
					priority_mutex(pm)
			{
				priority_mutex.high_priority_lock();
			}
			~HighPriorityLock()
			{
				priority_mutex.high_priority_unlock();
			}
	};

	class LowPriorityLock
	{
			PriorityMutex &priority_mutex;
		public:
			LowPriorityLock(PriorityMutex &pm) :
					priority_mutex(pm)
			{
				priority_mutex.low_priority_lock();
			}
			~LowPriorityLock()
			{
				priority_mutex.low_priority_unlock();
			}
	};

} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_UTILS_PRIORITYMUTEX_HPP_ */
