/*
 * configs.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_CONFIGS_HPP_
#define ALPHAGOMOKU_CONFIGS_HPP_

#include <alphagomoku/utils/game_rules.hpp>

class Json;
namespace ag
{
	struct GameConfig
	{
			GameRules rules = GameRules::FREESTYLE;
			int rows = 0;
			int cols = 0;

			GameConfig() = default;
			GameConfig(const Json &cfg);
	};

	struct GeneratorConfig
	{
			int simulations = 0;
			float temperature = 0.0f;
			bool use_opening = false;

			GeneratorConfig() = default;
			GeneratorConfig(const Json &cfg);
	};

	struct TreeConfig
	{
			int max_number_of_nodes = 5000000;
			int bucket_size = 100000;

			TreeConfig() = default;
			TreeConfig(const Json &cfg);
	};

	struct CacheConfig
	{
			int min_cache_size = 8192;
			int max_cache_size = 1048576;
			int update_from_search = false;
			int update_visit_treshold = 10;

			CacheConfig() = default;
			CacheConfig(const Json &cfg);
	};

	struct SearchConfig
	{
			int batch_size = 1;
			float exploration_constant = 1.25f;
			float expansion_prior_treshold = 0.0f;
			float noise_weight = 0.0f;
			bool use_endgame_solver = false;
			bool augment_position = false;

			SearchConfig() = default;
			SearchConfig(const Json &cfg);
	};
}

#endif /* ALPHAGOMOKU_CONFIGS_HPP_ */
