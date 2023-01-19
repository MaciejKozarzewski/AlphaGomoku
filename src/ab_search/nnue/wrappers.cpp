/*
 * wrappers.cpp
 *
 *  Created on: Oct 23, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/nnue/wrappers.hpp>

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

namespace ag
{
	namespace nnue
	{
		QuantizedLayer::QuantizedLayer(int inputs, int neurons) :
				data((sizeof(int32_t) + sizeof(float) + sizeof(int8_t) * inputs) * neurons),
				weights(data.data(), inputs, neurons),
				bias(reinterpret_cast<int32_t*>(data.data() + weights.sizeInBytes()), neurons),
				scale(reinterpret_cast<float*>(data.data() + weights.sizeInBytes() + bias.sizeInBytes()), neurons)
		{
		}
		QuantizedLayer::QuantizedLayer(const Json &json, const SerializedObject &so) :
				QuantizedLayer(json["inputs"].getInt(), json["neurons"].getInt())
		{
			const size_t offset = json["binary_offset"].getInt();
			so.load(data.data(), offset, data.sizeInBytes());
		}
		Json QuantizedLayer::save(SerializedObject &so) const
		{
			Json result;
			result["inputs"] = weights.rows();
			result["neurons"] = weights.cols();
			result["binary_offset"] = so.size();
			so.save(data.data(), data.sizeInBytes());
			return result;
		}

		RealSpaceLayer::RealSpaceLayer(int inputs, int neurons) :
				data(sizeof(float) * (1 + inputs) * neurons),
				weights(data.data(), inputs, neurons),
				bias(data.data() + weights.size(), neurons)
		{
		}
		RealSpaceLayer::RealSpaceLayer(const Json &json, const SerializedObject &so) :
				RealSpaceLayer(json["inputs"].getInt(), json["neurons"].getInt())
		{
			const size_t offset = json["binary_offset"].getInt();
			so.load(data.data(), offset, data.sizeInBytes());
		}
		Json RealSpaceLayer::save(SerializedObject &so) const
		{
			Json result;
			result["inputs"] = weights.rows();
			result["neurons"] = weights.cols();
			result["binary_offset"] = so.size();
			so.save(data.data(), data.sizeInBytes());
			return result;
		}
	}
}

