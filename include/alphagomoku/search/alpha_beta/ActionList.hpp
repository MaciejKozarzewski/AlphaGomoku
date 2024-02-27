/*
 * ActionList.hpp
 *
 *  Created on: Oct 4, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_ACTION_LIST_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_ACTION_LIST_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/Score.hpp>

#include <algorithm>
#include <iostream>
#include <cassert>

namespace ag
{
	struct Action
	{
			int32_t offset = -1;
			Move move;
			Score score;
			int16_t size = 0;
			Action() noexcept = default;
			void init(Move m, Score s) noexcept
			{
				offset = -1;
				move = m;
				score = s;
				size = 0;
			}

			friend bool operator<(const Action &lhs, const Action &rhs) noexcept
			{
				return lhs.score < rhs.score;
			}
	};

	class ActionStack;

	class ActionList
	{
			friend class ActionStack;

			Action *m_children = nullptr;
			size_t m_size = 0;
			int m_distance_from_root = 0;
			ActionList(Action *ptr, size_t size) noexcept :
					m_children(ptr),
					m_size(size)
			{
			}
		public:
			Score baseline_score; /* score of actions that have not been added to the list (for example moves other than defensive) */
			Move last_move;
			bool is_fully_expanded = false;
			bool has_initiative = false;
			bool must_defend = false;
			bool performed_pattern_update = false;

			bool isRoot() const noexcept
			{
				return distanceFromRoot() == 0;
			}
			int distanceFromRoot() const noexcept
			{
				return m_distance_from_root;
			}
			void clear() noexcept
			{
				baseline_score = Score();
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
			bool isEmpty() const noexcept
			{
				return size() == 0;
			}
			bool contains(Move m) const noexcept
			{
				for (auto iter = begin(); iter < end(); iter++)
					if (iter->move == m)
						return true;
				return false;
			}
			bool contains(Location m) const noexcept
			{
				for (auto iter = begin(); iter < end(); iter++)
					if (iter->move.location() == m)
						return true;
				return false;
			}
			void add(Move move, Score score, int num = 1) noexcept
			{
				m_children[m_size].init(move, score);
				m_size += num;
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
			Move getBestMove() const noexcept
			{
				Score best_score = Score::min_value();
				Move result;
				for (auto iter = begin(); iter < end(); iter++)
					if (iter->score >= best_score)
					{
						best_score = iter->score;
						result = iter->move;
					}
				return result;
			}
			Score getScoreOf(Move m) const noexcept
			{
				for (auto iter = begin(); iter < end(); iter++)
					if (iter->move == m)
						return iter->score;
				return Score();
			}
			void print() const
			{
				std::cout << "List of " << size() << " actions:\n";
				std::cout << "(must defend = " << must_defend << ", has initiative = " << has_initiative << ")\n";
				std::cout << "baseline score = " << baseline_score.toString() << '\n';
				for (int i = 0; i < size(); i++)
					std::cout << i << " : " << m_children[i].move.toString() << " : " << m_children[i].move.text() << " : " << m_children[i].score
							<< '\n';
				std::cout << '\n';
			}
			bool equals(const ActionList &other) const noexcept
			{
				if (this->size() != other.size() or this->is_fully_expanded != other.is_fully_expanded or this->must_defend != other.must_defend
						or this->has_initiative != other.has_initiative)
					return false;
				for (int i = 0; i < other.size(); i++)
					if (not this->contains(other[i].move))
						return false;
				return true;
			}
	};

	class ActionStack
	{
			std::vector<Action> m_data;
			size_t m_offset = 1; // offset equal zero is reserved
		public:
			ActionStack(size_t size = 0) :
					m_data(size)
			{
			}
			void resize(size_t newSize)
			{
				m_data.resize(1 + newSize);
			}
			ActionList create_root() noexcept
			{
				m_offset = 1;
				return ActionList(m_data.data(), 0);
			}
			ActionList create_from_actions(ActionList &parent, int index) noexcept
			{
				Action &a = parent[index];
				if (a.offset == -1)
				{
					assert(a.size == 0);
					a.offset = m_offset;
				}
				ActionList result(m_data.data() + a.offset, a.size);
				result.m_distance_from_root = parent.m_distance_from_root + 1;
				result.last_move = a.move;
				return result;
			}
			void increment(size_t num = 1) noexcept
			{
				assert(m_offset + num < size());
				m_offset += num;
			}
			void decrement(size_t num = 1) noexcept
			{
				assert(0 <= m_offset - num);
				m_offset -= num;
			}
			size_t size() const noexcept
			{
				return m_data.size();
			}
			size_t offset() const noexcept
			{
				return m_offset;
			}
			Action& operator[](size_t index)
			{
				assert(index < size());
				return m_data[index];
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_ACTION_LIST_HPP_ */
