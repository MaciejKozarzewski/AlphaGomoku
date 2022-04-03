/*
 * TimeManager.hpp
 *
 *  Created on: Sep 5, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_
#define ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_

#include <alphagomoku/utils/Parameter.hpp>

#include <map>
#include <mutex>

namespace ag
{
	class EngineSettings;
	struct Value;
	enum class GameRules;
} /* namespace ag */

namespace ag
{
	enum class OpeningType
	{

	};

	class MovesLeftEstimator
	{
			Parameter<float> c0;
			Parameter<float> c2;
		public:
			MovesLeftEstimator(const std::vector<std::pair<int, float>> &c0, const std::vector<std::pair<int, float>> &c2);
			double get(int moveNumber, Value eval) const noexcept;
	};

	class TimeManager
	{
		private:
			static constexpr double TIME_FRACTION = 0.04; /**< fraction of the remaining time that is used every move */
			static constexpr double SWAP2_FRACTION = 0.1;

			std::map<GameRules, MovesLeftEstimator> moves_left_estimators;
			mutable std::mutex mutex;

			double start_time = 0.0; /**< [seconds] */
			double stop_time = 0.0; /**< [seconds] */
			double used_time = 0.0; /**< [seconds] */

			double time_of_last_search = 0.0; /**< [seconds] */
			bool is_running = false;
		public:
			TimeManager();

			void resetTimer() noexcept;
			void startTimer() noexcept;
			void stopTimer() noexcept;
			double getElapsedTime() const noexcept;
			double getLastSearchTime() const noexcept;

			double getTimeForTurn(const EngineSettings &settings, int moveNumber, Value eval);
			double getTimeForOpening(const EngineSettings &settings);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_ */
