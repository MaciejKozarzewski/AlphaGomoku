/*
 * NNUE.cpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/NNUE.hpp>
#include <alphagomoku/ab_search/nnue/def_ops.hpp>
#include <alphagomoku/ab_search/nnue/sse2_ops.hpp>
#include <alphagomoku/ab_search/nnue/avx2_ops.hpp>

#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <iostream>
#include <string>

namespace
{
	float binary_cross_entropy(float output, float target)
	{
		return -target * std::log(1.0e-8f + output) - (1.0f - target) * std::log(1.0e-8f + (1.0f - output));
	}
	static std::map<ag::GameRules, std::string> paths_to_weights;
}

namespace ag
{
	namespace nnue
	{
		NNUEWeights::NNUEWeights(GameRules rules)
		{
			FileLoader fl(paths_to_weights.find(rules)->second);
			layer_1 = QuantizedLayer(fl.getJson()["layer_1"], fl.getBinaryData());
			layer_2 = RealSpaceLayer(fl.getJson()["layer_2"], fl.getBinaryData());
			layer_3 = RealSpaceLayer(fl.getJson()["layer_3"], fl.getBinaryData());
		}
		Json NNUEWeights::save(SerializedObject &so) const
		{
			Json result;
			result["layer_1"] = layer_1.save(so);
			result["layer_2"] = layer_2.save(so);
			result["layer_3"] = layer_3.save(so);
			return result;
		}

		void NNUEWeights::setPaths(const std::map<GameRules, std::string> &paths)
		{
			paths_to_weights = paths;
		}
		const NNUEWeights& NNUEWeights::get(GameRules rules)
		{
			switch (rules)
			{
				case GameRules::FREESTYLE:
				{
					static const NNUEWeights table(GameRules::FREESTYLE);
					return table;
				}
				case GameRules::STANDARD:
				{
					static const NNUEWeights table(GameRules::STANDARD);
					return table;
				}
				case GameRules::RENJU:
				{
					static const NNUEWeights table(GameRules::RENJU);
					return table;
				}
				case GameRules::CARO5:
				{
					static const NNUEWeights table(GameRules::CARO5);
					return table;
				}
				case GameRules::CARO6:
				{
					static const NNUEWeights table(GameRules::CARO6);
					return table;
				}
				default:
					throw std::logic_error("NNUEWeights::get() unknown rule " + std::to_string((int) rules));
			}
		}

		TrainingNNUE::TrainingNNUE(GameConfig gameConfig) :
				input_layer_threats(gameConfig, 32),
				input_layer_patterns(gameConfig, 32),
				output_layer_softmax(8),
				output_layer_sigmoid(8),
				input_variant(1),
				output_variant(2)
		{
			hidden_layers.push_back(MiddleLayer(32, 8));
//			hidden_layers.push_back(MiddleLayer(32, 32));
//			hidden_layers.push_back(MiddleLayer(32, 32));
		}
		Value TrainingNNUE::forward(const PatternCalculator &calc)
		{
			if (input_variant == 1)
				input_layer_threats.forward(calc);
			else
				input_layer_patterns.forward(calc);

			const auto &tmp = (input_variant == 1) ? input_layer_threats.getOutput() : input_layer_patterns.getOutput();
			hidden_layers.front().forward(tmp);
			for (size_t i = 1; i < hidden_layers.size(); i++)
				hidden_layers.at(i).forward(hidden_layers.at(i - 1).getOutput());

			if (output_variant == 1)
			{
				output_layer_softmax.forward(hidden_layers.back().getOutput());
				return output_layer_softmax.getOutput();
			}
			else
			{
				output_layer_sigmoid.forward(hidden_layers.back().getOutput());
				return Value(output_layer_sigmoid.getOutput());
			}
		}
		float TrainingNNUE::backward(const PatternCalculator &calc, Value target)
		{
			const Value output = (output_variant == 1) ? output_layer_softmax.getOutput() : output_layer_sigmoid.getOutput();
			if (output_variant == 1)
			{
				std::vector<float> gradient( { output.win - target.win, output.draw - target.draw, output.loss - target.loss });
				output_layer_softmax.backward(gradient);
			}
			else
			{
				target.win += 0.5 * target.draw; // calculate expectation value
				std::vector<float> gradient( { output.win - target.win });
				output_layer_sigmoid.backward(gradient);
			}

			auto &tmp = (output_variant == 1) ? output_layer_softmax.getGradientPrev() : output_layer_sigmoid.getGradientPrev();
			hidden_layers.back().backward(tmp);

			for (int i = (int) hidden_layers.size() - 2; i >= 0; i--)
				hidden_layers.at(i).backward(hidden_layers.at(i + 1).getGradientPrev());

			if (input_variant == 1)
				input_layer_threats.backward(calc, hidden_layers.front().getGradientPrev());
			else
				input_layer_patterns.backward(calc, hidden_layers.front().getGradientPrev());

			if (output_variant == 1)
			{
				const float loss = binary_cross_entropy(output.win, target.win) + binary_cross_entropy(output.draw, target.draw)
						+ binary_cross_entropy(output.loss, target.loss);
				const float tmp = binary_cross_entropy(target.win, target.win) + binary_cross_entropy(target.draw, target.draw)
						+ binary_cross_entropy(target.loss, target.loss);
				return loss - tmp;
			}
			else
				return binary_cross_entropy(output.win, target.win) - binary_cross_entropy(target.win, target.win);
		}
		void TrainingNNUE::learn(float learningRate, float weightDecay)
		{
			if (input_variant == 1)
				input_layer_threats.learn(learningRate, weightDecay);
			else
				input_layer_patterns.learn(learningRate, weightDecay);

			for (size_t i = 0; i < hidden_layers.size(); i++)
				hidden_layers[i].learn(learningRate, 0.0f);

			if (output_variant == 1)
				output_layer_softmax.learn(learningRate, weightDecay);
			else
				output_layer_sigmoid.learn(learningRate, weightDecay);
		}
		NNUEWeights TrainingNNUE::dump() const
		{
			NNUEWeights result;
			result.layer_1 = (input_variant == 1) ? input_layer_threats.dump_quantized() : input_layer_patterns.dump_quantized();
			result.layer_2 = hidden_layers[0].dump_real();
			result.layer_3 = (output_variant == 1) ? output_layer_softmax.dump_real() : output_layer_sigmoid.dump_real();
			return result;
		}

		/*
		 * InferenceNNUE
		 */
		InferenceNNUE::InferenceNNUE(GameConfig gameConfig) :
				InferenceNNUE(gameConfig, NNUEWeights::get(gameConfig.rules))
		{
		}
		InferenceNNUE::InferenceNNUE(GameConfig gameConfig, const NNUEWeights &weights) :
				game_config(gameConfig),
				accumulator_stack(gameConfig.rows * gameConfig.cols * weights.layer_1.bias.size()),
				weights(&weights)
		{
			removed_features.reserve(128);
			added_features.reserve(1024);
		}
		void InferenceNNUE::refresh(const PatternCalculator &calc)
		{
			current_depth = calc.getCurrentDepth();
			Wrapper1D<int32_t> accumulator = get_current_accumulator();

			added_features.clear();

			if (calc.getSignToMove() == Sign::CROSS)
				added_features.push_back(0);

			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
				{
					const int index = get_row_index(Location(row, col));
					const ThreatEncoding te = calc.getThreatAt(row, col);
					if (te.forCross() != ThreatType::NONE)
						added_features.push_back(index + static_cast<int>(te.forCross()) - 1);
					if (te.forCircle() != ThreatType::NONE)
						added_features.push_back(index + 9 + static_cast<int>(te.forCircle()) - 1);
					switch (calc.signAt(row, col))
					{
						case Sign::CROSS:
							added_features.push_back(index + 18);
							break;
						case Sign::CIRCLE:
							added_features.push_back(index + 19);
							break;
						default:
							break;
					}
				}

//			if (ml::Device::cpu().simd() >= ml::CpuSimd::AVX2)
//				avx2_refresh_accumulator(weights->layer_1, accumulator, added_features);
//			else
//			{
//				if (ml::Device::cpu().simd() >= ml::CpuSimd::SSE2)
//					sse2_refresh_accumulator(weights->layer_1, accumulator, added_features);
//				else
//					def_refresh_accumulator(weights->layer_1, accumulator, added_features);
//			}
		}
		void InferenceNNUE::update(const PatternCalculator &calc)
		{
			const bool is_accumulator_ready = current_depth >= calc.getCurrentDepth() or calc.getCurrentDepth() == 0;
			current_depth = calc.getCurrentDepth();
			if (is_accumulator_ready)
				return;
			Wrapper1D<int32_t> old_accumulator = get_previous_accumulator();
			Wrapper1D<int32_t> new_accumulator = get_current_accumulator();

			added_features.clear();
			removed_features.clear();
			if (calc.getSignToMove() == Sign::CROSS)
				added_features.push_back(0);
			else
				removed_features.push_back(0);

			for (auto iter = calc.getChangeOfThreats().begin(); iter < calc.getChangeOfThreats().end(); iter++)
			{
				const int base_row_index = get_row_index(iter->location);
				if (iter->previous.forCross() != iter->current.forCross())
				{
					if (iter->previous.forCross() != ThreatType::NONE)
						removed_features.push_back(base_row_index + 0 + static_cast<int>(iter->previous.forCross()) - 1);
					if (iter->current.forCross() != ThreatType::NONE)
						added_features.push_back(base_row_index + 0 + static_cast<int>(iter->current.forCross()) - 1);
				}
				if (iter->previous.forCircle() != iter->current.forCircle())
				{
					if (iter->previous.forCircle() != ThreatType::NONE)
						removed_features.push_back(base_row_index + 9 + static_cast<int>(iter->previous.forCircle()) - 1);
					if (iter->current.forCircle() != ThreatType::NONE)
						added_features.push_back(base_row_index + 9 + static_cast<int>(iter->current.forCircle()) - 1);
				}
			}

			const int base_row_index = get_row_index(calc.getChangeOfMoves().location);
			const Sign previous = calc.getChangeOfMoves().previous;
			const Sign current = calc.getChangeOfMoves().current;
			if (previous == Sign::NONE and current != Sign::NONE) // stone was placed
				added_features.push_back(base_row_index + 18 + static_cast<int>(current) - 1);
			if (previous != Sign::NONE and current == Sign::NONE) // stone was removed
				removed_features.push_back(base_row_index + 18 + static_cast<int>(previous) - 1);

//			if (ml::Device::cpu().simd() >= ml::CpuSimd::AVX2)
//				avx2_update_accumulator(weights->layer_1, old_accumulator, new_accumulator, removed_features, added_features);
//			else
//			{
//				if (ml::Device::cpu().simd() >= ml::CpuSimd::SSE2)
//					sse2_update_accumulator(weights->layer_1, old_accumulator, new_accumulator, removed_features, added_features);
//				else
//					def_update_accumulator(weights->layer_1, old_accumulator, new_accumulator, removed_features, added_features);
//			}
		}
		float InferenceNNUE::forward()
		{
			Wrapper1D<int32_t> accumulator = get_current_accumulator();

//			if (ml::Device::cpu().simd() >= ml::CpuSimd::AVX2)
//				return avx2_forward(accumulator, weights->layer_1, weights->layer_2, weights->layer_3);
//			else
//			{
//				if (ml::Device::cpu().simd() >= ml::CpuSimd::SSE2)
//					return sse2_forward(accumulator, weights->layer_1, weights->layer_2, weights->layer_3);
//				else
//					return def_forward(accumulator, weights->layer_1, weights->layer_2, weights->layer_3);
//			}
		}
		/*
		 * private
		 */
		Wrapper1D<int32_t> InferenceNNUE::get_current_accumulator()
		{
			const int length = weights->layer_1.bias.size();
			return Wrapper1D<int32_t>(accumulator_stack.data() + current_depth * length, length);
		}
		Wrapper1D<int32_t> InferenceNNUE::get_previous_accumulator()
		{
			assert(current_depth > 0);
			const int length = weights->layer_1.bias.size();
			return Wrapper1D<int32_t>(accumulator_stack.data() + (current_depth - 1) * length, length);
		}
		int InferenceNNUE::get_row_index(Location loc) const noexcept
		{
			return 1 + (loc.row * game_config.cols + loc.col) * InputLayerThreats::params_per_spot;
		}

	} /* namespace nnue */
} /* namespace ag */

