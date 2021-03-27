/*
 * BufferEntry.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAME_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAME_HPP_

#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/utils/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <libml/utils/serialization.hpp>

#include <vector>
#include <bits/stdint-uintn.h>

namespace ag
{
	class EvaluationRequest;
} /* namespace ag */

namespace ag
{
	struct GameState
	{
			matrix<uint16_t> state;
			Value minimax_value = 0.0f;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			uint16_t move = 0;

			GameState(const SerializedObject &so, size_t &offset);
			GameState(const matrix<Sign> &board, const matrix<float> &policy, Value minimax, Move m, ProvenValue pv);
			GameState(Move m);
			void copyTo(matrix<Sign> &board, matrix<float> &policy, Sign &signToMove) const;
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
			Game(GameConfig config);
			Game(const Json &json, const SerializedObject &binary_data);
			Game(const SerializedObject &so, size_t &offset);

			GameConfig getConfig() const noexcept;
			int rows() const noexcept;
			int cols() const noexcept;
			GameRules getRules() const noexcept;
			int length() const noexcept;
			void beginGame();
			Sign getSignToMove() const noexcept;
			Move getLastMove() const noexcept;
			const matrix<Sign>& getBoard() const noexcept;
			void setBoard(const matrix<Sign> &other, Sign signToMove);
			void loadOpening(const std::vector<Move> &moves);
			void makeMove(Move move);
			void makeMove(Move move, const matrix<float> &policy, Value minimax, ProvenValue pv);
			void resolveOutcome();
			bool isOver() const;
			bool isDraw() const;
			GameOutcome getOutcome() const noexcept;

			bool isCorrect() const;
			int getNumberOfSamples() const noexcept;
			void getSample(matrix<Sign> &board, matrix<float> &policy, Sign &signToMove, GameOutcome &gameOutcome, int index = -1);
			int randomizeState();
			void printSample(int index) const;

			Json serialize(SerializedObject &binary_data) const;
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_GAME_HPP_ */
