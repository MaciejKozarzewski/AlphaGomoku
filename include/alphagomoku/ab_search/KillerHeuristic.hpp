/*
 * KillerHeuristic.hpp
 *
 *  Created on: Oct 31, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_KILLERHEURISTIC_HPP_
#define ALPHAGOMOKU_AB_SEARCH_KILLERHEURISTIC_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <vector>

namespace ag
{
	namespace obsolete
	{
		template<int N>
		class KillerHeuristics
		{
				std::vector<ShortVector<Move, N>> m_data;
			public:
				KillerHeuristics(GameConfig gameConfig) :
						m_data(gameConfig.rows * gameConfig.cols)
				{
				}
				const ShortVector<Move, N>& get(int depth) const noexcept
				{
					return m_data[depth];
				}
				void insert(Move m, int depth) noexcept
				{
					ShortVector<Move, N> &list = m_data[depth];
					if (list.size() > 0 and m == list[0])
						return; // don't do anything if this move is already at the front
					for (int i = 1; i < list.size(); i++)
						if (list[i] == m)
						{ // the move is already in the list
							std::swap(list[i - 1], list[i]); // move closer to the front
							return;
						}
					if (list.size() < N)
						list.add(m); // if there is some space left, add move to the list
					else
						list[list.size() - 1] = m; // if the list is full, replace the last entry
				}
				int entrySize() const noexcept
				{
					return N;
				}
				void clear() noexcept
				{
					for (size_t i = 0; i < m_data.size(); i++)
						m_data[i].clear();
				}
		};
	} /* namespace obsolete */
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_KILLERHEURISTIC_HPP_ */
