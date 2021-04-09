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
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <libml/utils/serialization.hpp>

#include <vector>
#include <inttypes.h>

namespace ag
{
	class EvaluationRequest;
} /* namespace ag */

namespace ag
{

	class Game
	{
		private:
			std::vector<SearchData> search_data;
			std::vector<Move> played_moves;
			matrix<Sign> current_board;
			std::vector<int> use_count;
			GameRules rules;
			GameOutcome outcome = GameOutcome::UNKNOWN;

			std::string cross_player_name;
			std::string circle_player_name;

		public:
			Game(GameConfig config);
			Game(const Json &json, const SerializedObject &binary_data);

			GameConfig getConfig() const noexcept;
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
			void resolveOutcome();
			bool isOver() const;
			bool isDraw() const;
			GameOutcome getOutcome() const noexcept;
			void setPlayers(const std::string &crossPlayerName, const std::string &circlePlayerName);
			std::string generatePGN(bool fullGameHistory = false) const;

			bool isCorrect() const;
			int getNumberOfSamples() const noexcept;
			const SearchData& getSample(int index = -1);
			int randomizeState();

			Json serialize(SerializedObject &binary_data) const;
	};
}

#endif /* ALPHAGOMOKU_SELFPLAY_GAME_HPP_ */
