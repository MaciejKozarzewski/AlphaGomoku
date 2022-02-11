/*
 * TimeManager.cpp
 *
 *  Created on: Sep 5, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/TimeManager.hpp>

namespace
{
	double get_current_time() noexcept
	{
		return std::chrono::steady_clock::now().time_since_epoch().count() * 1.0e-9;
	}
}

namespace ag
{

	void TimeManager::resetTimer() noexcept
	{
		std::lock_guard lock(mutex);
		is_running = false;
		time_of_last_search = used_time;
		used_time = 0.0;
	}
	void TimeManager::startTimer() noexcept
	{
		std::lock_guard lock(mutex);
		is_running = true;
		start_time = get_current_time();
	}
	void TimeManager::stopTimer() noexcept
	{
		std::lock_guard lock(mutex);
		used_time += get_current_time() - start_time;
		is_running = false;
	}
	double TimeManager::getElapsedTime() const noexcept
	{
		std::lock_guard lock(mutex);
		if (is_running)
			return used_time + get_current_time() - start_time;
		else
			return used_time;
	}
	double TimeManager::getLastSearchTime() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_of_last_search;
	}
} /* namespace ag */

