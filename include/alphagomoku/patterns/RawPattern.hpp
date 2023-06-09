/*
 * RawPattern.hpp
 *
 *  Created on: Nov 9, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_RAWPATTERN_HPP_
#define ALPHAGOMOKU_PATTERNS_RAWPATTERN_HPP_

#include <cinttypes>

namespace ag
{
	struct ReducedPattern
	{
		private:
			uint32_t data = 0;
		public:
			static constexpr int length = 9;
			ReducedPattern() noexcept = default;
			explicit ReducedPattern(uint32_t x) noexcept :
					data(x)
			{
			}
			operator uint32_t() const noexcept
			{
				return data;
			}
	};
	struct NormalPattern
	{
		private:
			uint32_t data = 0;
		public:
			static constexpr int length = 11;
			NormalPattern() noexcept = default;
			explicit NormalPattern(uint32_t x) noexcept :
					data(x)
			{
			}
			operator uint32_t() const noexcept
			{
				return data;
			}
	};
	struct ExtendedPattern
	{
		private:
			uint32_t data = 0;
		public:
			static constexpr int length = 13;
			ExtendedPattern() noexcept = default;
			explicit ExtendedPattern(uint32_t x) noexcept :
					data(x)
			{
			}
			operator uint32_t() const noexcept
			{
				return data;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_RAWPATTERN_HPP_ */
