/*
 * PatternTable.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_PATTERNTABLE_HPP_
#define ALPHAGOMOKU_SOLVER_PATTERNTABLE_HPP_

#include <alphagomoku/rules/game_rules.hpp>

#include <array>
#include <map>
#include <cassert>
#include <iostream>

namespace ag::experimental
{
	enum class PatternType : int8_t
	{
		NONE,
		HALF_OPEN_3, // can become half open four in one move
		OPEN_3, // can become open four in one move
		HALF_OPEN_4,
		OPEN_4,
		DOUBLE_4, // inline 4x4 fork
		FIVE, // actually, it is a winning pattern so for some rules it means 'five but not overline'
		OVERLINE
	};
	std::string toString(PatternType pt);

	inline uint32_t invertColor(uint32_t pattern) noexcept
	{
		return ((pattern >> 1) & 0x55555555) | ((pattern & 0x55555555) << 1);
	}
	inline uint32_t reverse_bits(uint32_t x) noexcept
	{
		x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
		x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
		x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
		x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
		x = (x >> 16) | (x << 16);
		return x;
	}
	inline uint16_t reverse_bits(uint16_t x) noexcept
	{
		x = ((x >> 1) & 0x5555) | ((x & 0x5555) << 1);
		x = ((x >> 2) & 0x3333) | ((x & 0x3333) << 2);
		x = ((x >> 4) & 0x0F0F) | ((x & 0x0F0F) << 4);
		x = (x >> 8) | (x << 8);
		return x;
	}

	struct PatternTypeGroup
	{
			std::array<PatternType, 4> for_cross;
			std::array<PatternType, 4> for_circle;
	};

	template<typename T>
	struct BitMask
	{
		private:
			T m_data = 0;
		public:
			BitMask() noexcept = default;
			BitMask(T data) noexcept :
					m_data(data)
			{
			}
			BitMask(const std::string &str)
			{
				for (size_t i = 0; i < str.size(); i++)
				{
					assert(str.at(i) == '1' || str.at(i) == '0');
					set(i, str.at(i) == '1');
				}
			}
			void set(uint32_t index, bool b) noexcept
			{
				assert(index < 8 * sizeof(T));
				m_data &= (~(static_cast<T>(1) << index));
				m_data |= (static_cast<T>(b) << index);
			}
			bool get(uint32_t index) const noexcept
			{
				assert(index < 8 * sizeof(T));
				return static_cast<bool>((m_data >> index) & static_cast<T>(1));
			}
			T raw() const noexcept
			{
				return m_data;
			}
			void flip(int length) noexcept
			{
				m_data = reverse_bits(m_data) >> (8 * sizeof(T) - length);
			}
			friend bool operator==(const BitMask<T> &lhs, const BitMask<T> &rhs) noexcept
			{
				return lhs.raw() == rhs.raw();
			}
			friend BitMask<T>& operator&=(BitMask<T> &lhs, const BitMask<T> &rhs) noexcept
			{
				lhs.m_data &= rhs.m_data;
				return lhs;
			}
			BitMask<T> operator<<(int shift) const noexcept
			{
				return BitMask<T>(m_data << shift);
			}
			BitMask<T> operator>>(int shift) const noexcept
			{
				return BitMask<T>(m_data >> shift);
			}
	};

	struct PatternEncoding
	{
		private:
			uint32_t m_data = 0u;
		public:
			PatternEncoding() noexcept = default;
			PatternEncoding(PatternType cross, PatternType circle) noexcept :
					m_data(static_cast<uint32_t>(cross) | (static_cast<uint32_t>(circle) << 3))
			{
			}
			PatternType forCross() const noexcept
			{
				return static_cast<PatternType>(m_data & 7);
			}
			PatternType forCircle() const noexcept
			{
				return static_cast<PatternType>((m_data >> 3) & 7);
			}
			bool mustBeUpdated(int index) const noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6;
				return static_cast<bool>((m_data >> index) & 1);
			}
			void setUpdateMask(int index, bool b) noexcept
			{
				assert(index >= 0 && index < 11);
				index += 6;
				m_data &= (~(1 << index));
				m_data |= (static_cast<uint32_t>(b) << index);
			}
			uint32_t getDefensiveMoves(Sign sign) const noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				return (sign == Sign::CROSS) ? ((m_data >> 17) & 127) : ((m_data >> 24) & 127);
			}
			void setDefensiveMoves(Sign sign, uint32_t maskIndex) noexcept
			{
				assert(sign == Sign::CROSS || sign == Sign::CIRCLE);
				const uint32_t shift = (sign == Sign::CROSS) ? 17 : 24;
				m_data &= (~(127u << shift));
				m_data |= (maskIndex << shift);
			}
	}
	;

	class PatternTable
	{
		private:
			std::vector<PatternEncoding> patterns;

			std::map<uint32_t, BitMask<uint16_t>> double_four_defensive_moves;
			std::map<uint32_t, BitMask<uint16_t>> open_four_defensive_moves;
			std::map<uint32_t, BitMask<uint16_t>> half_open_four_defensive_moves;
			std::vector<BitMask<uint16_t>> defensive_move_mask;
			GameRules game_rules;
		public:
			PatternTable(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			PatternEncoding getPatternData(uint32_t pattern) const noexcept
			{
				assert(pattern < patterns.size());
				return patterns[pattern];
			}
			BitMask<uint16_t> getDefensiveMoves(PatternEncoding enc, Sign sign) const noexcept
			{
				return defensive_move_mask[enc.getDefensiveMoves(sign)];
			}
			static const PatternTable& get(GameRules rules);
		private:
			void init_features();
			void init_update_mask();
			void init_defensive_moves();
			size_t add_defensive_move_mask(BitMask<uint16_t> mask);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_PATTERNTABLE_HPP_ */
