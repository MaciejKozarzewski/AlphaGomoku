/*
 * NNUE.cpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/NNUE.hpp>
#include <alphagomoku/networks/nnue_ops/def_ops.hpp>
#include <alphagomoku/networks/nnue_ops/avx2_ops.hpp>
#include <alphagomoku/networks/nnue_ops/sse41_ops.hpp>

#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>
#include <minml/core/Device.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/graph/graph_optimizers.hpp>

#include <iostream>
#include <string>

namespace
{
	using namespace ag;
	using namespace ag::nnue;

	std::vector<float> get_scales(const ml::Tensor &weights, float max_range)
	{
		std::vector<float> result(weights.firstDim());
		for (int i = 0; i < weights.firstDim(); i++)
		{
			float tmp = 0.0f;
			for (int j = 0; j < weights.lastDim(); j++)
				tmp = std::max(tmp, std::abs(weights.get( { i, j })));
			result[i] = (tmp == 0.0f) ? 1.0f : (tmp / max_range);
		}
		return result;
	}
	ThreatType increment_threat_type(ThreatType tt) noexcept
	{
		return static_cast<ThreatType>(static_cast<int>(tt) + 1);
	}
	template<typename T>
	T round_to(float x) noexcept
	{
		return static_cast<T>(x);
	}
	[[maybe_unused]] void placeholder_refresh_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator,
			const std::vector<int> &active) noexcept
	{
	}
	[[maybe_unused]] void placeholder_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, const Accumulator<int16_t> &oldAccumulator,
			Accumulator<int16_t> &newAccumulator, const std::vector<int> &removed, const std::vector<int> &added) noexcept
	{
	}
	[[maybe_unused]] float placeholder_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
			const std::vector<NnueLayer<float, float>> &fp32_layers) noexcept
	{
		return 0.0f;
	}
}

namespace ag
{
	namespace nnue
	{
		NNUEWeights::NNUEWeights(const std::string &path)
		{
			FileLoader fl(path);
			load(fl.getJson(), fl.getBinaryData());
		}
		Json NNUEWeights::save(SerializedObject &so) const
		{
			Json result;
			result["layer_0"] = save_layer(layer_0, so);
			result["layer_1"] = save_layer(layer_1, so);
			for (size_t i = 0; i < fp32_layers.size(); i++)
				result["layer_" + std::to_string(2 + i)] = save_layer(fp32_layers[i], so);
			return result;
		}
		void NNUEWeights::load(const Json &json, const SerializedObject &so)
		{
			load_layer(layer_0, json["layer_0"], so);
			load_layer(layer_1, json["layer_1"], so);
			fp32_layers.resize(json.size() - 2);
			for (size_t i = 0; i < fp32_layers.size(); i++)
				load_layer(fp32_layers[i], json["layer_" + std::to_string(2 + i)], so);
		}

		TrainingNNUE::TrainingNNUE(GameConfig gameConfig, int batchSize, const std::string &path) :
				game_config(gameConfig),
				calculator(gameConfig)
		{
			load(path);
			model.setInputShape(ml::Shape( { batchSize, model.getInputShape().lastDim() }));
			input_on_cpu = std::vector<float>(model.getInputShape().volume());
			target_on_cpu = std::vector<float>(model.getOutputShape().volume());
		}
		TrainingNNUE::TrainingNNUE(GameConfig gameConfig, std::initializer_list<int> arch, int batchSize) :
				game_config(gameConfig),
				calculator(gameConfig)
		{
			auto x = model.addInput( { batchSize, 1 + gameConfig.rows * gameConfig.cols * 16 });
			for (size_t i = 0; i < arch.size() - 1; i++)
			{
				const int neurons = arch.begin()[i];
//				x = model.add(ml::Dense(neurons, "relu"), x);
				x = model.add(ml::Dense(neurons, "linear").useBias(false), x);
				x = model.add(ml::BatchNormalization("relu").useGamma(false), x);
			}
			assert(arch.begin()[arch.size() - 1] == 1);
			x = model.add(ml::Dense(1, "sigmoid"), x);
			model.addOutput(x);

			model.init();
			model.setOptimizer(ml::Optimizer());
			model.setRegularizer(ml::Regularizer(1.0e-5f));
			model.setLearningRate(1.0e-3f);
			model.moveTo(ml::Device::cuda(0));

			input_on_cpu = std::vector<float>(model.getInputShape().volume());
			target_on_cpu = std::vector<float>(model.getOutputShape().volume());
		}
		void TrainingNNUE::packInputData(int index, const matrix<Sign> &board, Sign signToMove)
		{
			calculator.setBoard(board, signToMove);
			const int inputs = model.getInputShape().lastDim();
			float *tensor = input_on_cpu.data() + index * inputs;
			std::memset(tensor, 0, sizeof(float) * inputs);

			tensor[0] = static_cast<float>(signToMove == Sign::CROSS);
			for (ThreatType tt = ThreatType::OPEN_3; tt <= ThreatType::FIVE; tt = increment_threat_type(tt))
			{
				const LocationList &cross_threats = calculator.getThreatHistogram(Sign::CROSS).get(tt);
				for (auto iter = cross_threats.begin(); iter < cross_threats.end(); iter++)
					tensor[get_row_index(*iter) + 0 + static_cast<int>(tt) - 2] = 1.0f;

				const LocationList &circle_threats = calculator.getThreatHistogram(Sign::CIRCLE).get(tt);
				for (auto iter = circle_threats.begin(); iter < circle_threats.end(); iter++)
					tensor[get_row_index(*iter) + 7 + static_cast<int>(tt) - 2] = 1.0f;
			}
			for (int i = 0; i < board.size(); i++)
				if (board[i] != Sign::NONE)
					tensor[1 + 16 * i + 14 + static_cast<int>(board[i]) - 1] = 1.0f;
		}
		void TrainingNNUE::packTargetData(int index, float valueTarget)
		{
			target_on_cpu[index] = valueTarget;
		}
		float TrainingNNUE::unpackOutput(int index) const
		{
			return model.getOutput().get( { index, 0 });
		}
		void TrainingNNUE::forward(int batchSize)
		{
			model.getInput().copyFromHost(model.context(), input_on_cpu.data(), sizeof(float) * input_on_cpu.size());
			model.forward(batchSize);
//			std::cout << '\n';
//			for (int i = 0; i < 64; i++)
//				std::cout << model.getNode(1).getOutputTensor().get( { 0, i }) << ' ';
//			std::cout << '\n';
//			for (int i = 0; i < 16; i++)
//				std::cout << model.getNode(2).getOutputTensor().get( { 0, i }) << ' ';
//			std::cout << '\n';
//			for (int i = 0; i < 16; i++)
//				std::cout << model.getNode(3).getOutputTensor().get( { 0, i }) << ' ';
//			std::cout << '\n' << '\n';
		}
		float TrainingNNUE::backward(int batchSize)
		{
			model.getTarget().copyFromHost(model.context(), target_on_cpu.data(), sizeof(float) * target_on_cpu.size());
			model.backward(batchSize);
			return model.getLoss(batchSize).at(0);
		}
		void TrainingNNUE::setLearningRate(float lr)
		{
			model.setLearningRate(lr);
		}
		void TrainingNNUE::learn()
		{
			model.learn();
		}
		void TrainingNNUE::save(const std::string &path) const
		{
			SerializedObject so;
			const Json json = model.save(so);
			FileSaver fs(path);
			fs.save(json, so, 2);
		}
		void TrainingNNUE::load(const std::string &path)
		{
			FileLoader fl(path);
			model.load(fl.getJson(), fl.getBinaryData());
		}
		NNUEWeights TrainingNNUE::dump()
		{
			model.makeNonTrainable();
			ml::FoldBatchNorm().optimize(model);

			NNUEWeights result;

			std::vector<float> scales;
			{ // first layer
				const ml::Tensor &weights = model.getNode(1).getLayer().getWeights().getParam();
				const ml::Tensor &bias = model.getNode(1).getLayer().getBias().getParam();
				scales = get_scales(weights, 127);

				const int neurons = weights.firstDim();
				const int inputs = weights.lastDim();
				result.layer_0 = NnueLayer<int8_t, int16_t>(inputs, neurons);
				int idx = 0;
				for (int i = 0; i < inputs; i++)
					for (int j = 0; j < neurons; j++, idx++)
						result.layer_0.weights()[idx] = round_to<int8_t>(weights.get( { j, i }) / scales[j]); // saved as transposed
				for (int i = 0; i < neurons; i++)
					result.layer_0.bias()[i] = round_to<int16_t>(bias.get( { i }) / scales[i]);

//				for (size_t i = 0; i < scales.size(); i++)
//					std::cout << scales[i] << ' ';
//				std::cout << '\n';
			}

			{ // second layer
				ml::Tensor weights = model.getNode(2).getLayer().getWeights().getParam();
				const ml::Tensor &bias = model.getNode(2).getLayer().getBias().getParam();

				const int neurons = weights.firstDim();
				const int inputs = weights.lastDim();

				result.layer_1 = NnueLayer<int16_t, int32_t>(inputs, neurons);
				for (int i = 0; i < neurons; i++)
					for (int j = 0; j < inputs; j++)
						weights.set(scales[j] * weights.get( { i, j }), { i, j }); // apply first layer scales to the second layer weights

				scales = get_scales(weights, 4095);
				int idx = 0;
				for (int i = 0; i < inputs; i += 2)
					for (int j = 0; j < neurons; j++, idx += 2)
					{ // interleave two rows
						result.layer_1.weights()[idx + 0] = round_to<int16_t>(weights.get( { j, i + 0 }) / scales[j]);
						result.layer_1.weights()[idx + 1] = round_to<int16_t>(weights.get( { j, i + 1 }) / scales[j]);
					}
				for (int i = 0; i < neurons; i++)
					result.layer_1.bias()[i] = round_to<int32_t>(bias.get( { i }) / scales[i]);

//				for (size_t i = 0; i < scales.size(); i++)
//					std::cout << scales[i] << ' ';
//				std::cout << '\n';
			}

			for (int n = 3; n < model.numberOfNodes(); n++)
			{ // remaining layers
				const ml::Tensor &weights = model.getNode(n).getLayer().getWeights().getParam();
				const ml::Tensor &bias = model.getNode(n).getLayer().getBias().getParam();

				const int neurons = weights.firstDim();
				const int inputs = weights.lastDim();

				NnueLayer<float, float> tmp(inputs, neurons);
				int idx = 0;
				for (int i = 0; i < inputs; i++)
					for (int j = 0; j < neurons; j++, idx++)
						tmp.weights()[idx] = weights.get( { j, i }) * scales[i];
				for (int i = 0; i < neurons; i++)
					tmp.bias()[i] = bias.get( { i });
				scales = std::vector<float>(neurons, 1.0f);
				result.fp32_layers.push_back(tmp);
			}

			return result;
		}
		/*
		 * private
		 */
		int TrainingNNUE::get_row_index(Location loc) const noexcept
		{
			return 1 + (loc.row * game_config.cols + loc.col) * 16;
		}

		/*
		 * NNUEStats
		 */
		NNUEStats::NNUEStats() :
				refresh("refresh"),
				update("update "),
				forward("forward")
		{
		}
		std::string NNUEStats::toString() const
		{
			std::string result = "----NNUEStats----\n";
			result += refresh.toString() + '\n';
			result += update.toString() + '\n';
			result += forward.toString() + '\n';
			return result;
		}

		/*
		 * InferenceNNUE
		 */
		InferenceNNUE::InferenceNNUE(GameConfig gameConfig, const NNUEWeights &weights) :
				game_config(gameConfig),
				accumulator_stack(gameConfig.rows * gameConfig.cols, Accumulator<int16_t>(weights.layer_0.neurons())),
				weights(weights)
		{
			removed_features.reserve(128);
			added_features.reserve(1024);
			init_functions();
		}
		void InferenceNNUE::refresh(const PatternCalculator &calc)
		{
//			TimerGuard tg(stats.refresh);

//			current_depth = 0;
			current_depth = calc.getCurrentDepth();

			added_features.clear();
			if (calc.getSignToMove() == Sign::CROSS)
				added_features.push_back(0);

			for (ThreatType tt = ThreatType::OPEN_3; tt <= ThreatType::FIVE; tt = increment_threat_type(tt))
			{
				const LocationList &cross_threats = calc.getThreatHistogram(Sign::CROSS).get(tt);
				for (auto iter = cross_threats.begin(); iter < cross_threats.end(); iter++)
					added_features.push_back(get_row_index(*iter) + 0 + static_cast<int>(tt) - 2);

				const LocationList &circle_threats = calc.getThreatHistogram(Sign::CIRCLE).get(tt);
				for (auto iter = circle_threats.begin(); iter < circle_threats.end(); iter++)
					added_features.push_back(get_row_index(*iter) + 7 + static_cast<int>(tt) - 2);
			}
			for (int i = 0; i < calc.getBoard().size(); i++)
				if (calc.getBoard()[i] != Sign::NONE)
					added_features.push_back(1 + 16 * i + 14 + static_cast<int>(calc.getBoard()[i]) - 1);

			refresh_function(weights.layer_0, get_current_accumulator(), added_features);
			added_features.clear();
//			for (int i = 0; i < accumulator.size(); i++)
//				std::cout << accumulator.data()[i] << ' ';
//			std::cout << '\n';
		}
		void InferenceNNUE::update(const PatternCalculator &calc)
		{
//			TimerGuard tg(stats.update);

			const bool is_accumulator_ready = current_depth >= calc.getCurrentDepth() or calc.getCurrentDepth() == 0;
			current_depth = calc.getCurrentDepth();
			if (is_accumulator_ready)
				return;
//
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
					ThreatType tt = iter->previous.forCross();
					if (ThreatType::OPEN_3 <= tt and tt <= ThreatType::FIVE)
						removed_features.push_back(base_row_index + 0 + static_cast<int>(tt) - 2);
					tt = iter->current.forCross();
					if (ThreatType::OPEN_3 <= tt and tt <= ThreatType::FIVE)
						added_features.push_back(base_row_index + 0 + static_cast<int>(tt) - 2);
				}
				if (iter->previous.forCircle() != iter->current.forCircle())
				{
					ThreatType tt = iter->previous.forCircle();
					if (ThreatType::OPEN_3 <= tt and tt <= ThreatType::FIVE)
						removed_features.push_back(base_row_index + 7 + static_cast<int>(tt) - 2);
					tt = iter->current.forCircle();
					if (ThreatType::OPEN_3 <= tt and tt <= ThreatType::FIVE)
						added_features.push_back(base_row_index + 7 + static_cast<int>(tt) - 2);
				}
			}

			const Change<Sign> last_move = calc.getChangeOfMoves();
			const int base_row_index = get_row_index(last_move.location);
			const Sign previous = last_move.previous;
			const Sign current = last_move.current;
			if (previous == Sign::NONE and current != Sign::NONE) // stone was placed
				added_features.push_back(base_row_index + 14 + static_cast<int>(current) - 1);
			else
			{
				assert(previous != Sign::NONE and current == Sign::NONE); // stone was removed
				removed_features.push_back(base_row_index + 14 + static_cast<int>(previous) - 1);
			}

			update_function(weights.layer_0, get_previous_accumulator(), get_current_accumulator(), removed_features, added_features);

