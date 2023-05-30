/*
 * layers.hpp
 *
 *  Created on: Oct 19, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_NNUE_OPS_LAYERS_HPP_
#define ALPHAGOMOKU_NETWORKS_NNUE_OPS_LAYERS_HPP_

#include <alphagomoku/utils/AlignedAllocator.hpp>

#include <vector>
#include <cmath>
#include <cinttypes>

class Json;
class SerializedObject;

namespace ag
{
	class PatternCalculator;
}

namespace ag
{
	namespace nnue
	{
		template<typename T>
		class Accumulator
		{
				std::vector<T, AlignedAllocator<T, 64>> m_data;
			public:
				Accumulator() noexcept = default;
				Accumulator(size_t size) :
						m_data(size)
				{
				}
				int size() const noexcept
				{
					return m_data.size();
				}
				const T* data() const noexcept
				{
					return m_data.data();
				}
				T* data() noexcept
				{
					return m_data.data();
				}
		};

		template<typename WeightT, typename BiasT>
		class NnueLayer
		{
				std::vector<WeightT, AlignedAllocator<WeightT, 64>> m_weights;
				std::vector<BiasT, AlignedAllocator<BiasT, 64>> m_bias;
			public:
				NnueLayer() noexcept = default;
				NnueLayer(int inputs, int neurons) :
						m_weights(inputs * neurons),
						m_bias(neurons)
				{
				}
				const WeightT* weights() const noexcept
				{
					return m_weights.data();
				}
				WeightT* weights() noexcept
				{
					return m_weights.data();
				}
				const BiasT* bias() const noexcept
				{
					return m_bias.data();
				}
				BiasT* bias() noexcept
				{
					return m_bias.data();
				}
				int neurons() const noexcept
				{
					return m_bias.size();
				}
				int inputs() const noexcept
				{
					return m_weights.size() / neurons();
				}
		};

		void load_layer(NnueLayer<int8_t, int16_t> &layer, const Json &json, const SerializedObject &so);
		void load_layer(NnueLayer<int16_t, int32_t> &layer, const Json &json, const SerializedObject &so);
		void load_layer(NnueLayer<float, float> &layer, const Json &json, const SerializedObject &so);

		Json save_layer(const NnueLayer<int8_t, int16_t> &layer, SerializedObject &so);
		Json save_layer(const NnueLayer<int16_t, int32_t> &layer, SerializedObject &so);
		Json save_layer(const NnueLayer<float, float> &layer, SerializedObject &so);

		inline float sigmoid(float x) noexcept
		{
			return 1.0f / (1.0f + std::exp(-x));
		}
	}

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NNUE_OPS_LAYERS_HPP_ */
