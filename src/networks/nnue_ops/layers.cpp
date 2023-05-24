/*
 * layers.cpp
 *
 *  Created on: May 23, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/nnue_ops/layers.hpp>

#include <minml/utils/serialization.hpp>

namespace
{
	using namespace ag::nnue;

	template<typename T, typename U>
	void load_impl(NnueLayer<T, U> &layer, const Json &json, const SerializedObject &so)
	{
		const int inputs = json["inputs"].getInt();
		const int neurons = json["neurons"].getInt();

		layer = NnueLayer<T, U>(inputs, neurons);
		so.load(layer.weights(), json["weights_offset"], sizeof(T) * inputs * neurons);
		so.load(layer.bias(), json["bias_offset"], sizeof(U) * neurons);
	}
	template<typename T, typename U>
	Json save_impl(const NnueLayer<T, U> &layer, SerializedObject &so)
	{
		Json json;
		json["inputs"] = layer.inputs();
		json["neurons"] = layer.neurons();
		json["weights_offset"] = so.size();
		so.save(layer.weights(), sizeof(T) * layer.inputs() * layer.neurons());
		json["bias_offset"] = so.size();
		so.save(layer.bias(), sizeof(U) * layer.neurons());
		return json;
	}
}

namespace ag
{
	namespace nnue
	{
		void load_layer(NnueLayer<int8_t, int16_t> &layer, const Json &json, const SerializedObject &so)
		{
			load_impl(layer, json, so);
		}
		void load_layer(NnueLayer<int16_t, int32_t> &layer, const Json &json, const SerializedObject &so)
		{
			load_impl(layer, json, so);
		}
		void load_layer(NnueLayer<float, float> &layer, const Json &json, const SerializedObject &so)
		{
			load_impl(layer, json, so);
		}

		Json save_layer(const NnueLayer<int8_t, int16_t> &layer, SerializedObject &so)
		{
			return save_impl(layer, so);
		}
		Json save_layer(const NnueLayer<int16_t, int32_t> &layer, SerializedObject &so)
		{
			return save_impl(layer, so);
		}
		Json save_layer(const NnueLayer<float, float> &layer, SerializedObject &so)
		{
			return save_impl(layer, so);
		}

	} /* namespace nnue */
} /* namespace ag */

