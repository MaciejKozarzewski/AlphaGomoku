/*
 * BufferEntry.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAME_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAME_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/utils/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <bits/stdint-uintn.h>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>
#include <vector>

namespace ag
{
	class EvaluationRequest;
} /* namespace ag */

namespace ag
{
	struct GameState
	{
			matrix<uint16_t> state;
			float minimax_value = 0.0f;
			uint16_t move = 0;

			GameState(const SerializedObject &so, size_t &offset);
			GameState(const matrix<Sign> &board, const matrix<float> &policy, float minimax, Move m);
			void copyTo(EvaluationRequest &request) const;
			void serialize(SerializedObject &binary_data) const;
			bool isCorrect() const noexcept;
	};

	class Game
	{
		private:
			std::vector<GameState> states;
			matrix<Sign> current_board;
			std::vector<int> use_count;
			GameRules rules;
			Sign sign_to_move = Sign::NONE;
			GameOutcome outcome = GameOutcome::UNKNOWN;

		public:
			Game(GameRules rules, int rows, int cols);
			Game(const Json &json, const SerializedObject &binary_data);
			Game(const SerializedObject &so, size_t &offset);

			int length() const noexcept;
			void beginGame();
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			const matrix<Sign>& getBoard() const noexcept
			{
				return current_board;
			}
			void setBoard(const matrix<Sign> &other, Sign signToMove);
			bool prepareOpening();
			void makeMove(Move move, const matrix<float> &policy, float minimax);
			void resolveOutcome();
			bool isOver() const;
			bool isDraw() const;
			GameOutcome getOutcome() const noexcept
			{
				return outcome;
			}

			bool isCorrect() const;

			int getNumberOfSamples() const;
//			void getSample(NNRequest &request, int index = -1);
			int randomizeState();
			void printSample(int index) const;

			Json saveOpening() const;
			void loadOpening(SerializedObject &so, bool invert_color);

			Json serialize(SerializedObject &binary_data) const;
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_GAME_HPP_ */
