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

	class TimeManager
	{
		private:
			mutable std::mutex mutex;
			const double TIME_FRACTION = 0.04; /**< fraction of time_left that is used every move */
			const double SWAP2_FRACTION = 0.1;

			const double PROTOCOL_OVERHEAD = 0.4; /**< [seconds] lag between sending a message from program and receiving it by GUI   */

			double search_start_time = 0.0; /**< [seconds] */
			double time_for_pondering = 0.0; /**< [seconds] */

		public:
			void setSearchStartTime(double t) noexcept;
			double getElapsedTime() const noexcept;

			uint64_t getMaxMemory() const noexcept;
			double getTimeForTurn() const noexcept;
			double getTimeForSwap2(int stones) const noexcept;
			double getTimeForPondering() const noexcept;

			void setMaxMemory(uint64_t m) noexcept;
			void setTimeForMatch(double t) noexcept;
			void setTimeForTurn(double t) noexcept;
			void setTimeLeft(double t) noexcept;
			void setTimeForPondering(double t) noexcept;

	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_ */
