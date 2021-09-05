/*
 * TimeManager.hpp
 *
 *  Created on: Sep 5, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_
#define ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_

#include <mutex>

namespace ag
{
	class EngineSettings;
} /* namespace ag */

namespace ag
{
	enum class OpeningType
	{

	};

	class TimeManager
	{
		private:
			static constexpr double TIME_FRACTION = 0.04; /**< fraction of the remaining time that is used every move */
			static constexpr double SWAP2_FRACTION = 0.1;
			const EngineSettings &engine_settings;

			mutable std::mutex mutex;
			double start_time = 0.0; /**< [seconds] */
			double stop_time = 0.0; /**< [seconds] */
			double used_time = 0.0; /**< [seconds] */

			bool is_search_running = false;

		public:
			TimeManager(const EngineSettings &settings);

			/**
			 * @brief Setup the time manager for general search.
			 */
			void setup() noexcept;
			/**
			 * @brief Setup the time manager for fixed time search.
			 */
			void setup(double time) noexcept;
			/**
			 * @brief Setup the time manager for specific phase of an opening.
			 */
			void setup(OpeningType opening, int phase) noexcept;
			/**
			 * @brief Starts measuring elapsed time.
			 */
			void startTimer() noexcept;
			/**
			 * @brief Stops the timer.
			 *
			 * Is needed to be able to get elapsed time after the search ended.
			 */
			void stopTimer() noexcept;
			/**
			 * @brief Decides whether the search should be continued.
			 */
			bool shouldTheSearchContinue() const noexcept;
			/**
			 * @brief Returns time spent on the search.
			 *
			 * If the search is not running at the point of calling this method, the time of previous search is returned.
			 */
			double getElapsedTime() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_ */
