/*
 * TimeManager.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_
#define ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_

#include <inttypes.h>

namespace ag
{

	class TimeManager
	{
			const double TIME_FRACTION = 0.05;
			const double SWAP2_FRACTION = 0.1;

			const uint64_t PROTOCOL_OVERHEAD = 400; // [milliseconds]
		public:
			uint64_t time_match = 0; // [milliseconds]
			uint64_t time_start = 0; // [milliseconds]
			uint64_t time_left = 5000; // [milliseconds]
			uint64_t timeout_turn = 5000; // [milliseconds]

			double getTimeForTurn() const noexcept;
			double getTimeForSwap2(int stage) const noexcept;
			double elapsedTime() const noexcept;
			void setTimeStart(double t) noexcept;
			void setTimeMatch(uint64_t t) noexcept;
			void setTimeLeft(uint64_t t) noexcept;
			void setTimeoutTurn(uint64_t t) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_TIMEMANAGER_HPP_ */
