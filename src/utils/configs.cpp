/*
 * configs.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/mcts/Node.hpp>

#include <libml/utils/json.hpp>

namespace
{
	template<typename T>
	T get_value(const Json &json, const std::string &key)
	{
		if (json.contains(key))
			return static_cast<T>(json[key]);
		else
			throw std::runtime_error("Missing parameter '" + key + "'");
	}
	template<typename T>
	T get_value(const Json &json, const std::string &key, T default_value)
	{
		if (json.contains(key))
			return static_cast<T>(json[key]);
		else
			return default_value;
	}
}

namespace ag
{

	GameConfig::GameConfig(GameRules rules, int rows, int cols) :
			rules(rules),
			rows(rows),
			cols(cols)
	{
	}
	GameConfig::GameConfig(GameRules rules, int size) :
			rules(rules),
			rows(size),
			cols(size)
	{
	}
	GameConfig::GameConfig(const Json &cfg) :
			rules(rulesFromString(get_value<std::string>(cfg, "rules"))),
			rows(get_value<int>(cfg, "rows")),
			cols(get_value<int>(cfg, "cols"))
	{
	}
	Json GameConfig::toJson() const
	{
		return Json( { { "rules", toString(rules) }, { "rows", rows }, { "cols", cols } });
	}

	TreeConfig::TreeConfig(const Json &cfg) :
			bucket_size(get_value<int>(cfg, "bucket_size", Defaults::bucket_size))
	{
	}
	Json TreeConfig::toJson() const
	{
		return Json( { { "bucket_size", bucket_size } });
	}

	CacheConfig::CacheConfig(const Json &cfg) :
			cache_size(get_value<int>(cfg, "cache_size", Defaults::cache_size))
	{
	}
	Json CacheConfig::toJson() const
	{
		return Json( { { "cache_size", cache_size } });
	}

	SearchConfig::SearchConfig(const Json &cfg) :
			max_batch_size(get_value<int>(cfg, "max_batch_size", Defaults::max_batch_size)),
			exploration_constant(get_value<float>(cfg, "exploration_constant", Defaults::exploration_constant)),
			expansion_prior_treshold(get_value<float>(cfg, "expansion_prior_treshold", Defaults::expansion_prior_treshold)),
			max_children(get_value<int>(cfg, "max_children", Defaults::max_children)),
			noise_weight(get_value<float>(cfg, "noise_weight", Defaults::use_endgame_solver)),
			use_endgame_solver(get_value<bool>(cfg, "use_endgame_solver", Defaults::use_endgame_solver)),
			use_vcf_solver(get_value<bool>(cfg, "use_vcf_solver", Defaults::use_vcf_solver))
	{
	}
	Json SearchConfig::toJson() const
	{
		return Json( { { "max_batch_size", max_batch_size }, { "exploration_constant", exploration_constant }, { "expansion_prior_treshold",
				expansion_prior_treshold }, { "max_children", max_children }, { "noise_weight", noise_weight }, { "use_endgame_solver",
				use_endgame_solver }, { "use_vcf_solver", use_vcf_solver } });
	}

	DeviceConfig::DeviceConfig(const Json &cfg) :
			device(ml::Device::fromString(cfg["device"])),
			batch_size(get_value<int>(cfg, "batch_size", Defaults::batch_size)),
			omp_threads(get_value<int>(cfg, "omp_threads", Defaults::omp_threads))
	{
	}
	Json DeviceConfig::toJson() const
	{
		return Json( { { "device", ml::Device::cpu().toString() }, { "batch_size", Defaults::batch_size }, { "omp_threads", Defaults::omp_threads } });
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
		result["search_options"] = SearchConfig().toJson();
		result["search_options"]["noise_weight"] = 0.25;
		result["tree_options"] = TreeConfig().toJson();
		result["cache_options"] = CacheConfig().toJson();
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
		result["search_options"] = SearchConfig().toJson();
		result["tree_options"] = TreeConfig().toJson();
		result["cache_options"] = CacheConfig().toJson();
		return result;
	}

} /* namespaec ag */

