/*
 * def_ops.hpp
 *
 *  Created on: Oct 21, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_NNUE_OPS_DEF_OPS_HPP_
#define ALPHAGOMOKU_NETWORKS_NNUE_OPS_DEF_OPS_HPP_

#include <alphagomoku/networks/nnue_ops/layers.hpp>
#include <vector>

namespace ag
{
	namespace nnue
	{
		/*
		 * \brief Computes values in the input features accumulator.
		 */
		void def_refresh_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, Accumulator<int16_t> &accumulator,
				const std::vector<int> &active) noexcept;

		/*
		 * \brief Updates the input feature accumulator.
		 */
		void def_update_accumulator(const NnueLayer<int8_t, int16_t> &layer_0, const Accumulator<int16_t> &oldAccumulator,
				Accumulator<int16_t> &newAccumulator, const std::vector<int> &removed, const std::vector<int> &added) noexcept;

		/*
		 * \brief Runs forward pass of the accumulator activation, middle and output layers.
		 */
		float def_forward(const Accumulator<int16_t> &accumulator, const NnueLayer<int16_t, int32_t> &layer_1,
				const std::vector<NnueLayer<float, float>> &fp32_layers) noexcept;

	} /* namespace nnue */
} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NNUE_OPS_DEF_OPS_HPP_ */
