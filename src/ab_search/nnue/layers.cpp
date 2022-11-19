/*
 * layers.cpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/layers.hpp>
#include <alphagomoku/ab_search/nnue/gemv.hpp>
#include <alphagomoku/ab_search/nnue/wrappers.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/mcts/Value.hpp>

#include <cassert>

namespace
{
	using namespace ag;
	using namespace ag::nnue;
	float round_small_to_zero(float x) noexcept
	{
		return (abs(x) < 1.0e-8f) ? 0.0f : x;
	}

	void get_scales(Wrapper1D<float> &result, const Param &weights, int rows, int cols)
	{
		for (int j = 0; j < cols; j++)
			result[j] = 0.0f;
		for (int i = 0; i < rows; i++)
			for (int j = 0; j < cols; j++)
				result[j] = std::max(result[j], std::abs(weights[i * cols + j]));
		for (int j = 0; j < cols; j++)
			result[j] /= 127.0f;
	}
	void quantize_weights(Wrapper2D<int8_t> &result, const Wrapper1D<float> &scales, const Param &weights)
	{
		for (int i = 0; i < result.rows(); i++)
			for (int j = 0; j < result.cols(); j++)
			{
				const float scaled = weights[i * result.cols() + j] / scales[j];
				const int8_t clipped = std::max(-127, std::min(127, static_cast<int>(scaled)));
				result.at(i, j) = clipped;
			}
	}
	void quantize_bias(Wrapper1D<int32_t> &result, const Wrapper1D<float> &scales, const Param &bias)
	{
		for (int j = 0; j < result.size(); j++)
			result[j] = static_cast<int32_t>(bias[j] / scales[j]);
	}

}

namespace ag
{
	namespace nnue
	{
		Param::Param(int size, float scale, bool gaussian) :
				weights(size, 0.0f),
				updates(size, 0.0f),
				momentum(size, 0.0f),
				variance(size, 0.0f)
		{
			if (gaussian)
				for (int i = 0; i < size; i++)
					weights[i] = randGaussian() * scale;
			else
				for (int i = 0; i < size; i++)
					weights[i] = randFloat() * scale;
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

		InputLayerThreats::InputLayerThreats(GameConfig gameConfig, int neurons) :
				game_config(gameConfig),
				inputs(1 + game_config.rows * game_config.cols * params_per_spot),
				neurons(neurons),
				weights(neurons * inputs, 1.0f / std::sqrt(inputs)),
				bias(neurons, 1.0f, false),
				output(neurons, 0.0f)
		{
		}
		void InputLayerThreats::forward(const PatternCalculator &calc)
		{
			for (int n = 0; n < neurons; n++)
				output[n] = bias[n];
			if (calc.getSignToMove() == Sign::CROSS)
				for (int n = 0; n < neurons; n++)
					output[n] += weights[n];
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
				{
					const int index = 1 + (row * game_config.cols + col) * params_per_spot;
					const ThreatEncoding te = calc.getThreatAt(row, col);
					if (te.forCross() != ThreatType::NONE)
						for (int n = 0; n < neurons; n++)
							output[n] += weights[(index + static_cast<int>(te.forCross()) - 1) * neurons + n];
					if (te.forCircle() != ThreatType::NONE)
						for (int n = 0; n < neurons; n++)
							output[n] += weights[(index + 9 + static_cast<int>(te.forCircle()) - 1) * neurons + n];
					switch (calc.signAt(row, col))
					{
						case Sign::CROSS:
							for (int n = 0; n < neurons; n++)
								output[n] += weights[(index + 18) * neurons + n];
							break;
						case Sign::CIRCLE:
							for (int n = 0; n < neurons; n++)
								output[n] += weights[(index + 19) * neurons + n];
							break;
						default:
							break;
					}
				}

			relu_forward(output);
		}
		void InputLayerThreats::backward(const PatternCalculator &calc, std::vector<float> &gradient)
		{
			bias.addUpdate();
			weights.addUpdate();

			relu_backward(output, gradient);
			for (int n = 0; n < neurons; n++)
				bias.update()[n] += gradient[n];

			if (calc.getSignToMove() == Sign::CROSS)
				for (int n = 0; n < neurons; n++)
					weights.update()[n] += gradient[n];
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
				{
					const int index = 1 + (row * game_config.cols + col) * params_per_spot;
					const ThreatEncoding te = calc.getThreatAt(row, col);
					if (te.forCross() != ThreatType::NONE)
						for (int n = 0; n < neurons; n++)
							weights.update()[(index + static_cast<int>(te.forCross()) - 1) * neurons + n] += gradient[n];
					if (te.forCircle() != ThreatType::NONE)
						for (int n = 0; n < neurons; n++)
							weights.update()[(index + 9 + static_cast<int>(te.forCircle()) - 1) * neurons + n] += gradient[n];
					switch (calc.signAt(row, col))
					{
						case Sign::CROSS:
							for (int n = 0; n < neurons; n++)
								weights.update()[(index + 18) * neurons + n] += gradient[n];
							break;
						case Sign::CIRCLE:
							for (int n = 0; n < neurons; n++)
								weights.update()[(index + 19) * neurons + n] += gradient[n];
							break;
						default:
							break;
					}
				}
		}
		void InputLayerThreats::learn(float learningRate, float weightDecay)
		{
			weights.learn(learningRate, weightDecay);
			bias.learn(learningRate, weightDecay);
		}
		const std::vector<float>& InputLayerThreats::getOutput() const noexcept
		{
			return output;
		}
		QuantizedLayer InputLayerThreats::dump_quantized() const
		{
			QuantizedLayer result(inputs, neurons);
			get_scales(result.scale, weights, inputs, neurons);
			quantize_weights(result.weights, result.scale, weights);
			quantize_bias(result.bias, result.scale, bias);
			return result;
		}
		RealSpaceLayer InputLayerThreats::dump_real() const
		{
			RealSpaceLayer result(inputs, neurons);
			std::copy(weights.data(), weights.data() + weights.size(), result.weights.data());
			std::copy(bias.data(), bias.data() + bias.size(), result.bias.data());
			return result;
		}

		/*
		 *
		 */
		InputLayerPatterns::InputLayerPatterns(GameConfig gameConfig, int neurons) :
				game_config(gameConfig),
				inputs(1 + game_config.rows * game_config.cols * params_per_spot),
				neurons(neurons),
				weights(neurons * inputs, 1.0f / std::sqrt(inputs)),
				bias(neurons, 1.0f, false),
				output(neurons, 0.0f)
		{
		}
		void InputLayerPatterns::forward(const PatternCalculator &calc)
		{
			const int neurons = bias.size();
			for (int n = 0; n < neurons; n++)
				output[n] = bias[n];
			if (calc.getSignToMove() == Sign::CROSS)
				for (int n = 0; n < neurons; n++)
					output[n] += weights[n];
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
				{
					const int index = 1 + (row * game_config.cols + col) * params_per_spot;
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternType pt = calc.getPatternTypeAt(Sign::CROSS, row, col, dir);
						if (pt != PatternType::NONE)
							for (int n = 0; n < neurons; n++)
								output[n] += weights[(index + 7 * dir + static_cast<int>(pt) - 1) * neurons + n];
					}
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternType pt = calc.getPatternTypeAt(Sign::CIRCLE, row, col, dir);
						if (pt != PatternType::NONE)
							for (int n = 0; n < neurons; n++)
								output[n] += weights[(index + 28 + 7 * dir + static_cast<int>(pt) - 1) * neurons + n];
					}
					switch (calc.signAt(row, col))
					{
						case Sign::CROSS:
							for (int n = 0; n < neurons; n++)
								output[n] += weights[(index + 56) * neurons + n];
							break;
						case Sign::CIRCLE:
							for (int n = 0; n < neurons; n++)
								output[n] += weights[(index + 57) * neurons + n];
							break;
						default:
							break;
					}

				}

			relu_forward(output);
		}
		void InputLayerPatterns::backward(const PatternCalculator &calc, std::vector<float> &gradient)
		{
			bias.addUpdate();
			weights.addUpdate();

			relu_backward(output, gradient);
			for (int n = 0; n < neurons; n++)
				bias.update()[n] += gradient[n];

			if (calc.getSignToMove() == Sign::CROSS)
				for (int n = 0; n < neurons; n++)
					weights.update()[n] += gradient[n];
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
				{
					const int index = 1 + (row * game_config.cols + col) * params_per_spot;
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternType pt = calc.getPatternTypeAt(Sign::CROSS, row, col, dir);
						if (pt != PatternType::NONE)
							for (int n = 0; n < neurons; n++)
								weights.update()[(index + 7 * dir + static_cast<int>(pt) - 1) * neurons + n] += gradient[n];
					}
					for (Direction dir = 0; dir < 4; dir++)
					{
						const PatternType pt = calc.getPatternTypeAt(Sign::CIRCLE, row, col, dir);
						if (pt != PatternType::NONE)
							for (int n = 0; n < neurons; n++)
								weights.update()[(index + 28 + 7 * dir + static_cast<int>(pt) - 1) * neurons + n] += gradient[n];
					}
					switch (calc.signAt(row, col))
					{
						case Sign::CROSS:
							for (int n = 0; n < neurons; n++)
								weights.update()[(index + 56) * neurons + n] += gradient[n];
							break;
						case Sign::CIRCLE:
							for (int n = 0; n < neurons; n++)
								weights.update()[(index + 57) * neurons + n] += gradient[n];
							break;
						default:
							break;
					}

				}
		}
		void InputLayerPatterns::learn(float learningRate, float weightDecay)
		{
			weights.learn(learningRate, weightDecay);
			bias.learn(learningRate, weightDecay);
		}
		const std::vector<float>& InputLayerPatterns::getOutput() const noexcept
		{
			return output;
		}
		QuantizedLayer InputLayerPatterns::dump_quantized() const
		{
			QuantizedLayer result(inputs, neurons);
			get_scales(result.scale, weights, inputs, neurons);
			quantize_weights(result.weights, result.scale, weights);
			quantize_bias(result.bias, result.scale, bias);
			return result;
		}
		RealSpaceLayer InputLayerPatterns::dump_real() const
		{
			RealSpaceLayer result(inputs, neurons);
			std::copy(weights.data(), weights.data() + weights.size(), result.weights.data());
			std::copy(bias.data(), bias.data() + bias.size(), result.bias.data());
			return result;
		}

		/*
		 *
		 */
		MiddleLayer::MiddleLayer(int inputs, int neurons) :
				inputs(inputs),
				neurons(neurons),
				weights(neurons * inputs, 1.0f / std::sqrt(inputs)),
				bias(neurons, 1.0f, false),
				output(neurons),
				gradient_prev(inputs)
		{
		}
		void MiddleLayer::forward(const std::vector<float> &input)
		{
			this->input = input;
			gemv(weights.data(), input.data(), output.data(), bias.data(), neurons, inputs);

			relu_forward(output);
		}
		void MiddleLayer::backward(std::vector<float> &gradientNext)
		{
			bias.addUpdate();
			weights.addUpdate();

			relu_backward(output, gradientNext);
			for (int n = 0; n < neurons; n++)
				bias.update()[n] += gradientNext[n];
			gemTv(weights.data(), gradientNext.data(), gradient_prev.data(), weights.update(), input.data(), neurons, inputs);
		}
		void MiddleLayer::learn(float learningRate, float weightDecay)
		{
			weights.learn(learningRate, weightDecay);
			bias.learn(learningRate, weightDecay);
		}
		const std::vector<float>& MiddleLayer::getOutput() const noexcept
		{
			return output;
		}
		std::vector<float>& MiddleLayer::getGradientPrev() noexcept
		{
			return gradient_prev;
		}
		QuantizedLayer MiddleLayer::dump_quantized() const
		{
			QuantizedLayer result(inputs, neurons);
			get_scales(result.scale, weights, inputs, neurons);
			quantize_weights(result.weights, result.scale, weights);
			quantize_bias(result.bias, result.scale, bias);
			return result;
		}
		RealSpaceLayer MiddleLayer::dump_real() const
		{
			RealSpaceLayer result(inputs, neurons);
			std::copy(weights.data(), weights.data() + weights.size(), result.weights.data());
			std::copy(bias.data(), bias.data() + bias.size(), result.bias.data());
			return result;
		}

		/*
		 *
		 */
		OutputLayerSoftmax::OutputLayerSoftmax(int inputs) :
				inputs(inputs),
				neurons(3),
				weights(neurons * inputs, 1.0f / std::sqrt(inputs)),
				bias(neurons, 1.0f, false),
				output(neurons),
				gradient_prev(inputs)
		{
		}
		void OutputLayerSoftmax::forward(const std::vector<float> &input)
		{
			this->input = input;
			gemv_softmax(weights.data(), input.data(), output.data(), bias.data(), inputs);
		}
		void OutputLayerSoftmax::backward(const std::vector<float> &gradientNext)
		{
			bias.addUpdate();
			weights.addUpdate();

			for (int n = 0; n < neurons; n++)
				bias.update()[n] += gradientNext[n];
			gemTv_softmax(weights.data(), gradientNext.data(), gradient_prev.data(), weights.update(), input.data(), inputs);
		}
		void OutputLayerSoftmax::learn(float learningRate, float weightDecay)
		{
			weights.learn(learningRate, weightDecay);
			bias.learn(learningRate, weightDecay);
		}
		Value OutputLayerSoftmax::getOutput() const noexcept
		{
			return Value(output[0], output[1], output[2]);
		}
		std::vector<float>& OutputLayerSoftmax::getGradientPrev() noexcept
		{
			return gradient_prev;
		}
		QuantizedLayer OutputLayerSoftmax::dump_quantized() const
		{
			QuantizedLayer result(inputs, neurons);
			get_scales(result.scale, weights, inputs, neurons);
			quantize_weights(result.weights, result.scale, weights);
			quantize_bias(result.bias, result.scale, bias);
			return result;
		}
		RealSpaceLayer OutputLayerSoftmax::dump_real() const
		{
			RealSpaceLayer result(inputs, neurons);
			std::copy(weights.data(), weights.data() + weights.size(), result.weights.data());
			std::copy(bias.data(), bias.data() + bias.size(), result.bias.data());
			return result;
		}

		/*
		 *
		 */
		OutputLayerSigmoid::OutputLayerSigmoid(int inputs) :
				inputs(inputs),
				neurons(1),
				weights(neurons * inputs, 1.0f / std::sqrt(inputs)),
				bias(neurons, 1.0f, false),
				output(neurons),
				gradient_prev(inputs)
		{
		}
		void OutputLayerSigmoid::forward(const std::vector<float> &input)
		{
			this->input = input;
			output[0] = gemv_sigmoid(weights.data(), input.data(), bias[0], inputs);
		}
		void OutputLayerSigmoid::backward(const std::vector<float> &gradientNext)
		{
			bias.addUpdate();
			weights.addUpdate();

			bias.update()[0] += gradientNext[0];
			gemTv_sigmoid(weights.data(), gradientNext[0], gradient_prev.data(), weights.update(), input.data(), inputs);
		}
		void OutputLayerSigmoid::learn(float learningRate, float weightDecay)
		{
			weights.learn(learningRate, weightDecay);
			bias.learn(learningRate, weightDecay);
		}
		float OutputLayerSigmoid::getOutput() const noexcept
		{
			return output[0];
		}
		std::vector<float>& OutputLayerSigmoid::getGradientPrev() noexcept
		{
			return gradient_prev;
		}
		QuantizedLayer OutputLayerSigmoid::dump_quantized() const
		{
			QuantizedLayer result(inputs, neurons);
			get_scales(result.scale, weights, inputs, neurons);
			quantize_weights(result.weights, result.scale, weights);
			quantize_bias(result.bias, result.scale, bias);
			return result;
		}
		RealSpaceLayer OutputLayerSigmoid::dump_real() const
		{
			RealSpaceLayer result(inputs, neurons);
			std::copy(weights.data(), weights.data() + weights.size(), result.weights.data());
			std::copy(bias.data(), bias.data() + bias.size(), result.bias.data());
			return result;
		}

	} /* namespace nnue */
} /* namespace ag */

