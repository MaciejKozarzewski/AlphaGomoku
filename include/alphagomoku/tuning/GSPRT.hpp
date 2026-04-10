/*
 * GSPRT.hpp
 *
 *  Created on: Mar 30, 2026
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TUNING_GSPRT_HPP_
#define ALPHAGOMOKU_TUNING_GSPRT_HPP_

#include <array>
#include <vector>

namespace ag
{
	class TwoMatch;
}

namespace ag
{

	class GSPRT
	{
			double elo0 = 0;
			double elo1 = 5;

			double LA, LB;
			double LLR = 0.0, min_LLR = 0.0, max_LLR = 0.0;
			double sq0 = 0.0, sq1 = 0.0;
			double o0 = 0.0, o1 = 0.0;
			int status = -1;

			std::array<double, 5> results;
		public:
			GSPRT(double elo0, double elo1, double alpha = 0.05, double beta = 0.05);
			void add_result(int result);

			double get_LLR() const noexcept
			{
				return LLR;
			}
			int get_status() const noexcept
			{
				return status;
			}
	};

	std::vector<int> convert_match_results(const std::vector<TwoMatch> &buffer);

} /* namespace ag */

#endif /* ALPHAGOMOKU_TUNING_GSPRT_HPP_ */
