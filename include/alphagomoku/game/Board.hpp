/*
 * Board.hpp
 *
 *  Created on: Jul 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_GAME_BOARD_HPP_
#define ALPHAGOMOKU_GAME_BOARD_HPP_

#include <alphagomoku/utils/matrix.hpp>

namespace ag
{
	struct Move;
	enum class ProvenValue
	;
	struct Value;
}

namespace ag
{
	enum class Sign
	{
		NONE, // empty spot on board
		CROSS, // or black
		CIRCLE, // or white
		ILLEGAL // typically outside of board
	};
	inline Sign invertSign(Sign sign) noexcept
	{
		switch (sign)
		{
			default:
				return Sign::NONE;
			case Sign::CROSS:
				return Sign::CIRCLE;
			case Sign::CIRCLE:
				return Sign::CROSS;
		}
	}
	std::string toString(Sign sign);
	Sign signFromString(const std::string &str);
	std::ostream& operator<<(std::ostream &stream, Sign sign);
	std::string operator+(const std::string &lhs, Sign rhs);
	std::string operator+(Sign lhs, const std::string &rhs);

	enum class GameRules
	{
		FREESTYLE,
		STANDARD,
		RENJU,
		CARO
	};
	std::string toString(GameRules rules);
	GameRules rulesFromString(const std::string &str);

	enum class GameOutcome
	{
		UNKNOWN,
		DRAW,
		CROSS_WIN, // or black win
		CIRCLE_WIN // or white win
	};
	std::string toString(GameOutcome outcome);
	GameOutcome outcomeFromString(const std::string &str);

	class Board
	{
		private:
			matrix<Sign> board_data;
			Sign sign_to_move;
			GameRules game_rules;
		public:
			Board(int rows, int cols, GameRules rules);
			Board(const std::string &str, Sign signToMove, GameRules rules);

			int rows() const noexcept
			{
				return board_data.rows();
			}
			int cols() const noexcept
			{
				return board_data.cols();
			}
			Sign signToMove() const noexcept
			{
				return sign_to_move;
			}
			GameRules rules() const noexcept
			{
				return game_rules;
			}
			Sign& at(int row, int col) noexcept
			{
				return board_data.at(row, col);
			}
			Sign at(int row, int col) const noexcept
			{
				return board_data.at(row, col);
			}
			Sign* data() noexcept
			{
				return board_data.data();
			}
			const Sign* data() const noexcept
			{
				return board_data.data();
			}

			Sign& at(const std::string &text) noexcept;
			Sign at(const std::string &text) const noexcept;

			bool isSquare() const noexcept;
			bool isEmpty() const noexcept;
			bool isFull() const noexcept;
			bool isForbidden(Move m) const noexcept;
			GameOutcome getOutcome() const noexcept;
			GameOutcome getOutcome(Move last_move) const noexcept;

			std::string toString() const;
			std::string toString(const matrix<ProvenValue> &pv) const;
			std::string toString(const matrix<float> &policy) const;
			std::string toString(const matrix<Value> &actionValues) const;

	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_GAME_BOARD_HPP_ */
