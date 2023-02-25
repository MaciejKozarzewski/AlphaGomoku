/*
 * Game.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_GAME_GAME_HPP_
#define ALPHAGOMOKU_GAME_GAME_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <vector>
#include <cinttypes>

class Json;
class SerializedObject;

namespace ag
{

	class Game
	{
		private:
			GameConfig game_config;

			std::vector<Move> played_moves;
			matrix<Sign> current_board;

			std::string cross_player_name;
			std::string circle_player_name;
		public:
			Game(GameConfig config);
			Game(const Json &json, const SerializedObject &binary_data);

			const GameConfig& getConfig() const noexcept;
			int rows() const noexcept;
			int cols() const noexcept;
			GameRules getRules() const noexcept;

			void beginGame();
			void loadOpening(const std::vector<Move> &moves);
			Sign getSignToMove() const noexcept;
			Move getLastMove() const noexcept;
			Move getMove(int index) const noexcept;
			const std::vector<Move> getMoves() const noexcept;
			const matrix<Sign>& getBoard() const noexcept;

			int numberOfMoves() const noexcept;
			void undoMove(Move move);
			void makeMove(Move move);

			bool isOver() const;
			bool isDraw() const;
			GameOutcome getOutcome() const noexcept;

			void setPlayers(const std::string &crossPlayerName, const std::string &circlePlayerName);
			std::string generatePGN(bool fullGameHistory = false) const;

			Json serialize(SerializedObject &binary_data) const;
	};
}

#endif /* ALPHAGOMOKU_GAME_GAME_HPP_ */
