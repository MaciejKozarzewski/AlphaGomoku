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

	TimeManager::TimeManager(const EngineSettings &settings) :
			engine_settings(settings)
	{
	}
	void TimeManager::setup() noexcept
	{
		std::lock_guard lock(mutex);
		used_time = 0.0;
	}
	void TimeManager::setup(double time) noexcept
	{
		std::lock_guard lock(mutex);
		used_time = 0.0;
	}
	void TimeManager::setup(OpeningType opening, int phase) noexcept
	{
		std::lock_guard lock(mutex);
		used_time = 0.0;
	}
	void TimeManager::startTimer() noexcept
	{
		std::lock_guard lock(mutex);
		start_time = get_current_time();
		is_search_running = true;
	}
	void TimeManager::stopTimer() noexcept
	{
		std::lock_guard lock(mutex);
		used_time += get_current_time() - start_time;
		is_search_running = false;
	}
	bool TimeManager::shouldTheSearchContinue() const noexcept
	{
		std::lock_guard lock(mutex);
		return true;
	}
	double TimeManager::getElapsedTime() const noexcept
	{
		std::lock_guard lock(mutex);
		if (is_search_running)
			return get_current_time() - start_time;
		else
			return used_time;
	}
} /* namespace ag */

