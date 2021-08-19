/*
 * Move.hpp
 *
 *  Created on: Feb 20, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_MOVE_HPP_
#define ALPHAGOMOKU_MCTS_MOVE_HPP_

#include <alphagomoku/game/Board.hpp>

#include <string>
#include <cassert>

class Json;
namespace ag
{

	struct Move
	{
			int16_t row = 0;
			int16_t col = 0;
			Sign sign = Sign::NONE;

			Move() = default;
			Move(const Json &json);
			Move(uint16_t m)
			{
				sign = static_cast<Sign>(m & 3);
				row = (m >> 2) & 127;
				col = (m >> 9) & 127;
			}
			Move(int r, int c) :
					Move(r, c, Sign::NONE)
			{
			}
			Move(int r, int c, Sign s) :
					row(r),
					col(c),
					sign(s)
			{
				assert(row >= 0 && row < 128);
				assert(col >= 0 && col < 128);
			}

			uint16_t toShort() const noexcept
			{
				return static_cast<uint16_t>(sign) + (row << 2) + (col << 9);
			}
			std::string toString() const;
			Json serialize() const;

			static uint16_t move_to_short(int r, int c, Sign s)
			{
				return static_cast<uint16_t>(s) + (r << 2) + (c << 9);
			}
			static int getRow(uint16_t move)
			{
				return (move >> 2) & 127;
			}
			static int getCol(uint16_t move)
			{
				return (move >> 9) & 127;
			}
			static Sign getSign(uint16_t move)
			{
				return static_cast<Sign>(move & 3);
			}
			static Move fromText(const std::string &text);

			friend bool operator==(const Move &lhs, const Move &rhs) noexcept
			{
				return (lhs.row == rhs.row) && (lhs.col == rhs.col) && (lhs.sign == rhs.sign);
			}
			friend bool operator!=(const Move &lhs, const Move &rhs) noexcept
			{
				return (lhs.row != rhs.row) || (lhs.col != rhs.col) || (lhs.sign != rhs.sign);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_MOVE_HPP_ */
