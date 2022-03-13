/*
 * configs.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/configs.hpp>

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
			initial_cache_size(get_value<int>(cfg, "cache_size", Defaults::initial_cache_size)),
			bucket_size(get_value<int>(cfg, "bucket_size", Defaults::bucket_size))
	{
	}
	Json TreeConfig::toJson() const
	{
		return Json( { { "cache_size", initial_cache_size }, { "bucket_size", bucket_size } });
	}

	SearchConfig::SearchConfig(const Json &cfg) :
			max_batch_size(get_value<int>(cfg, "max_batch_size", Defaults::max_batch_size)),
			exploration_constant(get_value<float>(cfg, "exploration_constant", Defaults::exploration_constant)),
			expansion_prior_treshold(get_value<float>(cfg, "expansion_prior_treshold", Defaults::expansion_prior_treshold)),
			max_children(get_value<int>(cfg, "max_children", Defaults::max_children)),
			noise_weight(get_value<float>(cfg, "noise_weight", Defaults::noise_weight)),
			vcf_solver_level(get_value<int>(cfg, "vcf_solver_level", Defaults::vcf_solver_level)),
			vcf_solver_max_positions(get_value(cfg, "vcf_solver_max_positions", Defaults::vcf_solver_max_positions)),
			style_factor(get_value<int>(cfg, "style_factor", Defaults::style_factor))
	{
	}
	Json SearchConfig::toJson() const
	{
		return Json(
				{ { "max_batch_size", max_batch_size }, { "exploration_constant", exploration_constant }, { "expansion_prior_treshold",
						expansion_prior_treshold }, { "max_children", max_children }, { "noise_weight", noise_weight }, { "vcf_solver_level",
						vcf_solver_level }, { "vcf_solver_max_positions", vcf_solver_max_positions }, { "style_factor", style_factor } });
	}

	DeviceConfig::DeviceConfig(const Json &cfg) :
			device(ml::Device::fromString(cfg["device"])),
			batch_size(get_value<int>(cfg, "batch_size", Defaults::batch_size)),
			omp_threads(get_value<int>(cfg, "omp_threads", Defaults::omp_threads))
	{
	}
	Json DeviceConfig::toJson() const
	{
		return Json( { { "device", device.toString() }, { "batch_size", batch_size }, { "omp_threads", omp_threads } });
	}

	TrainingConfig::TrainingConfig(const Json &options) :
			augment_training_data(get_value<bool>(options, "augment_training_data")),
			device_config(options["device_config"]),
			steps_per_iteration(get_value<int>(options, "steps_per_iteration")),
			blocks(get_value<int>(options, "blocks")),
			filters(get_value<int>(options, "filters")),
			l2_regularization(get_value<double>(options, "l2_regularization")),
			validation_percent(get_value<double>(options, "validation_percent"))
	{
		for (int i = 0; i < options["learning_rate_schedule"].size(); i++)
		{
			int first = options["learning_rate_schedule"][i]["from_epoch"];
			double second = options["learning_rate_schedule"][i]["value"];
			learning_rate_schedule.push_back( { first, second });
		}
		for (int i = 0; i < options["buffer_size"].size(); i++)
		{
			int first = options["buffer_size"][i]["from_epoch"];
			int second = options["buffer_size"][i]["size"];
			buffer_size.push_back( { first, second });
		}
	}
	Json TrainingConfig::toJson() const
	{
		Json result;
		result["augment_training_data"] = augment_training_data;
		result["device_config"] = device_config.toJson();
		result["steps_per_iteration"] = steps_per_iteration;
		result["blocks"] = blocks;
		result["filters"] = filters;
		result["l2_regularization"] = l2_regularization;
		result["validation_percent"] = validation_percent;
		for (size_t i = 0; i < learning_rate_schedule.size(); i++)
			result["learning_rate_schedule"][i] = Json(
					{ { "from_epoch", learning_rate_schedule[i].first }, { "value", learning_rate_schedule[i].second } });
		for (size_t i = 0; i < buffer_size.size(); i++)
			result["buffer_size"][i] = Json( { { "from_epoch", buffer_size[i].first }, { "size", buffer_size[i].second } });
		return result;
	}

	SelfplayConfig::SelfplayConfig(const Json &options) :
			use_opening(get_value<bool>(options, "use_opening")),
			use_symmetries(get_value<bool>(options, "use_symmetries")),
			games_per_iteration(get_value<int>(options, "games_per_iteration")),
			games_per_thread(get_value<int>(options, "games_per_thread")),
			simulations_min(get_value<int>(options, "simulations_min")),
			simulations_max(get_value<int>(options, "simulations_max", simulations_min)),
			positions_skip(get_value<int>(options, "positions_skip", 1)),
			device_config(),
			search_config(options["search_config"]),
			tree_config(options["tree_config"])
	{
		for (int i = 0; i < options["device_config"].size(); i++)
			device_config.push_back(DeviceConfig(options["device_config"][i]));
	}
	Json SelfplayConfig::toJson() const
	{
		Json result;
		result["use_opening"] = use_opening;
		result["use_symmetries"] = use_symmetries;
		result["games_per_iteration"] = games_per_iteration;
		result["games_per_thread"] = games_per_thread;
		result["simulations_min"] = simulations_min;
		result["simulations_max"] = simulations_max;
		result["positions_skip"] = positions_skip;
		for (size_t i = 0; i < device_config.size(); i++)
			result["device_config"][i] = device_config[i].toJson();
		result["search_config"] = search_config.toJson();
		result["tree_config"] = tree_config.toJson();
		return result;
	}

	EvaluationConfig::EvaluationConfig(const Json &options) :
			use_evaluation(get_value<bool>(options, "use_evaluation")),
			use_gating(get_value<bool>(options, "use_gating")),
			gating_threshold(get_value<double>(options, "gating_threshold")),
			selfplay_options(options["selfplay_options"])
	{
	}
	Json EvaluationConfig::toJson() const
	{
		Json result;
		result["use_evaluation"] = use_evaluation;
		result["use_gating"] = use_gating;
		result["gating_threshold"] = gating_threshold;
		result["selfplay_options"] = selfplay_options.toJson();
		return result;
	}

	MasterLearningConfig::MasterLearningConfig(const Json &options) :
			description(get_value(options, "description", std::string())),
			data_type(get_value(options, "data_type", std::string("games"))),
			game_config(options["game_config"]),
			training_config(options["training_config"]),
			generation_config(options["generation_config"]),
			evaluation_config(options["evaluation_config"])
	{
	}
	Json MasterLearningConfig::toJson() const
	{
		Json result;
		result["description"] = description;
		result["data_type"] = data_type;
		result["game_config"] = game_config.toJson();
		result["training_config"] = training_config.toJson();
		result["generation_config"] = generation_config.toJson();
		result["evaluation_config"] = evaluation_config.toJson();
		return result;
	}

} /* namespaec ag */

