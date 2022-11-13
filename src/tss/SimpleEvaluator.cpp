/*
 * SimpleEvaluator.cpp
 *
 *  Created on: Nov 12, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/tss/SimpleEvaluator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <stdexcept>
#include <cassert>
#include <cmath>

namespace
{
	float round_small_to_zero(float x) noexcept
	{
		return (abs(x) < 1.0e-8f) ? 0.0f : x;
	}
}

namespace ag
{
	namespace tss
	{
		Param::Param(int size) :
				weights(size, 0.0f),
				updates(size, 0.0f),
				momentum(size, 0.0f),
				variance(size, 0.0f)
		{
			for (int i = 0; i < size; i++)
				weights[i] = randFloat();
		}
		void Param::addUpdate()
		{
			batch_size++;
		}
		void Param::learn(float learningRate, float weightDecay)
		{
			assert(batch_size > 0);
			steps++;
			const float beta1 = 0.9f;
			const float beta2 = 0.999f;
			if (steps < 10000)
				learningRate *= std::sqrt(1.0f - pow(beta2, steps)) / (1.0f - pow(beta1, steps));

			for (size_t i = 0; i < updates.size(); i++)
			{
				const float update = updates[i] / batch_size + weights[i] * weightDecay;
				momentum[i] = momentum[i] * beta1 + update * (1.0f - beta1);
				variance[i] = variance[i] * beta2 + update * update * (1.0f - beta2);
				weights[i] -= momentum[i] * learningRate / std::sqrt(variance[i] + 1.0e-8f);
				weights[i] = round_small_to_zero(weights[i]);
				updates[i] = 0.0f;
			}
			batch_size = 0;
		}

		float SimpleEvaluator::evaluate(const PatternCalculator &calc)
		{
		}
		Param SimpleEvaluator::init()
		{
			return Param(number_of_params);
		}
		float SimpleEvaluator::train(const PatternCalculator &calc, float target, Param &weights)
		{
			if (weights.size() != number_of_params)
				throw std::logic_error("SimpleEvaluator::train() incorrect size of the weights vector");

			float layer[neurons];
			for (int i = 0; i < neurons; i++)
				layer[i] = weights[i]; // initialize with bias

		}
	}
} /* namespace ag */

