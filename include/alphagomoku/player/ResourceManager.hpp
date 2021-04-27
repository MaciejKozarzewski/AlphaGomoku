/*
 * TimeManager.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PLAYER_RESOURCEMANAGER_HPP_
#define ALPHAGOMOKU_PLAYER_RESOURCEMANAGER_HPP_

#include <inttypes.h>

namespace ag
{

	class ResourceManager
	{
		private:
			const double TIME_FRACTION = 0.05;
			const double SWAP2_FRACTION = 0.1;

			const double MIN_TIME_FOR_MOVE = 0.1; // [seconds]
			const double PROTOCOL_OVERHEAD = 0.4; // [seconds]

			uint64_t max_memory = 100 * 1024 * 1024; // [bytes] initially set to 100MB
			double time_for_match = 0.0; // [seconds]
			double time_for_turn = 5.0; // [seconds]
			double time_left = 5.0; // [seconds]
			double search_start_time = 0.0; // [seconds]
		public:

			void setSearchStartTime(double t) noexcept;
			double getElapsedTime() const noexcept;

			uint64_t getMaxMemory() const noexcept;
			double getTimeForTurn() const noexcept;
			double getTimeForSwap2(int stones) const noexcept;

			void setMaxMemory(uint64_t m) noexcept;
			void setTimeForMatch(double t) noexcept;
			void setTimeForTurn(double t) noexcept;
			void setTimeLeft(double t) noexcept;

	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_RESOURCEMANAGER_HPP_ */
