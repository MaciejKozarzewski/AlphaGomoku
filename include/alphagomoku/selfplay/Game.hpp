/*
 * Game.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_GAME_HPP_
#define ALPHAGOMOKU_SELFPLAY_GAME_HPP_

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
	class EvaluationRequest;
	class SearchData;
} /* namespace ag */

namespace ag
{

	class Game
	{
		private:
			GameConfig game_config;

			std::vector<SearchData> search_data;
			std::vector<Move> played_moves;
			matrix<Sign> current_board;

			mutable std::vector<int> use_count;
			GameOutcome outcome = GameOutcome::UNKNOWN;

			std::string cross_player_name;
			std::string circle_player_name;
		public:
			Game(GameConfig config);
			Game(const Json &json, const SerializedObject &binary_data);

			const GameConfig& getConfig() const noexcept;
			int rows() const noexcept;
			int cols() const noexcept;
			GameRules getRules() const noexcept;
			int length() const noexcept;
			void beginGame();
			Sign getSignToMove() const noexcept;
			Move getLastMove() const noexcept;
			const matrix<Sign>& getBoard() const noexcept;
			void loadOpening(const std::vector<Move> &moves);
			void undoMove(Move move);
			void makeMove(Move move);
			void addSearchData(const SearchData &state);
			void resolveOutcome(int numberfOfMovesForDraw = -1);
			bool isOver() const;
			bool isDraw() const;
			GameOutcome getOutcome() const noexcept;
			void setPlayers(const std::string &crossPlayerName, const std::string &circlePlayerName);
			std::string generatePGN(bool fullGameHistory = false) const;

			bool isCorrect() const;
			int getNumberOfSamples() const noexcept;
			const SearchData& getSample(int index = -1) const;
			SearchData& getSample(int index = -1);
			int randomizeState() const;

			Json serialize(SerializedObject &binary_data) const;
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_GAME_HPP_ */
