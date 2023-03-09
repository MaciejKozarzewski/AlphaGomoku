/*
 * Game.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Game.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <cstdlib>
#include <string>

namespace ag
{

	Game::Game(GameConfig config) :
			game_config(config),
			played_moves(),
			current_board(config.rows, config.cols)
	{
	}
	Game::Game(const Json &json, const SerializedObject &binary_data) :
			game_config(json["game_config"]),
			current_board(game_config.rows, game_config.cols)
	{
		const size_t nb_of_moves = json["moves"].size();
		for (size_t i = 0; i < nb_of_moves; i++)
			makeMove(Move(json["moves"][i].getString()));
	}

	const GameConfig& Game::getConfig() const noexcept
	{
		return game_config;
	}
	int Game::rows() const noexcept
	{
		return game_config.rows;
	}
	int Game::cols() const noexcept
	{
		return game_config.cols;
	}
	GameRules Game::getRules() const noexcept
	{
		return game_config.rules;
	}

	void Game::beginGame()
	{
		played_moves.clear();
		current_board.clear();
	}
	void Game::loadOpening(const std::vector<Move> &moves)
	{
		beginGame();
		for (size_t i = 0; i < moves.size(); i++)
			makeMove(moves[i]);
	}
	Sign Game::getSignToMove() const noexcept
	{
		return invertSign(getLastMove().sign);
	}
	Move Game::getLastMove() const noexcept
	{
		if (played_moves.empty())
			return Move(Sign::CIRCLE, 0, 0); // fake move so the first player starts with CROSS (black) sign
		else
			return Move(played_moves.back());
	}
	Move Game::getMove(int index) const noexcept
	{
		return played_moves.at(index);
	}
	const std::vector<Move> Game::getMoves() const noexcept
	{
		return played_moves;
	}
	const matrix<Sign>& Game::getBoard() const noexcept
	{
		return current_board;
	}

	int Game::numberOfMoves() const noexcept
	{
		return played_moves.size();
	}
	void Game::undoMove(Move move)
	{
		assert(numberOfMoves() > 0);
		assert(move == getLastMove());

		Board::undoMove(current_board, move);
		played_moves.pop_back();
	}
	void Game::makeMove(Move move)
	{
		assert(move.sign == getSignToMove());

		Board::putMove(current_board, move);
		played_moves.push_back(move);
	}

	bool Game::isOver() const
	{
		return this->getOutcome() != GameOutcome::UNKNOWN;
	}
	bool Game::isDraw() const
	{
		return Board::numberOfMoves(current_board) >= game_config.draw_after;
	}
	GameOutcome Game::getOutcome() const noexcept
	{
		return ag::getOutcome(game_config.rules, current_board, getLastMove(), game_config.draw_after);
	}
	void Game::setPlayers(const std::string &crossPlayerName, const std::string &circlePlayerName)
	{
		cross_player_name = crossPlayerName;
		circle_player_name = circlePlayerName;
	}
	std::string Game::generatePGN(bool fullGameHistory) const
	{
		std::string result;
		result += "[Event \"Evaluation\"]\n";
		result += "[Site \"N/A\"]\n";
		result += "[Date \"" + currentDateTime() + "\"]\n";
		result += "[Round \"0\"]\n";
		result += "[White \"" + circle_player_name + "\"]\n";
		result += "[Black \"" + cross_player_name + "\"]\n";

		switch (getOutcome())
		{
			case GameOutcome::UNKNOWN:
				throw std::logic_error("cannot save PGN of an unfinished game");
			case GameOutcome::DRAW:
				result += "[Result \"1/2-1/2\"]\n";
				break;
			case GameOutcome::CROSS_WIN:
				result += "[Result \"0-1\"]\n";
				break;
			case GameOutcome::CIRCLE_WIN:
				result += "[Result \"1-0\"]\n";
				break;
		}
		if (fullGameHistory == true)
		{
			// TODO add saving entire game history
		}
		else
			result += "1. N/A\n";
		return result;
	}

	Json Game::serialize(SerializedObject &binary_data) const
	{
		Json result;
		result["game_config"] = game_config.toJson();
		result["outcome"] = toString(getOutcome());
		result["moves"] = Json(JsonType::Array);
		for (size_t i = 0; i < played_moves.size(); i++)
			result["moves"][i] = played_moves[i].text();
		return result;
	}
}

