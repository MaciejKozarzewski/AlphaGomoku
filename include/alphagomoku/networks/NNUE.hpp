/*
 * NNUE.hpp
 *
 *  Created on: May 23, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_NNUE_HPP_
#define ALPHAGOMOKU_NETWORKS_NNUE_HPP_

#include <alphagomoku/networks/nnue_ops/layers.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/search/Value.hpp>

#include <map>
#include <string>

#include <minml/graph/Graph.hpp>

class Json;
class SerializedObject;

namespace ag
{
	namespace nnue
	{
		class NNUEWeights
		{
			public:
				NnueLayer<int8_t, int16_t> layer_0;
				NnueLayer<int16_t, int32_t> layer_1;
				std::vector<NnueLayer<float, float>> fp32_layers;

				NNUEWeights() = default;
				NNUEWeights(GameRules rules);
				Json save(SerializedObject &so) const;
				void load(const Json &json, const SerializedObject &so);

				static void setPaths(const std::map<GameRules, std::string> &paths);
				static const NNUEWeights& get(GameRules rules);
		};

		class TrainingNNUE
		{
				GameConfig game_config;
				ml::Graph model;
				std::vector<float> input_on_cpu, target_on_cpu;
				PatternCalculator calculator;
			public:
				TrainingNNUE(GameConfig gameConfig, int batchSize, const std::string &path);
				TrainingNNUE(GameConfig gameConfig, std::initializer_list<int> arch, int batchSize);
				void packInputData(int index, const matrix<Sign> &board, Sign signToMove);
				void packTargetData(int index, float valueTarget);
				float unpackOutput(int index) const;
				void forward(int batchSize);
				float backward(int batchSize);
				void setLearningRate(float lr);
				void learn();
				void save(const std::string &path) const;
				void load(const std::string &path);
				/*
				 * Optimize the network, perform quantization, pack weights into appropriate format and dump.
				 */
				NNUEWeights dump();
			private:
				int get_row_index(Location loc) const noexcept;
		};

		struct NNUEStats
		{
				TimedStat refresh;
				TimedStat update;
				TimedStat forward;
				NNUEStats();
				std::string toString() const;
		};

		class InferenceNNUE
		{
				GameConfig game_config;
				std::vector<Accumulator<int16_t>> accumulator_stack;
				int current_depth = 0;
				NNUEWeights weights;

				std::vector<int> removed_features;
				std::vector<int> added_features;

				NNUEStats stats;

				std::function<void(const NnueLayer<int8_t, int16_t>&, Accumulator<int16_t>&, const std::vector<int>&)> refresh_function;
				std::function<
						void(const NnueLayer<int8_t, int16_t>&, const Accumulator<int16_t>&, Accumulator<int16_t>&, const std::vector<int>&,
								const std::vector<int>&)> update_function;
				std::function<float(const Accumulator<int16_t>&, const NnueLayer<int16_t, int32_t>&, const std::vector<NnueLayer<float, float>>&)> forward_function;
			public:
				InferenceNNUE() = default;
				InferenceNNUE(GameConfig gameConfig);
				InferenceNNUE(GameConfig gameConfig, const NNUEWeights &weights);
				void refresh(const PatternCalculator &calc);
				void update(const PatternCalculator &calc);
				float forward();
				void print_stats() const;
			private:
				void init_functions() noexcept;
				Accumulator<int16_t>& get_current_accumulator();
				Accumulator<int16_t>& get_previous_accumulator();
				int get_row_index(Location loc) const noexcept;
		};

	} /* namespace nnue */
} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NNUE_HPP_ */
