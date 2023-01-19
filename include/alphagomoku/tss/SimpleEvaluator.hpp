/*
 * SimpleEvaluator.hpp
 *
 *  Created on: Nov 12, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TSS_SIMPLEEVALUATOR_HPP_
#define ALPHAGOMOKU_TSS_SIMPLEEVALUATOR_HPP_

#include <cinttypes>
#include <cstddef>
#include <vector>

namespace ag
{
	class PatternCalculator;
}
namespace ag
{
	namespace tss
	{

		class Param
		{
				std::vector<float> weights;
				std::vector<float> updates;
				std::vector<float> momentum;
				std::vector<float> variance;
				int batch_size = 0;
				int steps = 0;
			public:
				Param(int size);
				void addUpdate();
				void learn(float learningRate, float weightDecay);
				size_t size() const noexcept
				{
					return weights.size();
				}
				const float* data() const noexcept
				{
					return weights.data();
				}
				float* data() noexcept
				{
					return weights.data();
				}
				const float* update() const noexcept
				{
					return updates.data();
				}
				float* update() noexcept
				{
					return updates.data();
				}
				float operator[](int index) const noexcept
				{
					return weights[index];
				}
				float& operator[](int index) noexcept
				{
					return weights[index];
				}
		};

		class SimpleEvaluator
		{
			public:
				static constexpr int inputs = 64;
				static constexpr int neurons = 8;

				/*
				 * \brief Evaluates given board state.
				 * Predicts probability of finding a win.
				 */
				static float evaluate(const PatternCalculator &calc);

				static Param init();
				static float train(const PatternCalculator &calc, float target, Param &weights);
			private:
				static constexpr int number_of_params = (inputs + 1) * neurons + neurons + 1;
		};

	}
} /* namespace ag */

#endif /* ALPHAGOMOKU_TSS_SIMPLEEVALUATOR_HPP_ */
