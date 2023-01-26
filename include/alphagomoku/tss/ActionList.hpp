/*
 * ActionList.hpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TSS_ACTION_LIST_HPP_
#define ALPHAGOMOKU_TSS_ACTION_LIST_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/tss/Score.hpp>

#include <algorithm>
#include <iostream>
#include <cassert>

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
			const Action *m_parent = nullptr;
			Action *m_children = nullptr;
			size_t m_size = 0;
			ActionList(Action *ptr) noexcept :
					m_children(ptr)
			{
			}
		public:
			bool has_initiative = false;
			bool must_defend = false;
			ActionList(const ActionList &prev) noexcept :
					m_children(prev.end())
			{
			}
			ActionList(const ActionList &prev, const Action &parent) noexcept :
					m_parent(&parent),
					m_children(prev.end())
			{
			}
			bool isRoot() const noexcept
			{
				return m_parent == nullptr;
			}
			const Action& getParent() const noexcept
			{
				assert(isRoot() == false);
				return *m_parent;
			}
			void clear() noexcept
			{
				has_initiative = false;
				must_defend = false;
				m_size = 0;
			}
			Action* begin() noexcept
			{
				return m_children;
			}
			Action* begin() const noexcept
			{
				return m_children;
			}
			Action* end() noexcept
			{
				return m_children + m_size;
			}
			Action* end() const noexcept
			{
				return m_children + m_size;
			}
			Action& operator[](size_t index) noexcept
			{
				assert(index < m_size);
				return m_children[index];
			}
			const Action& operator[](size_t index) const noexcept
			{
				assert(index < m_size);
				return m_children[index];
			}
			int size() const noexcept
			{
				return m_size;
			}
			bool contains(Location m) const noexcept
			{
				return std::any_of(begin(), end(), [m](const Action &action)
				{	return action.move.location() == m;});
			}
			void add(Move move, Score score = Score()) noexcept
			{
				m_children[m_size++].init(move, score);
			}
			void removeAction(size_t index) noexcept
			{
				assert(index < m_size);
				for (size_t i = index + 1; i < m_size; i++)
					m_children[i - 1] = m_children[i];
				m_size--;
			}
			bool moveCloserToFront(Move move, size_t offset) noexcept
			{
				for (int i = offset; i < size(); i++)
					if (m_children[i].move == move)
					{
						std::swap(m_children[offset], m_children[i]); // move it closer to the front
						return true;
					}
				return false;
			}
			Score getScore() const noexcept
			{
				Score result = Score::min_value();
				for (auto iter = begin(); iter < end(); iter++)
					result = std::max(result, iter->score);
				if (size() == 0 or (result.isLoss() and not must_defend))
					return Score();
				else
					return result;
			}
			void print() const
			{
				std::cout << "List of actions:\n";
				std::cout << "(must defend = " << must_defend << ", has initiative = " << has_initiative << ")\n";
				for (int i = 0; i < size(); i++)
					std::cout << i << " : " << m_children[i].move.toString() << " : " << m_children[i].move.text() << " : " << m_children[i].score
							<< '\n';
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

#endif /* ALPHAGOMOKU_TSS_ACTION_LIST_HPP_ */
