/*
 * configs.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_CONFIGS_HPP_
#define ALPHAGOMOKU_UTILS_CONFIGS_HPP_

#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/utils/Parameter.hpp>

#include <minml/core/Device.hpp>
#include <minml/utils/json.hpp>

#include <vector>
#include <limits>
#include <cmath>

namespace ag
{
	struct GameConfig
	{
		private:
			struct Defaults
			{
					static constexpr GameRules rules = GameRules::FREESTYLE;
					static constexpr int rows = 0;
					static constexpr int cols = 0;
					static constexpr int draw_after = rows * cols;
			};
		public:
			GameRules rules = Defaults::rules;
			int rows = Defaults::rows;
			int cols = Defaults::cols;
			int draw_after = Defaults::draw_after;

			GameConfig() = default;
			GameConfig(GameRules rules, int rows, int cols);
			GameConfig(GameRules rules, int size);
			GameConfig(const Json &cfg);
			Json toJson() const;
	};

	struct TreeConfig
	{
		private:
			struct Defaults
			{
					static constexpr float information_leak_threshold = 0.01f;
					static constexpr int initial_node_cache_size = 65536;
					static constexpr int edge_bucket_size = 200000;
					static constexpr int node_bucket_size = 10000;
			};
		public:
			float information_leak_threshold = Defaults::information_leak_threshold;
			int initial_node_cache_size = Defaults::initial_node_cache_size;
			int edge_bucket_size = Defaults::edge_bucket_size;
			int node_bucket_size = Defaults::node_bucket_size;

			float weight_c = 0.1f;
			float min_uncertainty = 0.01f;

			TreeConfig() = default;
			TreeConfig(const Json &cfg);
			Json toJson() const;
	};

	struct EdgeSelectorConfig
	{
		private:
			struct Defaults
			{
					static constexpr float noise_weight = 0.0f;
					static constexpr float exploration_constant = 1.25f;
					static constexpr float exploration_exponent = 0.5f;
			};
		public:
			std::string policy = "puct"; // allowed values are: 'puct', 'uct', 'max_value', 'max_policy', 'max_visit', 'min_visit,' 'best'
			std::string init_to = "parent"; // allowed values are: 'parent', 'loss', 'draw', 'q_head'
			std::string noise_type = "none"; // allowed values are: 'none', 'custom', 'dirichlet', 'gumbel'
			float noise_weight = Defaults::noise_weight; // only relevant if noise_type != 'none'
			float exploration_constant = Defaults::exploration_constant;
			float exploration_exponent = Defaults::exploration_exponent;

			EdgeSelectorConfig() = default;
			EdgeSelectorConfig(const Json &cfg);
			Json toJson() const;
	};

	struct MCTSConfig
	{
		private:
			struct Defaults
			{
					static constexpr int max_children = std::numeric_limits<int>::max();
					static constexpr float policy_expansion_threshold = 1.0e-4f;
			};
		public:
			EdgeSelectorConfig edge_selector_config;
			int max_children = Defaults::max_children;
			float policy_expansion_threshold = Defaults::policy_expansion_threshold;

			MCTSConfig() = default;
			MCTSConfig(const Json &cfg);
			Json toJson() const;
	};

	struct TSSConfig
	{
		private:
			struct Defaults
			{
					static constexpr int mode = 0;
					static constexpr int max_positions = 100;
					static constexpr int hash_table_size = 1048576;
			};
		public:
			int mode = Defaults::mode; /**< 0 - only terminal moves, 1 - static evaluation, 2 - recursive search */
			int max_positions = Defaults::max_positions;
			int hash_table_size = Defaults::hash_table_size;

			TSSConfig() = default;
			TSSConfig(const Json &cfg);
			Json toJson() const;
	};

	struct SearchConfig
	{
		private:
			struct Defaults
			{
					static constexpr int max_batch_size = 1;
			};
		public:
			int max_batch_size = Defaults::max_batch_size;
			TreeConfig tree_config;
			MCTSConfig mcts_config;
			TSSConfig tss_config;

			SearchConfig() = default;
			SearchConfig(const Json &cfg);
			Json toJson() const;
	};

	struct DeviceConfig
	{
		private:
			struct Defaults
			{
					static constexpr int batch_size = 1;
					static constexpr int omp_threads = 1;
			};
		public:
			ml::Device device = ml::Device::cpu();
			int batch_size = Defaults::batch_size;
			int omp_threads = Defaults::omp_threads;

			DeviceConfig() = default;
			DeviceConfig(const Json &cfg);
			Json toJson() const;
	};

	struct TrainingConfig
	{
			std::string network_arch = "ResnetPV";
			std::string sampler_type = "visits";
			bool keep_loaded = true;
			bool augment_training_data = true;
			DeviceConfig device_config;
			int steps_per_iteration = 1000;
			int blocks = 2;
			int filters = 32;
			int patch_size = 1;
			double l2_regularization = 1.0e-4;
			double validation_percent = 0.05;
			Parameter<double> learning_rate = 1.0e-3;
			Parameter<int> buffer_size = 10;

			TrainingConfig() = default;
			TrainingConfig(const Json &options);
			Json toJson() const;
	};

	struct SelfplayConfig
	{
			bool use_opening = true;
			bool use_symmetries = true;
			int games_per_iteration = 100;
			int games_per_thread = 8;
			int simulations = 100;
			EdgeSelectorConfig final_selector;
			std::vector<DeviceConfig> device_config = { DeviceConfig() };
			SearchConfig search_config;

			SelfplayConfig() = default;
			SelfplayConfig(const Json &options);
			Json toJson() const;
	};

	struct EvaluationConfig
	{
			bool use_evaluation = true;
			bool in_parallel = true;
			std::vector<int> opponents;
			SelfplayConfig selfplay_options;

			EvaluationConfig() = default;
			EvaluationConfig(const Json &options);
			Json toJson() const;
	};

	struct MasterLearningConfig
	{
			std::string description;
			std::string data_type;
			GameConfig game_config;
			TrainingConfig training_config;
			SelfplayConfig generation_config;
			EvaluationConfig evaluation_config;

			MasterLearningConfig() = default;
			MasterLearningConfig(const Json &options);
			Json toJson() const;
	};
}

#endif /* ALPHAGOMOKU_UTILS_CONFIGS_HPP_ */
