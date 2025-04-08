/*
 * Parameter.hpp
 *
 *  Created on: Apr 3, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_PARAMETER_HPP_
#define ALPHAGOMOKU_UTILS_PARAMETER_HPP_

#include <minml/core/Device.hpp>
#include <minml/utils/json.hpp>

#include <vector>
#include <limits>
#include <cmath>

namespace ag
{
	template<typename T>
	class Parameter
	{
			T m_value = { };
			std::vector<std::pair<int, T>> m_schedule;
			std::string m_interpolate = "none";
		public:
			Parameter() = default;
			Parameter(T value) noexcept :
					m_value(value)
			{
			}
			Parameter(const std::vector<std::pair<int, T>> &schedule, std::string interpolate = "none") :
					m_schedule(schedule),
					m_interpolate(interpolate)
			{
			}
			Parameter(const Json &cfg)
			{
				if (cfg.isObject())
				{
					m_interpolate = cfg["interpolate"].getString();
					if (cfg["epoch"].size() != cfg["value"].size())
						throw std::invalid_argument("'epoch' and 'value' list must have the same number of elements");
					for (int i = 0; i < cfg["epoch"].size(); i++)
					{
						int epoch = cfg["epoch"][i].getInt();
						T value = cfg["value"][i].getDouble();
						m_schedule.push_back(std::pair<int, T>( { epoch, value }));
					}
				}
				else
					m_value = cfg.getDouble();
			}
			Json toJson() const
			{
				if (m_schedule.empty())
					return Json(m_value);
				else
				{
					Json result( { { "interpolate", m_interpolate }, { "epoch", Json(JsonType::Array) }, { "value", Json(JsonType::Array) } });
					for (size_t i = 0; i < m_schedule.size(); i++)
					{
						result["epoch"][i] = m_schedule.at(i).first;
						result["value"][i] = m_schedule.at(i).second;
					}
					return result;
				}
			}
			operator T() const
			{
				if (not m_schedule.empty())
					throw std::logic_error("The parameter does not have single value");
				return m_value;
			}
			T getValue() const
			{
				if (not m_schedule.empty())
					throw std::logic_error("The parameter does not have single value");
				return m_value;
			}
			T getValue(int epoch) const
			{
				if (m_schedule.empty())
					return m_value;
				if (m_interpolate == "none")
				{
					T result = m_schedule.front().second;
					for (size_t i = 0; i < m_schedule.size(); i++)
						if (epoch >= m_schedule.at(i).first)
							result = m_schedule.at(i).second;
					return result;
				}
				else
				{
					if (epoch <= m_schedule.front().first)
						return m_schedule.front().second;
					if (epoch >= m_schedule.back().first)
						return m_schedule.back().second;

					std::pair<int, T> prev, next;
					for (size_t i = 0; i < m_schedule.size(); i++)
						if (epoch > m_schedule.at(i).first)
						{
							prev = m_schedule.at(i);
							next = m_schedule.at(i + 1);
						}

					double result = T { };
					if (m_interpolate == "linear")
					{
						const double tmp1 = epoch - prev.first;
						const double tmp2 = next.first - prev.first;
						const double tmp3 = next.second - prev.second;
						result = prev.second + tmp1 / tmp2 * tmp3;
					}
					if (m_interpolate == "exp")
					{
						const double tmp1 = epoch - prev.first;
						const double tmp2 = next.first - prev.first;
						const double a = std::log(next.second / prev.second) / tmp2;
						result = prev.second * std::exp(a * tmp1);
					}
					if (m_interpolate == "cosine")
					{
						const double tmp1 = epoch - prev.first;
						const double tmp2 = next.first - prev.first;
						const double tmp3 = next.second - prev.second;
						result = prev.second + 0.5 * tmp3 * (1.0 - std::cos(tmp1 / tmp2 * M_PI));
					}
					if (std::is_integral<T>::value)
						return static_cast<T>(result + 0.5); // round to nearest
					else
						return result;
				}
			}
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_PARAMETER_HPP_ */
