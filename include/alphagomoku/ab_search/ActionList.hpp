/*
 * ActionList.hpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_ACTION_LIST_HPP_
#define ALPHAGOMOKU_AB_SEARCH_ACTION_LIST_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/ab_search/Score.hpp>

#include <algorithm>
#include <iostream>

namespace ag
{
	class ActionStack;

	struct Action
	{
			Move move;
			Score score;
			Action() noexcept = default;
			void init(Move m, Score s) noexcept
			{
				move = m;
				score = s;
			}
			friend bool operator<(const Action &lhs, const Action &rhs) noexcept
			{
				return lhs.score < rhs.score;
			}
	};

	class ActionList
	{
			friend class ActionStack;
			Action *m_ptr = nullptr;
			size_t m_size = 0;
			ActionList(Action *ptr) noexcept :
					m_ptr(ptr)
			{
			}
		public:
			bool is_in_danger = false;
			bool is_threatening = false;
			ActionList(const ActionList &prev) noexcept :
					m_ptr(prev.end())
			{
			}
			void clear() noexcept
			{
				m_size = 0;
			}
			Action* begin() noexcept
			{
				return m_ptr;
			}
			Action* begin() const noexcept
			{
				return m_ptr;
			}
			Action* end() noexcept
			{
				return m_ptr + m_size;
			}
			Action* end() const noexcept
			{
				return m_ptr + m_size;
			}
			Action& operator[](size_t index) noexcept
			{
				assert(index < m_size);
				return m_ptr[index];
			}
			const Action& operator[](size_t index) const noexcept
			{
				assert(index < m_size);
				return m_ptr[index];
			}
			int size() const noexcept
			{
				return m_size;
			}
			bool containsAction(Move m) const noexcept
			{
				return std::any_of(begin(), end(), [m](const Action &action)
				{	return action.move == m;});
			}
			void add(Move move) noexcept
			{
				m_ptr[m_size].init(move, Score());
				m_size++;
			}
			void add(Move move, Score score) noexcept
			{
				m_ptr[m_size].init(move, score);
				m_size++;
			}
			void removeAction(size_t index) noexcept
			{
				assert(index < m_size);
				for (size_t i = index + 1; i < m_size; i++)
					m_ptr[i - 1] = m_ptr[i];
				m_size--;
			}
			Score getScore(Score result = Score::loss()) const noexcept
			{
				if (size() == 0)
					return Score();
				for (auto iter = begin(); iter < end(); iter++)
					result = std::max(result, iter->score);
				return result;
			}
			void print() const
			{
				std::cout << "List of actions:\n";
				std::cout << "(is in danger = " << is_in_danger << ", is threatening = " << is_threatening << ")\n";
				for (int i = 0; i < size(); i++)
					std::cout << i << " : " << m_ptr[i].move.toString() << " : " << m_ptr[i].move.text() << " : " << m_ptr[i].score << '\n';
				std::cout << '\n';
			}
	};

	class ActionStack
	{
			std::vector<Action> m_data;
		public:
			ActionStack(size_t size) :
					m_data(size)
			{
			}
			ActionList getRoot() noexcept
			{
				return ActionList(m_data.data());
			}
			size_t size() const noexcept
			{
				return m_data.size();
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_ACTION_LIST_HPP_ */
