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
#include <vector>
#include <map>
#include <mutex>

class Json;
namespace ag
{
	struct Option;
	enum class GameRules;
} /* namespace ag */

namespace ag
{
	enum class EngineStyle
	{
		DEFENSIVE, /**< search maximizes probability of not losing */
		CLASSIC, /**< search maximizes expectation outcome */
		OFFENSIVE /**< search maximizes probability of winning */
	};
	std::string toString(EngineStyle es);
	EngineStyle engineStyleFromString(const std::string &str);

	enum class SetOptionOutcome
	{
		SUCCESS,
		SUCCESS_BUT_REALLOCATE_ENGINE,
		FAILURE
	};

	class EngineSettings
	{
		private:
			static constexpr double max_double_value = 9007199254740992;
			static constexpr int max_int_value = std::numeric_limits<int>::max();

			mutable std::mutex mutex;
			std::map<GameRules, std::string> path_to_networks;

			GameConfig game_config;
			TreeConfig tree_config;
			CacheConfig cache_config;
			SearchConfig search_config;

			double time_for_match = max_double_value; /**< [seconds] */
			double time_left = max_double_value; /**< [seconds] */
			double time_for_turn = 5.0; /**< [seconds] */
			double time_increment = 0.0; /**< [seconds] */
			double protocol_lag = 0.4; /**< [seconds] lag between sending a message from program and receiving it by GUI   */

			int max_depth = max_int_value;
			int max_nodes = max_int_value;
			int thread_num = 1;
			int64_t max_memory = 256 * 1024 * 1024; /**< [bytes] initially set to 256MB */

			bool analysis_mode = false;
			bool auto_pondering = false;
			bool use_symmetries = true;

			std::vector<DeviceConfig> device_configs;

		public:
			EngineSettings(const Json &config);

			SetOptionOutcome setOption(const Option &option) noexcept;

			std::string getPathToNetwork() const;
			const GameConfig& getGameConfig() const noexcept;
			const TreeConfig& getTreeConfig() const noexcept;
			const CacheConfig& getCacheConfig() const noexcept;
			const SearchConfig& getSearchConfig() const noexcept;

			double getTimeForMatch() const noexcept;
			double getTimeLeft() const noexcept;
			double getTimeForTurn() const noexcept;
			double getTimeIncrement() const noexcept;
			double getProtocolLag() const noexcept;
			int getMaxDepth() const noexcept;
			int getMaxNodes() const noexcept;
			int getThreadNum() const noexcept;
			int64_t getMaxMemory() const noexcept;
			float getStyleFactor() const noexcept;
			bool isInAnalysisMode() const noexcept;
			bool isUsingAutoPondering() const noexcept;
			bool isUsingSymmetries() const noexcept;

			const std::vector<DeviceConfig>& getDeviceConfigs() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_ */
