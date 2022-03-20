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
#include <libml/utils/json.hpp>

#include <vector>
#include <limits>
#include <cmath>

namespace ag
{
	template<typename T>
	class Parameter
	{
			T m_value = { };
			std::vector<std::pair<int, T>> m_schedule;
			std::string m_interpolate = "none";
		public:
			Parameter() = default;
			Parameter(T value) noexcept :
					m_value(value)
			{
			}
			Parameter(const std::vector<std::pair<int, T>> &schedule, std::string interpolate = "none") :
					m_schedule(schedule),
					m_interpolate(interpolate)
			{
			}
			Parameter(const Json &cfg)
			{
				if (cfg.isObject())
				{
					m_interpolate = cfg["interpolate"].getString();
					if (cfg["epoch"].size() != cfg["value"].size())
						throw std::invalid_argument("'epoch' and 'value' list must have the same number of elements");
					for (int i = 0; i < cfg["epoch"].size(); i++)
					{
						int epoch = cfg["epoch"][i].getInt();
						T value = cfg["value"][i].getDouble();
						m_schedule.push_back(std::pair<int, T>( { epoch, value }));
					}
				}
				else
					m_value = cfg.getDouble();
			}
			Json toJson() const
			{
				if (m_schedule.empty())
					return Json(m_value);
				else
				{
					Json result( { { "interpolate", m_interpolate }, { "epoch", Json(JsonType::Array) }, { "value", Json(JsonType::Array) } });
					for (size_t i = 0; i < m_schedule.size(); i++)
					{
						result["epoch"][i] = m_schedule.at(i).first;
						result["value"][i] = m_schedule.at(i).second;
					}
					return result;
				}
			}
			operator T() const
			{
				if (not m_schedule.empty())
					throw std::logic_error("The parameter does not have single value");
				return m_value;
			}
			T getValue() const
			{
				if (not m_schedule.empty())
					throw std::logic_error("The parameter does not have single value");
				return m_value;
			}
			T getValue(int epoch) const
			{
				if (m_interpolate == "none")
				{
					T result = m_schedule.front().second;
					for (size_t i = 0; i < m_schedule.size(); i++)
						if (epoch >= m_schedule.at(i).first)
							result = m_schedule.at(i).second;
					return result;
				}
				else
				{
					if (epoch <= m_schedule.front().first)
						return m_schedule.front().second;
					if (epoch >= m_schedule.back().first)
						return m_schedule.back().second;

					std::pair<int, T> prev, next;
					for (size_t i = 0; i < m_schedule.size(); i++)
						if (epoch > m_schedule.at(i).first)
						{
							prev = m_schedule.at(i);
							next = m_schedule.at(i + 1);
						}

					double result = T { };
					if (m_interpolate == "linear")
					{
						const double tmp1 = epoch - prev.first;
						const double tmp2 = next.first - prev.first;
						const double tmp3 = next.second - prev.second;
						result = prev.second + tmp1 / tmp2 * tmp3;
					}
					if (m_interpolate == "exp")
					{
						const double tmp1 = epoch - prev.first;
						const double tmp2 = next.first - prev.first;
						const double a = std::log(next.second / prev.second) / tmp2;
						result = prev.second * std::exp(a * tmp1);
					}
					if (std::is_integral<T>::value)
						return static_cast<T>(result + 0.5); // round to nearest
					else
						return result;
				}
			}
	};

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
			GameRules rules = Defaults::rules;
			int rows = Defaults::rows;
			int cols = Defaults::cols;

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
					static constexpr int initial_cache_size = 65536;
					static constexpr int bucket_size = 100000;
			};
		public:
			int initial_cache_size = Defaults::initial_cache_size;
			int bucket_size = Defaults::bucket_size;

			TreeConfig() = default;
			TreeConfig(const Json &cfg);
			Json toJson() const;
	};

	struct SearchConfig
	{
		private:
			struct Defaults
			{
					static constexpr int max_batch_size = 1;
					static constexpr float exploration_constant = 1.25f;
					static constexpr float expansion_prior_treshold = 0.0f;
					static constexpr int max_children = std::numeric_limits<int>::max();
					static constexpr float noise_weight = 0.0f;
					static constexpr int vcf_solver_level = 0;
					static constexpr int vcf_solver_max_positions = 100;
					static constexpr int style_factor = 1;
			};
		public:
			int max_batch_size = Defaults::max_batch_size;
			float exploration_constant = Defaults::exploration_constant;
			float expansion_prior_treshold = Defaults::expansion_prior_treshold;
			int max_children = Defaults::max_children;
			float noise_weight = Defaults::noise_weight;
			int vcf_solver_level = Defaults::vcf_solver_level; /**< 0 - only terminal moves, 1 - static evaluation, 2 - recursive search */
			int vcf_solver_max_positions = Defaults::vcf_solver_max_positions;
			int style_factor = Defaults::style_factor; /**< 0 - win + draw, 1 - win + 0.5 * draw, 2 - win */

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
			bool augment_training_data = true;
			DeviceConfig device_config;
			int steps_per_iteration = 1000;
			int blocks = 2;
			int filters = 32;
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
			Parameter<int> simulations_min = 100;
			Parameter<int> simulations_max = 800;
			Parameter<int> positions_skip = 1;
			std::vector<DeviceConfig> device_config = { DeviceConfig() };
			SearchConfig search_config;
			TreeConfig tree_config;

			SelfplayConfig() = default;
			SelfplayConfig(const Json &options);
			Json toJson() const;
	};

	struct EvaluationConfig
	{
			bool use_evaluation = true;
			bool use_gating = false;
			double gating_threshold = 0.55;
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

#endif /* ALPHAGOMOKU_CONFIGS_HPP_ */