//			for (int i = 0; i < accumulator.size(); i++)
//				std::cout << accumulator.data()[i] << ' ';
//			std::cout << '\n';
		}
		float InferenceNNUE::forward()
		{
//			TimerGuard tg(stats.forward);

//			std::cout << "added   : ";
//			for (size_t i = 0; i < added_features.size(); i++)
//				std::cout << added_features[i] << ' ';
//			std::cout << '\n' << "removed : ";
//			for (size_t i = 0; i < removed_features.size(); i++)
//				std::cout << removed_features[i] << ' ';
//			std::cout << '\n';

//			size_t i = 0;
//			while (i < added_features.size())
//			{
//				bool found_duplicate = false;
//				for (size_t j = 0; j < removed_features.size(); j++)
//					if (added_features[i] == removed_features[j])
//					{ // found an element that appears in both lists
//						added_features.erase(added_features.begin() + i);
//						removed_features.erase(removed_features.begin() + j);
//						found_duplicate = true;
//						break;
//					}
//				if (not found_duplicate)
//					i++;
//			}

//			std::cout << "added   : ";
//			for (size_t i = 0; i < added_features.size(); i++)
//				std::cout << added_features[i] << ' ';
//			std::cout << '\n' << "removed : ";
//			for (size_t i = 0; i < removed_features.size(); i++)
//				std::cout << removed_features[i] << ' ';
//			std::cout << '\n' << '\n';

//			update_function(weights.layer_0, get_previous_accumulator(), get_current_accumulator(), removed_features, added_features);
//			added_features.clear();
//			removed_features.clear();

			return forward_function(get_current_accumulator(), weights.layer_1, weights.fp32_layers);
		}
		void InferenceNNUE::print_stats() const
		{
			std::cout << stats.toString() << '\n';
		}
		/*
		 * private
		 */
		void InferenceNNUE::init_functions() noexcept
		{
			if (ml::Device::cpuSimdLevel() >= ml::CpuSimd::AVX2)
			{
				refresh_function = avx2_refresh_accumulator;
				update_function = avx2_update_accumulator;
				forward_function = avx2_forward;
			}
			else
			{
				if (ml::Device::cpuSimdLevel() >= ml::CpuSimd::SSE2)
				{
					refresh_function = sse41_refresh_accumulator;
					update_function = sse41_update_accumulator;
					forward_function = sse41_forward;
				}
				else
				{
					refresh_function = def_refresh_accumulator;
					update_function = def_update_accumulator;
					forward_function = def_forward;
				}
			}
		}
		Accumulator<int16_t>& InferenceNNUE::get_current_accumulator()
		{
			return accumulator_stack[current_depth];
//			return accumulator_stack[0];
		}
		Accumulator<int16_t>& InferenceNNUE::get_previous_accumulator()
		{
			assert(current_depth > 0);
			return accumulator_stack[current_depth - 1];
//			return accumulator_stack[0];
		}
		int InferenceNNUE::get_row_index(Location loc) const noexcept
		{
			return 1 + (loc.row * game_config.cols + loc.col) * 16;
		}
	} /* namespace nnue */
} /* namespace ag */

