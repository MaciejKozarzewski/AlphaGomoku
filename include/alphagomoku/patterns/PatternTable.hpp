/*
 * PatternTable.hpp
 *
 *  Created on: Apr 20, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_PATTERNTABLE_HPP_
#define ALPHAGOMOKU_PATTERNS_PATTERNTABLE_HPP_

#include <alphagomoku/patterns/RawPattern.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/utils/BitMask.hpp>

#include <vector>
#include <string>
#include <cassert>

namespace ag
{
	enum class PatternType : int8_t
	{
		NONE,
		HALF_OPEN_3, // can become half open four in one move
		OPEN_3, // can become open four in one move
		HALF_OPEN_4, // can become a five in exactly one way
		OPEN_4, // can become a five in more than one way
		DOUBLE_4, // inline 4x4 fork
		FIVE, // actually, it is a winning pattern so for some rules it means 'five but not overline'
		OVERLINE
	};
	std::string toString(PatternType pt);

	class PatternEncoding
	{
		private:
			uint8_t data = 0u;
		public:
			PatternEncoding() noexcept = default;
			PatternEncoding(PatternEncoding cross, PatternEncoding circle) noexcept :
					data((cross.data & 0x0F) | (circle.data & 0xF0))
			{
			}
			PatternEncoding(PatternType cross, PatternType circle) noexcept :
					data(static_cast<uint8_t>(cross) | (static_cast<uint8_t>(circle) << 4))
			{
			}
			explicit PatternEncoding(PatternType tt) noexcept :
					PatternEncoding(tt, tt)
			{
			}
			PatternType forCross() const noexcept
			{
				return static_cast<PatternType>(data & 0x0F);
			}
			PatternType forCircle() const noexcept
			{
				return static_cast<PatternType>(data >> 4);
			}
			void setForCross(PatternType pt) noexcept
			{
				data = (data & 0xF0) | static_cast<uint8_t>(pt);
			}
			void setForCircle(PatternType pt) noexcept
			{
				data = (data & 0x0F) | (static_cast<uint8_t>(pt) << 4);
			}
	};

	class UpdateMask
	{
			uint32_t data = 0;
		public:
			UpdateMask() noexcept = default;
			int get(int index) const noexcept
			{
				return (data >> (2 * index)) & 3;
			}
			void set(int index, bool cross, bool circle) noexcept
			{
				const uint32_t tmp = static_cast<uint32_t>(cross) | (static_cast<uint32_t>(circle) << 1);
				data = (data & (~(3 << (2 * index)))) | (tmp << (2 * index));
			}
			void flip(int length) noexcept
			{
				uint32_t tmp = 0;
				for (int i = 0; i < length; i++)
					tmp |= (get(i) << (2 * (length - 1 - i)));
				data = tmp;
			}
	};

	class PatternTable
	{
		private:
			std::vector<PatternEncoding> pattern_types;
			std::vector<UpdateMask[2]> update_mask;
			std::vector<bool> half_open_3_cross;
			std::vector<bool> half_open_3_circle;
			GameRules game_rules;
		public:
			PatternTable(GameRules rules);
			GameRules getRules() const noexcept
			{
				return game_rules;
			}
			UpdateMask getUpdateMask(NormalPattern pattern, Sign newStoneColor) const noexcept
			{
				assert(narrow_down(pattern) < update_mask.size());
				assert(newStoneColor == Sign::CROSS || newStoneColor == Sign::CIRCLE);
				return update_mask[narrow_down(pattern)][newStoneColor == Sign::CIRCLE];
			}
			PatternEncoding getPatternType(NormalPattern pattern) const noexcept
			{
				assert(narrow_down(pattern) < pattern_types.size());
				assert((pattern & 3072u) == 0); // central spot must be empty
				return pattern_types[narrow_down(pattern)];
			}
			bool isHalfOpenThree(NormalPattern pattern, Sign s) const noexcept
			{
				assert(s == Sign::CROSS || s == Sign::CIRCLE);
				assert(narrow_down(pattern) < half_open_3_cross.size());
				assert((pattern & 3072u) == 0); // central spot must be empty
				const uint32_t idx = narrow_down(pattern);
				return (s == Sign::CROSS) ? half_open_3_cross[idx] : half_open_3_circle[idx];
			}
			static const PatternTable& get(GameRules rules);
		private:
			void init_features();
			void init_update_mask();
			/*
			 * \brief Removes 2 central bits from the 22-bit number producing 20-bit number.
			 */
			uint32_t narrow_down(uint32_t x) const noexcept
			{
				return (x & 1023u) | ((x & 4190208u) >> 2);
			}
			/*
			 * \brief Inserts 2 zeros in the central position, producing 22-bit number out of 20-bit one.
			 */
			uint32_t expand(uint32_t x) const noexcept
			{
				return (x & 1023u) | ((x & 1047552u) << 2u);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_PATTERNTABLE_HPP_ */
