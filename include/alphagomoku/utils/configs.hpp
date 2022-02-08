/*
 * configs.hpp
 *
 *  Created on: Mar 21, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_CONFIGS_HPP_
#define ALPHAGOMOKU_CONFIGS_HPP_

#include <alphagomoku/game/rules.hpp>

#include <libml/hardware/Device.hpp>

class Json;
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
			};
		public:
			GameRules rules;
			int rows;
			int cols;

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
					static constexpr int bucket_size = 1000000;
			};
		public:
			int bucket_size = Defaults::bucket_size;

			TreeConfig() = default;
			TreeConfig(const Json &cfg);
			Json toJson() const;
	};

	struct CacheConfig
	{
		private:
			struct Defaults
			{
					static constexpr int cache_size = 1048576;
			};
		public:
			int cache_size = Defaults::cache_size;

			CacheConfig() = default;
			CacheConfig(const Json &cfg);
			Json toJson() const;
	};

	struct SearchConfig
	{
		private:
			struct Defaults
			{
					static constexpr int max_batch_size = 256;
					static constexpr float exploration_constant = 1.25f;
					static constexpr float expansion_prior_treshold = 0.0f;
					static constexpr int max_children = std::numeric_limits<int>::max();
					static constexpr float noise_weight = 0.0f;
					static constexpr bool use_endgame_solver = false;
					static constexpr int vcf_solver_level = 0;
			};
		public:
			int max_batch_size = Defaults::max_batch_size;
			float exploration_constant = Defaults::exploration_constant;
			float expansion_prior_treshold = Defaults::expansion_prior_treshold;
			int max_children = Defaults::max_children;
			float noise_weight = Defaults::noise_weight;
			bool use_endgame_solver = Defaults::use_endgame_solver;
			int vcf_solver_level = Defaults::vcf_solver_level; /**< 0 - only terminal moves, 1 - static evaluation, 2 - recursive search */

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

	Json getDefaultSelfplayConfig();
	Json getDefaultTrainingConfig();
	Json getDefaultEvaluationConfig();
}

#endif /* ALPHAGOMOKU_CONFIGS_HPP_ */
