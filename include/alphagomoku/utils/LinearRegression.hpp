/*
 * LinearRegression.hpp
 *
 *  Created on: 23 mar 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_LINEARREGRESSION_HPP_
#define ALPHAGOMOKU_UTILS_LINEARREGRESSION_HPP_

#include <vector>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace ag
{
	/**
	 * Following https://science.widener.edu/svb/stats/regress.html
	 */
	template<typename T, typename U>
	class LinearRegression
	{
		private:
			double a = 0.0, b = 0.0, N = 0.0;
			double avg_x = 0.0, avg_y = 0.0;
			double Sxy = 0.0, Sxx = 0.0, Syy = 0.0, Sr = 0.0;
		public:
			LinearRegression(const std::vector<std::pair<T, U>> &data) :
					N(data.size())
			{
				if (data.size() < 2)
					throw std::invalid_argument("Linear regression requires at least 2 points of data.");

				for (auto iter = data.begin(); iter < data.end(); iter++)
				{
					avg_x += iter->first;
					avg_y += iter->second;
				}
				avg_x /= N;
				avg_y /= N;
				for (auto iter = data.begin(); iter < data.end(); iter++)
				{
					Sxy += (iter->first - avg_x) * (iter->second - avg_y);
					Sxx += (iter->first - avg_x) * (iter->first - avg_x);
					Syy += (iter->second - avg_y) * (iter->second - avg_y);
				}
				a = Sxy / Sxx;
				b = avg_y - a * avg_x;
				Sr = std::sqrt((Syy - a * a * Sxx) / (N - 2));
			}
			std::pair<double, double> predict(T x) const noexcept
			{
				assert(N >= 2);
				const double y = a * x + b;
				if (N == 2)
					return std::pair<double, double>( { y, 0.0 });
				else
				{
					double err_y = Sr * std::sqrt(1.0 + 1.0 / N + (x - avg_x) * (x - avg_x) / Sxx);
					return std::pair<double, double>( { y, err_y });
				}
			}
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_LINEARREGRESSION_HPP_ */
