/*
 * configs.cpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/configs.hpp>

namespace
{
	template<typename T>
	T get_value(const Json &json, const std::string &key)
	{
		if (json.hasKey(key))
			return static_cast<T>(json[key]);
		else
			throw std::runtime_error("Missing parameter '" + key + "'");
	}
	template<typename T>
	T get_value(const Json &json, const std::string &key, T default_value)
	{
		if (json.hasKey(key))
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
			cols(cols),
			draw_after(rows * cols)
	{
	}
	GameConfig::GameConfig(GameRules rules, int size) :
			GameConfig(rules, size, size)
	{
	}
	GameConfig::GameConfig(const Json &cfg) :
			rules(rulesFromString(get_value<std::string>(cfg, "rules"))),
			rows(get_value<int>(cfg, "rows")),
			cols(get_value<int>(cfg, "cols")),
			draw_after(get_value<int>(cfg, "draw_after", rows * cols))
	{
	}
	Json GameConfig::toJson() const
	{
		return Json( { { "rules", toString(rules) }, { "rows", rows }, { "cols", cols }, { "draw_after", draw_after } });
	}

	TreeConfig::TreeConfig(const Json &cfg) :
			information_leak_threshold(get_value<float>(cfg, "information_leak_threshold", Defaults::information_leak_threshold)),
			initial_node_cache_size(get_value<int>(cfg, "initial_node_cache_size", Defaults::initial_node_cache_size)),
			edge_bucket_size(get_value<int>(cfg, "edge_bucket_size", Defaults::edge_bucket_size)),
			node_bucket_size(get_value<int>(cfg, "node_bucket_size", Defaults::node_bucket_size))
	{
	}
	Json TreeConfig::toJson() const
	{
		return Json( { { "information_leak_threshold", information_leak_threshold }, { "initial_node_cache_size", initial_node_cache_size }, {
				"edge_bucket_size", edge_bucket_size }, { "node_bucket_size", node_bucket_size } });
	}

	EdgeSelectorConfig::EdgeSelectorConfig(const Json &cfg) :
			policy(get_value<std::string>(cfg, "policy", "puct")),
			init_to(get_value<std::string>(cfg, "init_to", "parent")),
			noise_type(get_value<std::string>(cfg, "noise_type", "none")),
			noise_weight(get_value<float>(cfg, "noise_weight", Defaults::noise_weight)),
			exploration_constant(get_value<float>(cfg, "exploration_constant", Defaults::exploration_constant)),
			exploration_exponent(get_value<float>(cfg, "exploration_exponent", Defaults::exploration_exponent))
	{
	}
	Json EdgeSelectorConfig::toJson() const
	{
		return Json( { { "policy", policy }, { "init_to", init_to }, { "noise_type", noise_type }, { "noise_weight", noise_weight }, {
				"exploration_constant", exploration_constant }, { "exploration_exponent", exploration_exponent } });
	}

	MCTSConfig::MCTSConfig(const Json &cfg) :
			edge_selector_config(get_value<EdgeSelectorConfig>(cfg, "edge_selector_config", EdgeSelectorConfig())),
			max_children(get_value<int>(cfg, "max_children", Defaults::max_children)),
			policy_expansion_threshold(get_value<float>(cfg, "policy_expansion_threshold", Defaults::policy_expansion_threshold))
	{
	}
	Json MCTSConfig::toJson() const
	{
		return Json( { { "edge_selector_config", edge_selector_config.toJson() }, { "max_children", max_children }, { "policy_expansion_threshold",
				policy_expansion_threshold } });
	}

	TSSConfig::TSSConfig(const Json &cfg) :
			mode(get_value<int>(cfg, "mode", Defaults::mode)),
			max_positions(get_value(cfg, "max_positions", Defaults::max_positions)),
			hash_table_size(get_value<int>(cfg, "hash_table_size", Defaults::hash_table_size))
	{
	}
	Json TSSConfig::toJson() const
	{
		return Json( { { "mode", mode }, { "max_positions", max_positions }, { "hash_table_size", hash_table_size } });
	}

	SearchConfig::SearchConfig(const Json &cfg) :
			max_batch_size(get_value<int>(cfg, "max_batch_size", Defaults::max_batch_size)),
			tree_config(cfg["tree_config"]),
			mcts_config(cfg["mcts_config"]),
			tss_config(cfg["tss_config"])
	{
	}
	Json SearchConfig::toJson() const
	{
		Json result;
		result["max_batch_size"] = max_batch_size;
		result["tree_config"] = tree_config.toJson();
		result["mcts_config"] = mcts_config.toJson();
		result["tss_config"] = tss_config.toJson();
		return result;
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
			network_arch(get_value<std::string>(options, "network_arch")),
			sampler_type(get_value<std::string>(options, "sampler_type", "visits")),
			keep_loaded(get_value<bool>(options, "keep_loaded", true)),
			augment_training_data(get_value<bool>(options, "augment_training_data")),
			device_config(options["device_config"]),
			steps_per_iteration(get_value<int>(options, "steps_per_iteration")),
			blocks(get_value<int>(options, "blocks")),
			filters(get_value<int>(options, "filters")),
			l2_regularization(get_value<double>(options, "l2_regularization")),
			validation_percent(get_value<double>(options, "validation_percent")),
			learning_rate(options["learning_rate"]),
			buffer_size(options["buffer_size"])
	{
//		for (int i = 0; i < options["learning_rate_schedule"].size(); i++)
//		{
//			int first = options["learning_rate_schedule"][i]["from_epoch"];
//			double second = options["learning_rate_schedule"][i]["value"];
//			learning_rate_schedule.push_back( { first, second });
//		}
//		for (int i = 0; i < options["buffer_size"].size(); i++)
//		{
//			int first = options["buffer_size"][i]["from_epoch"];
//			int second = options["buffer_size"][i]["size"];
//			buffer_size.push_back( { first, second });
//		}
	}
	Json TrainingConfig::toJson() const
	{
		Json result;
		result["network_arch"] = network_arch;
		result["sampler_type"] = sampler_type;
		result["keep_loaded"] = keep_loaded;
		result["augment_training_data"] = augment_training_data;
		result["device_config"] = device_config.toJson();
		result["steps_per_iteration"] = steps_per_iteration;
		result["blocks"] = blocks;
		result["filters"] = filters;
		result["l2_regularization"] = l2_regularization;
		result["validation_percent"] = validation_percent;
		result["learning_rate"] = learning_rate.toJson();
		result["buffer_size"] = buffer_size.toJson();
//		for (size_t i = 0; i < learning_rate_schedule.size(); i++)
//			result["learning_rate_schedule"][i] = Json(
//					{ { "from_epoch", learning_rate_schedule[i].first }, { "value", learning_rate_schedule[i].second } });
//		for (size_t i = 0; i < buffer_size.size(); i++)
//			result["buffer_size"][i] = Json( { { "from_epoch", buffer_size[i].first }, { "size", buffer_size[i].second } });
		return result;
	}

	SelfplayConfig::SelfplayConfig(const Json &options) :
			use_opening(get_value<bool>(options, "use_opening")),
			use_symmetries(get_value<bool>(options, "use_symmetries")),
			games_per_iteration(get_value<int>(options, "games_per_iteration")),
			games_per_thread(get_value<int>(options, "games_per_thread")),
			simulations(get_value<int>(options, "simulations")),
			final_selector(get_value<EdgeSelectorConfig>(options, "final_selector")),
			device_config(),
			search_config(options["search_config"])
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
		result["simulations"] = simulations;
		result["final_selector"] = final_selector.toJson();
		for (size_t i = 0; i < device_config.size(); i++)
			result["device_config"][i] = device_config[i].toJson();
		result["search_config"] = search_config.toJson();
		return result;
	}

	EvaluationConfig::EvaluationConfig(const Json &options) :
			use_evaluation(get_value<bool>(options, "use_evaluation")),
			in_parallel(get_value<bool>(options, "in_parallel")),
			selfplay_options(options["selfplay_options"])
	{
		if (options.hasKey("opponents"))
			for (int i = 0; i < options["opponents"].size(); i++)
				opponents.push_back(options["opponents"][i].getInt());
	}
	Json EvaluationConfig::toJson() const
	{
		Json result;
		result["use_evaluation"] = use_evaluation;
		result["in_parallel"] = in_parallel;
		result["opponents"] = Json(JsonType::Array);
		for (size_t i = 0; i < opponents.size(); i++)
			result["opponents"][i] = opponents[i];
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

