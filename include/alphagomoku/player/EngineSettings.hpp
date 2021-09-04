/*
 * EngineSettings.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_
#define ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_

#include <alphagomoku/utils/configs.hpp>

#include <inttypes.h>
#include <mutex>

namespace ag
{
	struct Option;
} /* namespace ag */

namespace ag
{

	class EngineSettings
	{
		private:
			static constexpr double max_double_value = 9007199254740992;
			mutable std::mutex mutex;
			const double TIME_FRACTION = 0.04; /**< fraction of time_left that is used every move */
			const double SWAP2_FRACTION = 0.1;

			const double PROTOCOL_OVERHEAD = 0.4; /**< [seconds] lag between sending a message from program and receiving it by GUI   */

			GameConfig game_config;
			int64_t max_memory = 256 * 1024 * 1024; /**< [bytes] initially set to 256MB */
			double time_for_match = max_double_value; /**< [seconds] */
			double time_for_turn = 5.0; /**< [seconds] */
			double time_left = max_double_value; /**< [seconds] */
			double search_start_time = 0.0; /**< [seconds] */
			double time_for_pondering = 0.0; /**< [seconds] */

			bool analysis_mode = false;
		public:
			EngineSettings();

			void setRows(int rows) noexcept;
			void setCols(int cols) noexcept;
			void setRules(GameRules rules) noexcept;
			GameConfig getGameConfig() const noexcept;

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

			bool setOption(const Option &option) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_ */
