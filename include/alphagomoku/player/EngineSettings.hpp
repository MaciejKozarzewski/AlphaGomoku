/*
 * EngineSettings.hpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_
#define ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_

#include <alphagomoku/utils/configs.hpp>
#include <libml/hardware/Device.hpp>

#include <inttypes.h>
#include <mutex>

namespace ag
{
	struct Option;
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

	class EngineSettings
	{
		private:
			static constexpr double max_double_value = 9007199254740992;
			static constexpr int max_int_value = std::numeric_limits<int>::max();

			mutable std::mutex mutex;
			/* following settings can be changed by GUI */
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
			uint64_t max_memory = 256 * 1024 * 1024; /**< [bytes] initially set to 256MB */
			EngineStyle style = EngineStyle::CLASSIC;

			bool analysis_mode = false;
			bool auto_pondering = false;
			bool use_symmetries = true;

			/* following settings cannot be changed after engine has initialized */
			std::vector<ml::Device> devices;

		public:
			EngineSettings();
			bool setOption(const Option &option) noexcept;

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
			uint64_t getMaxMemory() const noexcept;
			EngineStyle getStyle() const noexcept;
			bool isInAnalysisMode() const noexcept;
			bool isUsingAutoPondering() const noexcept;

			bool isUsingSymmetries() const noexcept;
			const std::vector<ml::Device>& getDevices() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINESETTINGS_HPP_ */
