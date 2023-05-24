/*
 * def_ops.cpp
 *
 *  Created on: Oct 21, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/nnue_ops/avx2_ops.hpp>

#include <x86intrin.h>
#include <cassert>

#include <cassert>
#include <iostream>

namespace
{
	using namespace ag::nnue;

	template<typename T>
	T relu(T x) noexcept
	{
		return std::max(T { 0 }, x);
	}

	void layer_1_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1, int32_t *output)
	{
		assert(output != nullptr);

		const int input_length = accumulator.size();
		const int middle_length = layer_1.neurons();
		for (int i = 0; i < middle_length; i++)
			output[i] = layer_1.bias()[i];

		int idx = 0;
		for (int i = 0; i < input_length; i += 2)
		{
			const int32_t input0 = relu(accumulator.data()[i + 0]);
			const int32_t input1 = relu(accumulator.data()[i + 1]);

			for (int j = 0; j < middle_length; j++, idx += 2)
			{
				output[j] += input0 * static_cast<int32_t>(layer_1.weights()[idx + 0]);
				output[j] += input1 * static_cast<int32_t>(layer_1.weights()[idx + 1]);
			}
		}

		for (int i = 0; i < middle_length; i++)
			output[i] = relu(output[i]);
	}
}

namespace ag
{
	namespace nnue
	{

		void def_refresh_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator,
				const std::vector<int> &active) noexcept
		{
			assert(layer_0.neurons() == accumulator.size());
			const int neurons = layer_0.neurons();
			for (int j = 0; j < accumulator.size(); j++)
				accumulator.data()[j] = layer_0.bias()[j];

			for (size_t i = 0; i < active.size(); i++)
			{
				const int8_t *ptr = layer_0.weights() + active[i] * neurons;
				for (int j = 0; j < neurons; j++)
					accumulator.data()[j] += static_cast<int16_t>(ptr[j]);
			}
		}

		void def_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator, const std::vector<int> &removed,
				const std::vector<int> &added) noexcept
		{
			assert(layer_0.neurons() == accumulator.size());

			const int neurons = layer_0.neurons();

			for (size_t i = 0; i < removed.size(); i++)
			{
				const int8_t *ptr = layer_0.weights() + removed[i] * neurons;
				for (int j = 0; j < neurons; j++)
					accumulator.data()[j] -= static_cast<int16_t>(ptr[j]);
			}
			for (size_t i = 0; i < added.size(); i++)
			{
				const int8_t *ptr = layer_0.weights() + added[i] * neurons;
				for (int j = 0; j < neurons; j++)
					accumulator.data()[j] += static_cast<int16_t>(ptr[j]);
			}
		}

		float def_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
				const std::vector<NnueLayer<float, float>> &fp32_layers) noexcept
		{
			const int middle_length = layer_1.neurons();

			assert(middle_length <= 128);
			int32_t output_2[128];
			layer_1_forward(accumulator, layer_1, output_2);

			if (fp32_layers.size() == 1)
			{
				float output = fp32_layers[0].bias()[0];
				for (int i = 0; i < middle_length; i++)
					output += static_cast<float>(output_2[i]) * fp32_layers[0].weights()[i];
				return sigmoid(output);
			}
			else
			{
				float output = 0.0f;
				int idx = 0;
				for (int i = 0; i < fp32_layers[0].neurons(); i++)
				{
					float acc = fp32_layers[0].bias()[i];
					for (int j = 0; j < fp32_layers[0].inputs(); j++, idx++)
						acc += output_2[i] * fp32_layers[0].weights()[idx];
					output += relu(acc) * fp32_layers[1].weights()[i];
				}
				return sigmoid(output + fp32_layers[1].bias()[0]);
			}
		}
	} /* namespace nnue */
} /* namespace ag */

