/*
 * Board.hpp
 *
 *  Created on: Jul 30, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_GAME_BOARD_HPP_
#define ALPHAGOMOKU_GAME_BOARD_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/game/Move.hpp>

namespace ag
{

	class Board
	{
		public:
			static matrix<Sign> fromString(const std::string &str);
			static bool isValid(const matrix<Sign> &board, Sign signToMove) noexcept;
			static bool isEmpty(const matrix<Sign> &board) noexcept;
			static bool isFull(const matrix<Sign> &board) noexcept;
			static bool isForbidden(const matrix<Sign> &board, Move move) noexcept;
			static bool isTransitionPossible(const matrix<Sign> &from, const matrix<Sign> &to) noexcept;

			static int numberOfMoves(const matrix<Sign> &board) noexcept;
			static GameOutcome getOutcome(const matrix<Sign> &board) noexcept;
			static GameOutcome getOutcome(const matrix<Sign> &board, Move lastMove) noexcept;

			static void putMove(matrix<Sign> &board, Move move) noexcept;
			static void undoMove(matrix<Sign> &board, Move move) noexcept;

			static std::string toString(const matrix<Sign> &board);
			static std::string toString(const matrix<Sign> &board, const matrix<ProvenValue> &pv);
			static std::string toString(const matrix<Sign> &board, const matrix<float> &policy);
			static std::string toString(const matrix<Sign> &board, const matrix<Value> &actionValues);
	};
} /* namespace ag */

#endif /* ALPHAGOMOKU_GAME_BOARD_HPP_ */
