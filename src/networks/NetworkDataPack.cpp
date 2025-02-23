/*
 * NetworkDataPack.cpp
 *
 *  Created on: Feb 21, 2025
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/NetworkDataPack.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <alphagomoku/networks/networks.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/core/Event.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/Tensor.hpp>
#include <minml/core/math.hpp>
#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <algorithm>
#include <memory>
#include <cassert>
#include <cstring>
#include <string>
#include <bitset>

namespace
{
	void* get_pointer(ml::Tensor &src, std::initializer_list<int> idx)
	{
		assert(src.device().isCPU());
		return reinterpret_cast<uint8_t*>(src.data()) + ml::sizeOf(src.dtype()) * src.getIndexOf(idx);
	}
	const void* get_pointer(const ml::Tensor &src, std::initializer_list<int> idx)
	{
		assert(src.device().isCPU());
		return reinterpret_cast<const uint8_t*>(src.data()) + ml::sizeOf(src.dtype()) * src.getIndexOf(idx);
	}

	ag::float3 to_float3(const ag::Value &value) noexcept
	{
		return ag::float3 { value.win_rate, value.draw_rate, value.loss_rate() };
	}
	ag::Value from_float3(const ag::float3 &f) noexcept
	{
		return ag::Value(f.x, f.y);
	}

	int move_to_bucket_number(int m) noexcept
	{
		static const std::vector<int> table = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
				19, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 26,
				26, 26, 26, 26, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31,
				31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 34, 34, 34, 34, 34, 34, 34,
				34, 34, 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
				38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
				40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
				41, 41, 41, 41, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 43, 43, 43, 43,
				43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
				44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
				45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 47, 47, 47,
				47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
				48, 48, 48, 48, 48, 48, 48, 48, 48, 48 };

		return table[std::max(0, std::min((int) table.size() - 1, m))];
	}
	float bucket_center(int idx) noexcept
	{
		static const std::vector<float> table = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 11.0f, 13.0f, 15.0f, 17.0f, 19.0f,
				21.0f, 23.0f, 25.0f, 27.0f, 29.0f, 32.0f, 38.0f, 43.0f, 48.0f, 53.0f, 58.0f, 63.0f, 68.0f, 73.0f, 78.0f, 85.0f, 95.0f, 105.0f, 115.0f,
				125.0f, 135.0f, 145.0f, 155.0f, 165.0f, 175.0f, 192.5f, 217.5f, 242.5f, 267.5f, 292.5f, 317.5f, 342.5f, 367.5f, 392.5f };

		return table[std::max(0, std::min((int) table.size() - 1, idx))];
	}
}

namespace ag
{
	NetworkDataPack::NetworkDataPack(const GameConfig &cfg, int batchSize, ml::DataType dtype) :
			game_config(cfg)
	{
		const ml::Shape input_shape( { batchSize, cfg.rows, cfg.cols, 1 });
		const ml::Shape policy_shape( { batchSize, cfg.rows * cfg.cols });
		const ml::Shape value_shape( { batchSize, 3 });
		const ml::Shape moves_left_shape( { batchSize, 50 });
		const ml::Shape action_values_shape( { batchSize, cfg.rows, cfg.cols, 3 });

		input_on_cpu = ml::Tensor(input_shape, ml::DataType::INT32, ml::Device::cpu());

		outputs_on_cpu.emplace_back(policy_shape, dtype, ml::Device::cpu());
		outputs_on_cpu.emplace_back(value_shape, dtype, ml::Device::cpu());
		outputs_on_cpu.emplace_back(moves_left_shape, dtype, ml::Device::cpu());
		outputs_on_cpu.emplace_back(action_values_shape, dtype, ml::Device::cpu());
	}
	void NetworkDataPack::packInputData(int index, const matrix<Sign> &board, Sign signToMove)
	{
		get_pattern_calculator().setBoard(board, signToMove);
		input_features.encode(get_pattern_calculator());
		packInputData(index, input_features);
	}
	void NetworkDataPack::packInputData(int index, const NNInputFeatures &features)
	{
		assert(index >= 0 && index < getBatchSize());
		std::memcpy(get_pointer(input_on_cpu, { index, 0, 0, 0 }), features.data(), features.sizeInBytes());
	}

	void NetworkDataPack::packPolicyTarget(int index, const matrix<float> &target)
	{
		assert(index >= 0 && index < getBatchSize());
		std::memcpy(get_pointer(getTarget('p'), { index, 0 }), target.data(), target.sizeInBytes());
	}
	void NetworkDataPack::packValueTarget(int index, Value target)
	{
		assert(index >= 0 && index < getBatchSize());
		const float3 tmp = to_float3(target);
		std::memcpy(get_pointer(getTarget('v'), { index, 0 }), &tmp, sizeof(tmp));
	}
	void NetworkDataPack::packMovesLeftTarget(int index, int target)
	{
		assert(index >= 0 && index < getBatchSize());
		const int last_dim_size = getTarget('m').lastDim() * sizeof(float);
		std::memset(get_pointer(getTarget('m'), { index, 0 }), 0, last_dim_size);
		getTarget('m').at( { index, move_to_bucket_number(target) }) = 1.0f;
	}
	void NetworkDataPack::packActionValuesTarget(int index, const matrix<Value> &target)
	{
		assert(index >= 0 && index < getBatchSize());
		workspace.resize(game_config.rows * game_config.cols);
		for (int i = 0; i < target.size(); i++)
			workspace[i] = to_float3(target[i]);
		std::memcpy(get_pointer(getTarget('q'), { index, 0, 0, 0 }), workspace.data(), sizeof(float3) * workspace.size());
	}

	void NetworkDataPack::unpackPolicy(int index, matrix<float> &policy) const
	{
		assert(index >= 0 && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('p');
		ml::convertType(context_on_cpu, policy.data(), ml::DataType::FLOAT32, get_pointer(tensor, { index, 0 }), tensor.dtype(), policy.size());
	}
	void NetworkDataPack::unpackValue(int index, Value &value) const
	{
		assert(index >= 0 && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('v');
		float3 tmp;
		ml::convertType(context_on_cpu, &tmp, ml::DataType::FLOAT32, get_pointer(tensor, { index, 0 }), tensor.dtype(), 3);
		value = from_float3(tmp);
	}
	void NetworkDataPack::unpackMovesLeft(int index, float &movesLeft) const
	{
		assert(index >= 0 && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('m');
		const int size = tensor.lastDim();
		std::vector<float> tmp(size);
		ml::convertType(context_on_cpu, tmp.data(), ml::DataType::FLOAT32, get_pointer(tensor, { index, 0 }), tensor.dtype(), size);
		float result = 0.0f;
		for (int i = 0; i < size; i++)
			result += tmp[i] * bucket_center(i);
		movesLeft = result;
	}
	void NetworkDataPack::unpackActionValues(int index, matrix<Value> &actionValues) const
	{
		assert(index >= 0 && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('q');
		workspace.resize(game_config.rows * game_config.cols);
		ml::convertType(context_on_cpu, workspace.data(), ml::DataType::FLOAT32, get_pointer(tensor, { index, 0, 0, 0 }), tensor.dtype(),
				3 * workspace.size());
		for (int i = 0; i < actionValues.size(); i++)
			actionValues[i] = from_float3(workspace[i]);
	}
	void NetworkDataPack::pinMemory()
	{
		if (not input_on_cpu.isPageLocked())
		{
			input_on_cpu.pageLock();
			for (size_t i = 0; i < outputs_on_cpu.size(); i++)
				outputs_on_cpu.at(i).pageLock();

			for (size_t i = 0; i < targets_on_cpu.size(); i++)
				targets_on_cpu.at(i).pageLock();
		}
	}
	const ml::Tensor& NetworkDataPack::getInput() const
	{
		return input_on_cpu;
	}
	ml::Tensor& NetworkDataPack::getOutput(char c)
	{
		switch (c)
		{
			case 'p':
				return outputs_on_cpu.at(0);
			case 'v':
				return outputs_on_cpu.at(1);
			case 'm':
				return outputs_on_cpu.at(2);
			case 'q':
				return outputs_on_cpu.at(3);
			default:
				throw std::logic_error("unknown output type '" + std::string(1, c) + "'");
		}
	}
	const ml::Tensor& NetworkDataPack::getOutput(char c) const
	{
		switch (c)
		{
			case 'p':
				return outputs_on_cpu.at(0);
			case 'v':
				return outputs_on_cpu.at(1);
			case 'm':
				return outputs_on_cpu.at(2);
			case 'q':
				return outputs_on_cpu.at(3);
			default:
				throw std::logic_error("unknown output type '" + std::string(1, c) + "'");
		}
	}
	ml::Tensor& NetworkDataPack::getTarget(char c)
	{
		if (targets_on_cpu.empty())
			allocate_target_tensors();
		switch (c)
		{
			case 'p':
				return targets_on_cpu.at(0);
			case 'v':
				return targets_on_cpu.at(1);
			case 'm':
				return targets_on_cpu.at(2);
			case 'q':
				return targets_on_cpu.at(3);
			default:
				throw std::logic_error("unknown target type '" + std::string(1, c) + "'");
		}
	}
	const ml::Tensor& NetworkDataPack::getTarget(char c) const
	{
		switch (c)
		{
			case 'p':
				return targets_on_cpu.at(0);
			case 'v':
				return targets_on_cpu.at(1);
			case 'm':
				return targets_on_cpu.at(2);
			case 'q':
				return targets_on_cpu.at(3);
			default:
				throw std::logic_error("unknown target type '" + std::string(1, c) + "'");
		}
	}
	GameConfig NetworkDataPack::getGameConfig() const noexcept
	{
		return game_config;
	}
	int NetworkDataPack::getBatchSize() const noexcept
	{
		return input_on_cpu.firstDim();
	}
	/*
	 * private
	 */
	PatternCalculator& NetworkDataPack::get_pattern_calculator()
	{
		if (pattern_calculator == nullptr)
		{
			pattern_calculator = std::make_unique<PatternCalculator>(game_config);
			input_features = NNInputFeatures(game_config.rows, game_config.cols);
		}
		return *pattern_calculator;
	}
	void NetworkDataPack::allocate_target_tensors()
	{
		targets_on_cpu.clear();
		for (size_t i = 0; i < outputs_on_cpu.size(); i++)
			targets_on_cpu.push_back(ml::zeros_like(outputs_on_cpu.at(i)));
	}

	std::vector<float> getAccuracy(int batchSize, const NetworkDataPack &pack, int top_k)
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(pack.getGameConfig().rows, pack.getGameConfig().cols);
		matrix<float> answer = empty_like(output);
		for (int b = 0; b < batchSize; b++)
		{
			std::memcpy(output.data(), get_pointer(pack.getOutput('p'), { b, 0 }), output.sizeInBytes());
			std::memcpy(answer.data(), get_pointer(pack.getTarget('p'), { b, 0 }), output.sizeInBytes());

			Move correct = pickMove(answer);
			for (int l = 0; l < top_k; l++)
			{
				Move best = pickMove(output);
				if (correct == best)
					for (int m = l; m < top_k; m++)
						result.at(1 + m) += 1;
				output.at(best.row, best.col) = 0.0f;
			}
			result.at(0) += 1;
		}
		return result;
	}

} /* namespace ag */

