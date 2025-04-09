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

	ag::float3 to_float3(float x) noexcept
	{
		return ag::float3 { x, x, x };
	}
	ag::float3 to_float3(const ag::Value &value) noexcept
	{
		return ag::float3 { value.win_rate, value.draw_rate, value.loss_rate() };
	}
	ag::Value from_float3(const ag::float3 &f) noexcept
	{
		return ag::Value(f.x, f.y);
	}

	struct Bucket
	{
			int start = 0;
			int end = 0;

			Bucket() noexcept = default;
			Bucket(int s, int e) noexcept :
					start(s),
					end(e)
			{
			}
			bool is_inside(int i) const noexcept
			{
				return start <= i and i <= end;
			}
			float get_center() const noexcept
			{
				return 0.5f * (start + end);
			}
	};

	const std::vector<Bucket>& get_buckets()
	{
		static const std::vector<Bucket> table = { Bucket(0, 0), Bucket(1, 1), Bucket(2, 2), Bucket(3, 3), Bucket(4, 4), Bucket(5, 5), Bucket(6, 6),
				Bucket(7, 7), Bucket(8, 8), Bucket(9, 9), Bucket(10, 11), Bucket(12, 13), Bucket(14, 15), Bucket(16, 17), Bucket(18, 19), Bucket(20,
						21), Bucket(22, 23), Bucket(24, 25), Bucket(26, 27), Bucket(28, 29), Bucket(30, 34), Bucket(35, 39), Bucket(40, 44), Bucket(
						45, 49), Bucket(50, 54), Bucket(55, 59), Bucket(60, 64), Bucket(65, 69), Bucket(70, 74), Bucket(75, 79), Bucket(80, 89),
				Bucket(90, 99), Bucket(100, 109), Bucket(110, 119), Bucket(120, 129), Bucket(130, 139), Bucket(140, 149), Bucket(150, 159), Bucket(
						160, 169), Bucket(170, 179), Bucket(180, 204), Bucket(205, 229), Bucket(230, 254), Bucket(255, 279), Bucket(280, 304), Bucket(
						305, 329), Bucket(330, 354), Bucket(355, 379), Bucket(380, 404), Bucket(405, 429) };
		return table;
	}

	int move_to_bucket_number(int m) noexcept
	{
		assert(m >= 0);
		const std::vector<Bucket> &table = get_buckets();
		for (size_t i = 0; i < table.size(); i++)
			if (table[i].is_inside(m))
				return i;
		return table.size() - 1;
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
		assert(0 <= index && index < getBatchSize());
		std::memcpy(get_pointer(input_on_cpu, { index, 0, 0, 0 }), features.data(), features.sizeInBytes());
	}

	void NetworkDataPack::packPolicyTarget(int index, const matrix<float> &target)
	{
		assert(0 <= index && index < getBatchSize());
		ml::Tensor &tensor = getTarget('p');
		ml::convertType(context_on_cpu, get_pointer(tensor, { index, 0 }), tensor.dtype(), target.data(), ml::DataType::FLOAT32, target.size());
	}
	void NetworkDataPack::packValueTarget(int index, Value target)
	{
		assert(0 <= index && index < getBatchSize());
		const float3 tmp = to_float3(target);
		ml::Tensor &tensor = getTarget('v');
		ml::convertType(context_on_cpu, get_pointer(tensor, { index, 0 }), tensor.dtype(), &tmp, ml::DataType::FLOAT32, 3);
	}
	void NetworkDataPack::packMovesLeftTarget(int index, int target)
	{
		assert(0 <= index && index < getBatchSize());
		ml::Tensor &tensor = getTarget('m');
		std::memset(get_pointer(tensor, { index, 0 }), 0, tensor.lastDim() * ml::sizeOf(tensor.dtype()));
		tensor.at( { index, move_to_bucket_number(target) }) = 1.0f;
	}
	void NetworkDataPack::packActionValuesTarget(int index, const matrix<Value> &target, const matrix<float> &mask)
	{
		assert(0 <= index && index < getBatchSize());
		workspace.resize(target.size());

		for (int i = 0; i < target.size(); i++)
			workspace[i] = to_float3(target[i]);
		ml::Tensor &tensor = getTarget('q');
		ml::convertType(context_on_cpu, get_pointer(tensor, { index, 0, 0, 0 }), tensor.dtype(), workspace.data(), ml::DataType::FLOAT32,
				3 * workspace.size());

		for (int i = 0; i < mask.size(); i++)
			workspace[i] = to_float3(mask[i]);
		ml::Tensor &_mask = getMask('q');
		ml::convertType(context_on_cpu, get_pointer(_mask, { index, 0, 0, 0 }), _mask.dtype(), workspace.data(), ml::DataType::FLOAT32,
				3 * workspace.size());
	}

	void NetworkDataPack::unpackPolicy(int index, matrix<float> &policy) const
	{
		assert(0 <= index && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('p');
		ml::convertType(context_on_cpu, policy.data(), ml::DataType::FLOAT32, get_pointer(tensor, { index, 0 }), tensor.dtype(), policy.size());
	}
	void NetworkDataPack::unpackValue(int index, Value &value) const
	{
		assert(0 <= index && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('v');
		float3 tmp;
		ml::convertType(context_on_cpu, &tmp, ml::DataType::FLOAT32, get_pointer(tensor, { index, 0 }), tensor.dtype(), 3);
		value = from_float3(tmp);
	}
	void NetworkDataPack::unpackMovesLeft(int index, float &movesLeft) const
	{
		assert(0 <= index && index < getBatchSize());
		const ml::Tensor &tensor = getOutput('m');
		const int size = tensor.lastDim();
		std::vector<float> tmp(size);
		ml::convertType(context_on_cpu, tmp.data(), ml::DataType::FLOAT32, get_pointer(tensor, { index, 0 }), tensor.dtype(), size);
		float result = 0.0f;
		for (int i = 0; i < size; i++)
			result += tmp[i] * get_buckets()[i].get_center();
		movesLeft = result;
	}
	void NetworkDataPack::unpackActionValues(int index, matrix<Value> &actionValues) const
	{
		assert(0 <= index && index < getBatchSize());
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

			for (size_t i = 0; i < masks_on_cpu.size(); i++)
				masks_on_cpu.at(i).pageLock();
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
	ml::Tensor& NetworkDataPack::getMask(char c)
	{
		if (masks_on_cpu.empty())
			allocate_mask_tensors();
		switch (c)
		{
			case 'p':
				return masks_on_cpu.at(0);
			case 'v':
				return masks_on_cpu.at(1);
			case 'm':
				return masks_on_cpu.at(2);
			case 'q':
				return masks_on_cpu.at(3);
			default:
				throw std::logic_error("unknown mask type '" + std::string(1, c) + "'");
		}
	}
	const ml::Tensor& NetworkDataPack::getMask(char c) const
	{
		switch (c)
		{
			case 'p':
				return masks_on_cpu.at(0);
			case 'v':
				return masks_on_cpu.at(1);
			case 'm':
				return masks_on_cpu.at(2);
			case 'q':
				return masks_on_cpu.at(3);
			default:
				throw std::logic_error("unknown mask type '" + std::string(1, c) + "'");
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
	void NetworkDataPack::allocate_mask_tensors()
	{
		masks_on_cpu.clear();
		for (size_t i = 0; i < outputs_on_cpu.size(); i++)
			masks_on_cpu.push_back(ml::ones_like(outputs_on_cpu.at(i)));
	}

	std::vector<float> getAccuracy(int batchSize, const NetworkDataPack &pack, int top_k)
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(pack.getGameConfig().rows, pack.getGameConfig().cols);
		matrix<float> answer = empty_like(output);
		for (int b = 0; b < batchSize; b++)
		{
			ml::convertType(ml::Context(), output.data(), ml::DataType::FLOAT32, get_pointer(pack.getOutput('p'), { b, 0 }),
					pack.getOutput('p').dtype(), output.size());
			ml::convertType(ml::Context(), answer.data(), ml::DataType::FLOAT32, get_pointer(pack.getTarget('p'), { b, 0 }),
					pack.getTarget('p').dtype(), answer.size());

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

