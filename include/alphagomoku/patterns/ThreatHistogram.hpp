/*
 * ThreatHistogram.hpp
 *
 *  Created on: Oct 6, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_THREATHISTOGRAM_HPP_
#define ALPHAGOMOKU_PATTERNS_THREATHISTOGRAM_HPP_

#include <alphagomoku/patterns/ThreatTable.hpp>
#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <algorithm>
#include <iostream>
#include <x86intrin.h>

namespace ag
{
	class ThreatHistogram
	{
//			std::array<LocationList, 10> threats;
			LocationList threats[10];
		public:
			ThreatHistogram()
			{
				for (int i = 1; i < 10; i++) // do not reserve for 'NONE' threat
					threats[i].reserve(128);
			}
			const LocationList& get(ThreatType threat) const noexcept
			{
				assert(static_cast<int>(threat) < 10);
				return threats[static_cast<size_t>(threat)];
			}
			void remove(ThreatType threat, Location location) noexcept
			{
				assert(static_cast<int>(threat) < 10);
				if (threat != ThreatType::NONE)
				{
					LocationList &list = threats[static_cast<size_t>(threat)];
					const uint16_t *ptr = reinterpret_cast<uint16_t*>(list.data());
					const uint16_t value = location.toShort();
					const int length = list.size();
#ifdef __AVX2__
					const __m256i val = _mm256_set1_epi16(value);
					for (int i = 0; i < length; i += 32, ptr += 32)
					{
						const __m256i tmp0 = _mm256_load_si256((const __m256i*) (ptr + 0));
						const __m256i tmp1 = _mm256_load_si256((const __m256i*) (ptr + 16));
						const __m256i cmp0 = _mm256_cmpeq_epi16(tmp0, val);
						const __m256i cmp1 = _mm256_cmpeq_epi16(tmp1, val);
						const __m256i tmp = _mm256_permute4x64_epi64(_mm256_packs_epi16(cmp0, cmp1), 0b11'01'10'00);
						const int mask = _mm256_movemask_epi8(tmp);
						if (mask != 0)
						{
							const int idx = i + _bit_scan_forward(mask);
							if (idx < length)
							{
								list[idx] = list[length - 1]; // swap found element with the last in the list
								list.pop_back();
							}
							return;
						}
					}
#elif defined(__SSE2__)
					const __m128i val = _mm_set1_epi16(value);
					for (int i = 0; i < length; i += 16, ptr += 16)
					{
						const __m128i tmp0 = _mm_load_si128((const __m128i*) (ptr + 0));
						const __m128i tmp1 = _mm_load_si128((const __m128i*) (ptr + 8));
						const __m128i cmp0 = _mm_cmpeq_epi16(tmp0, val);
						const __m128i cmp1 = _mm_cmpeq_epi16(tmp1, val);
						const __m128i tmp = _mm_packs_epi16(cmp0, cmp1);
						const int mask = _mm_movemask_epi8(tmp);
						if (mask != 0)
						{
							const int idx = i + _bit_scan_forward(mask);
							if (idx < length)
							{
								list[idx] = list[length - 1]; // swap found element with the last in the list
								list.pop_back();
							}
							return;
						}
					}
#else
					for (int i = 0; i < length; i++)
						if (ptr[i] == value)
						{
							list[idx] = list[length - 1]; // swap found element with the last in the list
							list.pop_back();
							return;
						}
#endif
				}
			}
			void add(ThreatType threat, Location location) noexcept
			{
				assert(static_cast<int>(threat) < 10);
				if (threat != ThreatType::NONE)
				{
					LocationList &list = threats[static_cast<size_t>(threat)];
					if (list.size() == list.capacity())
						list.reserve(2 * list.capacity()); // ensuring that the size will remain divisible by 32
					list.push_back(location);
				}
			}
			void clear() noexcept
			{
				for (int i = 0; i < 10; i++)
					threats[i].clear();
			}
			bool hasAnyFour() const noexcept
			{
				return get(ThreatType::HALF_OPEN_4).size() > 0 or get(ThreatType::FORK_4x3).size() > 0 or get(ThreatType::FORK_4x4).size() > 0
						or get(ThreatType::OPEN_4).size() > 0;
			}
			bool canMakeAnyThreat() const noexcept
			{
				for (int i = 2; i <= 8; i++)
					if (threats[i].size() > 0)
						return true;
				return false;
			}
			void print() const
			{
				for (int i = 1; i < 10; i++)
					for (size_t j = 0; j < threats[i].size(); j++)
					{
						std::cout << threats[i][j].toString() << " : " << threats[i][j].text() << ((threats[i][j].row < 10) ? " " : "") << " : "
								<< toString(static_cast<ThreatType>(i)) << '\n';
					}
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_THREATHISTOGRAM_HPP_ */
