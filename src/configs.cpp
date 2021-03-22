/*
 * configs.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: maciek
 */

#include <alphagomoku/configs.hpp>
#include <libml/utils/json.hpp>

namespace ag
{
	GameConfig::GameConfig(const Json &cfg) :
			rules(static_cast<GameRules>(static_cast<int>(cfg["rules"]))),
			rows(static_cast<int>(cfg["rows"])),
			cols(static_cast<int>(cfg["cols"]))
	{
	}

	GeneratorConfig::GeneratorConfig(const Json &cfg) :
			simulations(cfg["simulations"]),
			temperature(cfg["remperature"]),
			use_opening(cfg["use_opening"])
	{
	}

	TreeConfig::TreeConfig(const Json &cfg) :
			max_number_of_nodes(cfg["max_number_of_nodes"]),
			bucket_size(cfg["bucket_size"])
	{
	}

	CacheConfig::CacheConfig(const Json &cfg) :
			min_cache_size(cfg["min_cache_size"]),
			max_cache_size(cfg["max_cache_size"]),
			update_from_search(cfg["update_from_search"]),
			update_visit_treshold(cfg["update_visit_treshold"])
	{
	}

	SearchConfig::SearchConfig(const Json &cfg) :
			batch_size(cfg["batch_size"]),
			exploration_constant(cfg["exploration_constant"]),
			expansion_prior_treshold(cfg["expansion_prior_treshold"]),
			noise_weight(cfg["noise_weight"]),
			use_endgame_solver(cfg["use_endgame_solver"]),
			augment_position(cfg["augment_position"])
	{
	}
} /* namespaec ag */

