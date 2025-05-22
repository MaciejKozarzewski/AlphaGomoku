/*
 * TimeManager.cpp
 *
 *  Created on: Sep 5, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <iostream>

namespace
{
	ag::MovesLeftEstimator get_freestyle_estimator()
	{
		std::vector<std::pair<int, float>> c0;
		c0.push_back( { 0, 60 });
		c0.push_back( { 20, 53 });
		c0.push_back( { 350, 50 });
		c0.push_back( { 400, 0 });

		std::vector<std::pair<int, float>> c2;
		c2.push_back( { 0, 200 });
		c2.push_back( { 20, 180 });
		c2.push_back( { 349, 180 });
		c2.push_back( { 350, 0 });

		return ag::MovesLeftEstimator(c0, c2);
	}
	ag::MovesLeftEstimator get_standard_estimator()
	{
		std::vector<std::pair<int, float>> c0;
		c0.push_back( { 0, 85 });
		c0.push_back( { 15, 85 });
		c0.push_back( { 65, 135 });
		c0.push_back( { 80, 135 });
		c0.push_back( { 100, 125 });
		c0.push_back( { 225, 0 });

		std::vector<std::pair<int, float>> c2;
		c2.push_back( { 0, 320 });
		c2.push_back( { 20, 320 });
		c2.push_back( { 65, 525 });
		c2.push_back( { 80, 525 });
		c2.push_back( { 125, 375 });
		c2.push_back( { 140, 0 });

		return ag::MovesLeftEstimator(c0, c2);
	}
}

namespace ag
{

	MovesLeftEstimator::MovesLeftEstimator(const std::vector<std::pair<int, float>> &c0, const std::vector<std::pair<int, float>> &c2) :
			c0(c0, "linear"),
			c2(c2, "linear")
	{
	}
	double MovesLeftEstimator::get(int moveNumber, Value eval) const noexcept
	{
		const double _c0 = c0.getValue(moveNumber);
		const double _c2 = c2.getValue(moveNumber);
		const double x = std::abs(eval.getExpectation() - 0.5);
		return std::max(1.0, _c0 - _c2 * square(x));
	}

	TimeManager::TimeManager()
	{
		moves_left_estimators.insert( { GameRules::FREESTYLE, get_freestyle_estimator() });
		moves_left_estimators.insert( { GameRules::STANDARD, get_standard_estimator() });
		moves_left_estimators.insert( { GameRules::RENJU, get_standard_estimator() });
		moves_left_estimators.insert( { GameRules::CARO5, get_standard_estimator() });
		moves_left_estimators.insert( { GameRules::CARO6, get_standard_estimator() });
	}
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
		start_time = getTime();
	}
	void TimeManager::stopTimer() noexcept
	{
		std::lock_guard lock(mutex);
		used_time += getTime() - start_time;
		is_running = false;
	}
	double TimeManager::getElapsedTime() const noexcept
	{
		std::lock_guard lock(mutex);
		if (is_running)
			return used_time + getTime() - start_time;
		else
			return used_time;
	}
	double TimeManager::getLastSearchTime() const noexcept
	{
		std::lock_guard lock(mutex);
		return time_of_last_search;
	}

	double TimeManager::getTimeForTurn(const EngineSettings &settings, int moveNumber, float moves_left)
	{
		std::lock_guard lock(mutex);
		const double fraction = 0.9;
		const double sum = (1.0 - std::pow(fraction, moves_left)) / std::max(1.0e-8, 1.0 - fraction);

		return std::min(settings.getTimeForTurn(), (settings.getTimeLeft() / sum)) - settings.getProtocolLag();
	}
	double TimeManager::getTimeForOpening(const EngineSettings &settings)
	{
		std::lock_guard lock(mutex);
		return std::min(settings.getTimeForTurn(), (SWAP2_FRACTION * settings.getTimeLeft())) - settings.getProtocolLag();
	}

} /* namespace ag */

