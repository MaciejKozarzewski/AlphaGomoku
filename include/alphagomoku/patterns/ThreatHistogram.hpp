/*
 * ThreatHistogram.hpp
 *
 *  Created on: Oct 6, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_THREATHISTOGRAM_HPP_
#define ALPHAGOMOKU_PATTERNS_THREATHISTOGRAM_HPP_

#include <alphagomoku/patterns/ThreatTable.hpp>

#include <algorithm>

namespace ag
{
	class ThreatHistogram
	{
			std::array<std::vector<Move>, 10> threats;
		public:
			ThreatHistogram()
			{
				for (size_t i = 1; i < threats.size(); i++) // do not reserve for 'NONE' threat
					threats[i].reserve(64);
			}
			const std::vector<Move>& get(ThreatType threat) const noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				return threats[static_cast<size_t>(threat)];
			}
			void remove(ThreatType threat, Move move) noexcept
			{
				assert(static_cast<size_t>(threat) < threats.size());
				if (threat != ThreatType::NONE)
				{
					std::vector<Move> &list = threats[static_cast<size_t>(threat)];
					const auto index = std::find(list.begin(), list.end(), move);
					assert(index != list.end()); // the threat must exist in the list
					list.erase(index);
				}
			}
			void add(ThreatType threat, Move move)
			{
				assert(static_cast<size_t>(threat) < threats.size());
				if (threat != ThreatType::NONE)
					threats[static_cast<size_t>(threat)].push_back(move);
			}
			void clear() noexcept
			{
				for (size_t i = 0; i < threats.size(); i++)
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
				for (size_t i = 1; i < threats.size(); i++)
					for (size_t j = 0; j < threats[i].size(); j++)
						std::cout << threats[i][j].toString() << " : " << threats[i][j].text() << ((threats[i][j].row < 10) ? " " : "") << " : "
								<< toString(static_cast<ThreatType>(i)) << '\n';
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_THREATHISTOGRAM_HPP_ */
