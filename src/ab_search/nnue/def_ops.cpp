/*
 * def_ops.cpp
 *
 *  Created on: Oct 21, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/def_ops.hpp>

#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cassert>
#include <iostream>

namespace
{
	float relu(float x) noexcept
	{
		return std::max(0.0f, x);
	}
}

namespace ag
{
	namespace nnue
	{

		void def_refresh_accumulator(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept
		{
			assert(layer_1.weights.cols() == accumulator.size());

			const int neurons = layer_1.bias.size();
			for (int j = 0; j < accumulator.size(); j++)
				accumulator[j] = layer_1.bias[j];

			for (size_t i = 0; i < active.size(); i++)
			{
				const int8_t *ptr = layer_1.weights.data() + active[i] * neurons;
				for (int j = 0; j < neurons; j++)
					accumulator[j] += static_cast<int32_t>(ptr[j]);
			}
		}

		void def_update_accumulator(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
				const std::vector<int> &removed, const std::vector<int> &added) noexcept
		{
			assert(layer_1.weights.cols() == oldAccumulator.size());
			assert(layer_1.weights.cols() == newAccumulator.size());

			const int length = layer_1.bias.size();
			for (int j = 0; j < length; j++)
				newAccumulator[j] = oldAccumulator[j];

			for (size_t i = 0; i < removed.size(); i++)
			{
				const int8_t *ptr = layer_1.weights.data() + removed[i] * length;
				for (int j = 0; j < length; j++)
					newAccumulator[j] -= static_cast<int32_t>(ptr[j]);
			}
			for (size_t i = 0; i < added.size(); i++)
			{
				const int8_t *ptr = layer_1.weights.data() + added[i] * length;
				for (int j = 0; j < length; j++)
					newAccumulator[j] += static_cast<int32_t>(ptr[j]);
			}
		}

		float def_forward(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
				const RealSpaceLayer &layer_3) noexcept
		{
			const int input_length = accumulator.size();
			const int middle_length = layer_2.bias.size();
			assert(middle_length <= 32);

			float middle_output[32];
			for (int i = 0; i < middle_length; i++)
				middle_output[i] = layer_2.bias[i];

			for (int i = 0; i < input_length; i++)
			{
				const float input = relu(static_cast<float>(accumulator[i]) * layer_1.scale[i]);
				for (int j = 0; j < middle_length; j++)
					middle_output[j] += input * layer_2.weights[i * middle_length + j];
			}

			for (int i = 0; i < middle_length; i++)
				middle_output[i] = relu(middle_output[i]);

			float output = layer_3.bias[0];
			for (int i = 0; i < middle_length; i++)
				output += middle_output[i] * layer_3.weights[i];
			return approximate_sigmoid(output);
		}
	} /* namespace nnue */
} /* namespace ag */

