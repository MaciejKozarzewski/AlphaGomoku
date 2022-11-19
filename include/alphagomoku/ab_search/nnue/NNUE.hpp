/*
 * NNUE.hpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_NNUE_NNUE_HPP_
#define ALPHAGOMOKU_AB_SEARCH_NNUE_NNUE_HPP_

#include <alphagomoku/ab_search/nnue/layers.hpp>
#include <alphagomoku/ab_search/nnue/wrappers.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <map>
#include <string>

class Json;
class SerializedObject;

namespace ag
{
	namespace nnue
	{
		class NNUEWeights
		{
			public:
				QuantizedLayer layer_1;
				RealSpaceLayer layer_2;
				RealSpaceLayer layer_3;

				NNUEWeights() = default;
				NNUEWeights(GameRules rules);
				Json save(SerializedObject &so) const;

				static void setPaths(const std::map<GameRules, std::string> &paths);
				static const NNUEWeights& get(GameRules rules);
		};

		class TrainingNNUE
		{
				InputLayerThreats input_layer_threats;
				InputLayerPatterns input_layer_patterns;

				std::vector<MiddleLayer> hidden_layers;

				OutputLayerSoftmax output_layer_softmax;
				OutputLayerSigmoid output_layer_sigmoid;
				int input_variant = 1;
				int output_variant = 1;
			public:
				TrainingNNUE(GameConfig gameConfig);
				Value forward(const PatternCalculator &calc);
				float backward(const PatternCalculator &calc, Value target);
				void learn(float learningRate, float weightDecay = 0.0f);
				NNUEWeights dump() const;
		};

		class InferenceNNUE
		{
				GameConfig game_config;
				AlignedStorage<int32_t, 32> accumulator_stack;
				int current_depth = 0;
				const NNUEWeights *weights;

				std::vector<int> removed_features;
				std::vector<int> added_features;
			public:
				InferenceNNUE(GameConfig gameConfig);
				InferenceNNUE(GameConfig gameConfig, const NNUEWeights &weights);
				void refresh(const PatternCalculator &calc);
				void update(const PatternCalculator &calc);
				float forward();
			private:
				Wrapper1D<int32_t> get_current_accumulator();
				Wrapper1D<int32_t> get_previous_accumulator();
				int get_row_index(Location loc) const noexcept;
		};

	} /* namespace nnue */
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_NNUE_NNUE_HPP_ */
