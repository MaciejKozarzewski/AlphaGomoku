/*
 * layers.hpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_NNUE_LAYERS_HPP_
#define ALPHAGOMOKU_AB_SEARCH_NNUE_LAYERS_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/configs.hpp>

namespace ag
{
	class PatternCalculator;
	class Value;
	namespace nnue
	{
		class QuantizedLayer;
		class RealSpaceLayer;
	}
}

namespace ag
{
	namespace nnue
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
				Param(int size, float scale = 1.0f, bool gaussian = true);
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

		class InputLayerThreats
		{
				GameConfig game_config;
				int inputs, neurons;
				Param weights;
				Param bias;
				std::vector<float> output;
			public:
				/*
				 * row 0 - is cross (black) to move. If set to 0 it means that circle (white) is moving.
				 * Input encoding
				 *   0:9 - cross (black) threats without none
				 *   9:18 - circle (white) threats without none
				 *     18 - is cross (black) stone
				 *     19 - is circle (white) stone
				 */
				static constexpr int params_per_spot = 20;
				InputLayerThreats(GameConfig gameConfig, int neurons);
				void forward(const PatternCalculator &calc);
				void backward(const PatternCalculator &calc, std::vector<float> &gradient);
				void learn(float learningRate, float weightDecay = 0.0f);
				const std::vector<float>& getOutput() const noexcept;
				QuantizedLayer dump_quantized() const;
				RealSpaceLayer dump_real() const;
		};

		class InputLayerPatterns
		{
				/*
				 * Input encoding
				 *   0: 7 - cross (black) patterns horizontal
				 *   7:14 - cross (black) patterns vertical
				 *  14:21 - cross (black) patterns diagonal
				 *  21:28 - cross (black) patterns antidiagonal
				 *  28:56 - circle (white) patterns
				 *     56 - is empty spot
				 *     57 - is cross (black) stone
				 *     58 - is circle (white) stone
				 *     59 - is cross (black) to move. If set to 0 it means that circle (white) is moving.
				 */
				static constexpr int params_per_spot = 58;
				GameConfig game_config;
				int inputs, neurons;
				Param weights;
				Param bias;
				std::vector<float> output;
			public:
				InputLayerPatterns(GameConfig gameConfig, int neurons);
				void forward(const PatternCalculator &calc);
				void backward(const PatternCalculator &calc, std::vector<float> &gradient);
				void learn(float learningRate, float weightDecay = 0.0f);
				const std::vector<float>& getOutput() const noexcept;
				QuantizedLayer dump_quantized() const;
				RealSpaceLayer dump_real() const;
		};

		class MiddleLayer
		{
				int inputs, neurons;
				Param weights;
				Param bias;
				std::vector<float> input;
				std::vector<float> output;
				std::vector<float> gradient_prev;
			public:
				MiddleLayer(int inputs, int neurons);
				void forward(const std::vector<float> &input);
				void backward(std::vector<float> &gradientNext);
				void learn(float learningRate, float weightDecay = 0.0f);
				const std::vector<float>& getOutput() const noexcept;
				std::vector<float>& getGradientPrev() noexcept;
				QuantizedLayer dump_quantized() const;
				RealSpaceLayer dump_real() const;
		};

		class OutputLayerSoftmax
		{
				int inputs, neurons;
				Param weights;
				Param bias;
				std::vector<float> input;
				std::vector<float> output;
				std::vector<float> gradient_prev;
			public:
				OutputLayerSoftmax(int inputs);
				void forward(const std::vector<float> &input);
				void backward(const std::vector<float> &gradientNext);
				void learn(float learningRate, float weightDecay = 0.0f);
				Value getOutput() const noexcept;
				std::vector<float>& getGradientPrev() noexcept;
				QuantizedLayer dump_quantized() const;
				RealSpaceLayer dump_real() const;
		};
		class OutputLayerSigmoid
		{
				int inputs, neurons;
				Param weights;
				Param bias;
				std::vector<float> input;
				std::vector<float> output;
				std::vector<float> gradient_prev;
			public:
				OutputLayerSigmoid(int inputs);
				void forward(const std::vector<float> &input);
				void backward(const std::vector<float> &gradientNext);
				void learn(float learningRate, float weightDecay = 0.0f);
				float getOutput() const noexcept;
				std::vector<float>& getGradientPrev() noexcept;
				QuantizedLayer dump_quantized() const;
				RealSpaceLayer dump_real() const;
		};
	}
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_NNUE_LAYERS_HPP_ */
