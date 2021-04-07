/*
 * configs.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <libml/utils/json.hpp>

namespace ag
{
	GameConfig::GameConfig(int rows, int cols) :
			rows(rows),
			cols(cols)
	{
	}
	GameConfig::GameConfig(GameRules rules) :
			rules(rules)
	{
	}
	GameConfig::GameConfig(GameRules rules, int rows, int cols) :
			rules(rules),
			rows(rows),
			cols(cols)
	{
	}
	GameConfig::GameConfig(const Json &cfg) :
			rules(rulesFromString(cfg["rules"])),
			rows(static_cast<int>(cfg["rows"])),
			cols(static_cast<int>(cfg["cols"]))
	{
	}
	Json GameConfig::getDefault()
	{
		return Json( { { "rules", toString(GameRules::FREESTYLE) }, { "rows", 0 }, { "cols", 0 } });
	}

	TreeConfig::TreeConfig(const Json &cfg) :
			max_number_of_nodes(cfg["max_number_of_nodes"]),
			bucket_size(cfg["bucket_size"])
	{
	}
	Json TreeConfig::getDefault()
	{
		return Json( { { "max_number_of_nodes", 5000000 }, { "bucket_size", 100000 } });
	}

	CacheConfig::CacheConfig(const Json &cfg) :
			min_cache_size(cfg["min_cache_size"]),
			max_cache_size(cfg["max_cache_size"]),
			update_from_search(cfg["update_from_search"]),
			update_visit_treshold(cfg["update_visit_treshold"])
	{
	}
	Json CacheConfig::getDefault()
	{
		return Json( { { "min_cache_size", 8192 }, { "max_cache_size", 1048576 }, { "update_from_search", false }, { "update_visit_treshold", 10 } });
	}

	SearchConfig::SearchConfig(const Json &cfg) :
			batch_size(cfg["batch_size"]),
			exploration_constant(cfg["exploration_constant"]),
			expansion_prior_treshold(cfg["expansion_prior_treshold"]),
			noise_weight(cfg["noise_weight"]),
			use_endgame_solver(cfg["use_endgame_solver"])
	{
	}
	Json SearchConfig::getDefault()
	{
		return Json( { { "batch_size", 1 }, { "exploration_constant", 1.25 }, { "expansion_prior_treshold", 1.0e-6 }, { "noise_weight", 0.0 }, {
				"use_endgame_solver", false } });
	}

	Json getDefaultTrainingConfig()
	{
		return Json::load("{"
				"\"device\": \"CPU\","
				"\"batch_size\": 128,"
				"\"blocks\": 5,"
				"\"filters\": 64,"
				"\"l2_regularization\": 1.0e-4,"
				"\"learning_rate_schedule\": [{\"from_epoch\": 0, \"value\": 1.0e-3}],"
				"\"augment_training_data\": true,"
				"\"validation_percent\": 5,"
				"\"steps_per_interation\": 1000,"
				"\"buffer_size\": 20}");
	}
	Json getDefaultSelfplayConfig()
	{
		Json result = Json::load("{"
				"\"games_per_iteration\": 1000,"
				"\"games_per_thread\": 1,"
				"\"simulations\": 100,"
				"\"use_opening\": true,"
				"\"temperature\": 0.0,"
				"\"use_symmetries\": \"false\""
				"\"threads\": [{\"device\": \"CPU\"}]}");
		result["search_options"] = SearchConfig::getDefault();
		result["search_options"]["noise_weight"] = 0.25;
		result["tree_options"] = TreeConfig::getDefault();
		result["cache_options"] = CacheConfig::getDefault();
		return result;
	}
	Json getDefaultEvaluationConfig()
	{
		Json result = Json::load("{"
				"\"use_evaluation\": false,"
				"\"games_per_iteration\": 240,"
				"\"games_per_thread\": 1,"
				"\"simulations\": 100,"
				"\"use_opening\": true,"
				"\"temperature\": 0.0,"
				"\"use_symmetries\": \"false\""
				"\"threads\": [{\"device\": \"CPU\"}]}");
		result["search_options"] = SearchConfig::getDefault();
		result["tree_options"] = TreeConfig::getDefault();
		result["cache_options"] = CacheConfig::getDefault();
		return result;
	}

} /* namespaec ag */

