/*
 * avx2_ops.hpp
 *
 *  Created on: Oct 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_NNUE_AVX2_OPS_HPP_
#define ALPHAGOMOKU_AB_SEARCH_NNUE_AVX2_OPS_HPP_

#include <alphagomoku/ab_search/nnue/wrappers.hpp>

#include <vector>

namespace ag
{
	namespace nnue
	{
		/*
		 * \brief Computes values in the input features accumulator.
		 */
		void avx2_refresh_accumulator(const QuantizedLayer &layer_1, Wrapper1D<int32_t> &accumulator, const std::vector<int> &active) noexcept;

		/*
		 * \brief Updates the input feature accumulator.
		 */
		void avx2_update_accumulator(const QuantizedLayer &layer_1, const Wrapper1D<int32_t> &oldAccumulator, Wrapper1D<int32_t> &newAccumulator,
				const std::vector<int> &removed, const std::vector<int> &added) noexcept;

		/*
		 * \brief Runs forward pass of the accumulator activation, middle and output layers.
		 */
		float avx2_forward(const Wrapper1D<int32_t> &accumulator, const QuantizedLayer &layer_1, const RealSpaceLayer &layer_2,
				const RealSpaceLayer &layer_3) noexcept;

	} /* namespace nnue */
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_NNUE_AVX2_OPS_HPP_ */
