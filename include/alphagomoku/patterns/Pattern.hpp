/*
 * Pattern.hpp
 *
 *  Created on: Jul 17, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_PATTERN_HPP_
#define ALPHAGOMOKU_PATTERNS_PATTERN_HPP_

#include <alphagomoku/game/Move.hpp>

#include <cassert>
#include <iostream>

namespace ag
{
	template<typename T>
	inline T power(T base, T exponent) noexcept
	{
		assert(exponent >= 0);
		T result = 1;
		for (T i = 0; i < exponent; i++)
			result *= base;
		return result;
	}

	class Pattern
	{
		private:
			uint32_t m_feature = 0;
			uint32_t m_length = 0;
		public:
			static constexpr int length = 11;

			Pattern() = default;
			Pattern(int len) :
					m_feature(0),
					m_length(len)
			{
			}
			Pattern(int len, uint32_t feature) :
					Pattern(len)
			{
				m_feature = feature;
			}
			Pattern(const std::string &str) :
					Pattern(str.size())
			{
				for (uint32_t i = 0; i < m_length; i++)
					set(i, signFromText(str[i]));
			}
			bool isValid() const noexcept
			{
				if (getCenter() != Sign::NONE)
					return false;
				for (uint32_t i = 0; i < m_length / 2; i++)
					if (get(i) != Sign::ILLEGAL and get(i + 1) == Sign::ILLEGAL)
						return false;
				for (uint32_t i = 1 + m_length / 2; i < m_length; i++)
					if (get(i - 1) == Sign::ILLEGAL and get(i) != Sign::ILLEGAL)
						return false;
				return true;
			}
			void invert() noexcept
			{
				for (uint32_t i = 0; i < m_length; i++)
					set(i, invertSign(get(i)));
			}
			void flip() noexcept
			{
				m_feature = ((m_feature >> 2) & 0x33333333) | ((m_feature & 0x33333333) << 2);
				m_feature = ((m_feature >> 4) & 0x0F0F0F0F) | ((m_feature & 0x0F0F0F0F) << 4);
				m_feature = ((m_feature >> 8) & 0x00FF00FF) | ((m_feature & 0x00FF00FF) << 8);
				m_feature = (m_feature >> 16) | (m_feature << 16);
				m_feature >>= (32u - 2 * m_length);
			}
			Sign get(uint32_t index) const noexcept
			{
				assert(index < m_length);
				return static_cast<Sign>((m_feature >> (2 * index)) & 3);
			}
			void set(uint32_t index, Sign sign) noexcept
			{
				assert(index < m_length);
				m_feature &= (~(3 << (2 * index)));
				m_feature |= (static_cast<uint32_t>(sign) << (2 * index));
			}
			Sign getCenter() const noexcept
			{
				return get(m_length / 2);
			}
			void setCenter(Sign sign) noexcept
			{
				set(m_length / 2, sign);
			}
			void decode(uint32_t feature) noexcept
			{
				m_feature = feature;
			}
			uint32_t encode() const noexcept
			{
				return m_feature;
			}
			std::string toString() const
			{
				std::string result;
				for (size_t i = 0; i < m_length; i++)
					result += text(get(i));
				return result;
			}
			size_t size() const noexcept
			{
				return m_length;
			}
			void shiftLeft(size_t n) noexcept
			{
				m_feature >>= (2 * n); // this is intentionally inverted, as the features are read from left to right
			}
			void shiftRight(size_t n) noexcept
			{
				const uint32_t mask = (1 << (2 * m_length)) - 1;
				m_feature = (m_feature << (2 * n)) & mask; // this is intentionally inverted, as the features are read from left to right
			}
			void mergeWith(const Pattern &other) noexcept
			{
				assert(this->size() == other.size());
				m_feature |= other.m_feature;
			}
			Pattern subPattern(int start, int length) const noexcept
			{
				const uint32_t shift = 2u * start;
				const uint32_t mask = (1u << (2u * length)) - 1u;
				return Pattern(length, (m_feature >> shift) & mask);
			}
			friend bool operator==(const Pattern &lhs, const Pattern &rhs)
			{
				return lhs.m_length == rhs.m_length and lhs.m_feature == rhs.m_feature;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_PATTERN_HPP_ */
