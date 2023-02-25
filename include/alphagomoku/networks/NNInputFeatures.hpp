/*
 * NNInputFeatures.hpp
 *
 *  Created on: Jan 19, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_NNINPUTFEATURES_HPP_
#define ALPHAGOMOKU_SELFPLAY_NNINPUTFEATURES_HPP_

#include <alphagomoku/utils/matrix.hpp>

#include <cinttypes>

namespace ag
{
	class PatternCalculator;
}

namespace ag
{

	class NNInputFeatures: public matrix<uint32_t>
	{
		public:
			NNInputFeatures() noexcept = default;
			NNInputFeatures(int rows, int cols);
			void encode(PatternCalculator &calc);
			void augment(int mode) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_NNINPUTFEATURES_HPP_ */
