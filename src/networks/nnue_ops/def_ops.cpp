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

		void def_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, const Accumulator<int16_t> &oldAccumulator,
				Accumulator<int16_t> &newAccumulator, const std::vector<int> &removed, const std::vector<int> &added) noexcept
		{
			assert(layer_0.neurons() == oldAccumulator.size());
			assert(layer_0.neurons() == newAccumulator.size());

			const int neurons = layer_0.neurons();

			for (int j = 0; j < neurons; j++)
				newAccumulator.data()[j] = oldAccumulator.data()[j];

			for (size_t i = 0; i < removed.size(); i++)
			{
				const int8_t *ptr = layer_0.weights() + removed[i] * neurons;
				for (int j = 0; j < neurons; j++)
					newAccumulator.data()[j] -= static_cast<int16_t>(ptr[j]);
			}
			for (size_t i = 0; i < added.size(); i++)
			{
				const int8_t *ptr = layer_0.weights() + added[i] * neurons;
				for (int j = 0; j < neurons; j++)
					newAccumulator.data()[j] += static_cast<int16_t>(ptr[j]);
			}
		}

		float def_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
				const std::vector<NnueLayer<float, float>> &fp32_layers) noexcept
		{
//			for (int i = 0; i < accumulator.size(); i++)
//				std::cout << accumulator.data()[i] << ' ';
//			std::cout << '\n';

			const int inputs = accumulator.size();
			const int neurons = layer_1.neurons();
			assert(neurons <= 128);
			int32_t output_2[128];

			for (int i = 0; i < neurons; i++)
				output_2[i] = layer_1.bias()[i];

			int idx = 0;
			for (int i = 0; i < inputs; i += 2)
			{
				const int32_t input0 = relu(accumulator.data()[i + 0]);
				const int32_t input1 = relu(accumulator.data()[i + 1]);
				for (int j = 0; j < neurons; j++, idx += 2)
				{
					output_2[j] += input0 * static_cast<int32_t>(layer_1.weights()[idx + 0]);
					output_2[j] += input1 * static_cast<int32_t>(layer_1.weights()[idx + 1]);
				}
			}
			for (int i = 0; i < neurons; i++)
				output_2[i] = relu(output_2[i]);

//			for (int i = 0; i < neurons; i++)
//				std::cout << output_2[i] << ' ';
//			std::cout << '\n';

			if (fp32_layers.size() == 1)
			{
				const int inputs = fp32_layers[0].inputs();

				float output = fp32_layers[0].bias()[0];
				for (int i = 0; i < inputs; i++)
					output += static_cast<float>(output_2[i]) * fp32_layers[0].weights()[i];
				return sigmoid(output);
			}
			else
			{
				assert(fp32_layers.size() == 2);

				const int inputs = layer_1.neurons();
				const int neurons = fp32_layers[0].neurons();
				assert(neurons <= 32);
				float output_3[32];

				for (int i = 0; i < neurons; i++)
					output_3[i] = fp32_layers[0].bias()[i];

				int idx = 0;
				for (int i = 0; i < inputs; i++)
				{
					const float in = static_cast<float>(output_2[i]);
					for (int j = 0; j < neurons; j++, idx++)
						output_3[j] += in * fp32_layers[0].weights()[idx];
				}
				for (int i = 0; i < neurons; i++)
					std::cout << relu(output_3[i]) << ' ';
				std::cout << '\n';

				float output = fp32_layers[1].bias()[0];
				for (int i = 0; i < neurons; i++)
					output += relu(output_3[i]) * fp32_layers[1].weights()[i];
//				std::cout << sigmoid(output) << '\n';
				return sigmoid(output);
			}
		}
	} /* namespace nnue */
} /* namespace ag */

