/*
 * EngineSettings.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_
#define ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_

#include <alphagomoku/utils/configs.hpp>

#include <cinttypes>
#include <vector>
#include <mutex>

class Json;
namespace ag
{
	struct Option;
	struct GameConfig;
	class Move;
} /* namespace ag */

namespace ag
{
	enum class SetOptionOutcome
	{
		SUCCESS,
		SUCCESS_BUT_REALLOCATE_ENGINE,
		FAILURE
	};

	class EngineSettings
	{
		private:
			static constexpr double max_double_value = 9007199254740992.0;
			static constexpr int max_int_value = std::numeric_limits<int>::max();

			mutable std::mutex mutex;
			std::vector<std::pair<GameConfig, std::string>> path_to_conv_networks;
			std::vector<std::pair<GameConfig, std::string>> path_to_nnue_networks;
			std::vector<std::vector<Move>> swap2_openings;

			GameConfig game_config;
			SearchConfig search_config;

			double time_for_match = max_double_value; /**< [seconds] */
			double time_left = max_double_value; /**< [seconds] */
			double time_for_turn = 5.0; /**< [seconds] */
			double time_increment = 0.0; /**< [seconds] */
			double protocol_lag = 0.1; /**< [seconds] lag between sending a message from program and receiving it by GUI */
			double time_for_pondering = 0.0; /**< [seconds] */

			int max_depth = max_int_value;
			int max_nodes = max_int_value;
			int thread_num = 1;
			int64_t max_memory = 256 * 1024 * 1024; /**< [bytes] initially set to 256MB */

			bool analysis_mode = false;
			bool auto_pondering = false;
			bool use_symmetries = true;
			bool use_database = false;

			std::vector<DeviceConfig> device_configs;
		public:
			EngineSettings(const Json &config);

			SetOptionOutcome setOption(const Option &option) noexcept;

			std::string getPathToConvNetwork() const;
			std::string getPathToNnueNetwork() const;
			const std::vector<std::vector<Move>>& getSwap2Openings() const;
			const GameConfig& getGameConfig() const noexcept;
			const SearchConfig& getSearchConfig() const noexcept;

			double getTimeForMatch() const noexcept;
			double getTimeLeft() const noexcept;
			double getTimeForTurn() const noexcept;
			double getTimeIncrement() const noexcept;
			double getProtocolLag() const noexcept;
			double getTimeForPondering() const noexcept;
			int getMaxDepth() const noexcept;
			int getMaxNodes() const noexcept;
			int getThreadNum() const noexcept;
			int64_t getMaxMemory() const noexcept;
			bool isInAnalysisMode() const noexcept;
			bool isUsingAutoPondering() const noexcept;
			bool isUsingSymmetries() const noexcept;

			const std::vector<DeviceConfig>& getDeviceConfigs() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_ */
