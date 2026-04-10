/*
 * MovingAverage.hpp
 *
 *  Created on: Mar 27, 2026
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_MOVINGAVERAGE_HPP_
#define ALPHAGOMOKU_UTILS_MOVINGAVERAGE_HPP_

#include <cmath>

namespace ag
{

	template<typename T>
	class MovingAverage
	{
			T m_mean { };
			T m_count { };
			T m_alpha { };
		public:
			MovingAverage() noexcept = default;
			MovingAverage(T alpha) noexcept :
					m_alpha(alpha)
			{
			}
			MovingAverage(T mean, T count, T alpha) noexcept :
					m_mean(mean),
					m_count(count),
					m_alpha(alpha)
			{
			}
			void add(T value) noexcept
			{
				m_count++;
				m_mean += (value - m_mean) * std::max(m_alpha, 1 / m_count);
			}
			T mean() const noexcept
			{
				return m_mean;
			}
			T count() const noexcept
			{
				return m_count;
			}
			T alpha() const noexcept
			{
				return m_alpha;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_MOVINGAVERAGE_HPP_ */
