/*
 * Game.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace ag
{

	Game::Game(GameConfig config) :
			search_data(),
			played_moves(),
			current_board(config.rows, config.cols),
			use_count(current_board.size(), 0),
			rules(config.rules)
	{
	}
	Game::Game(const Json &json, const SerializedObject &binary_data) :
			current_board(static_cast<int>(json["rows"]), static_cast<int>(json["cols"])),
			use_count(current_board.size(), 0),
			rules(rulesFromString(json["rules"])),
			outcome(outcomeFromString(json["outcome"]))
	{
		size_t offset = json["binary_offset"];
		size_t nb_of_moves = json["nb_of_moves"];
		for (size_t i = 0; i < nb_of_moves; i++, offset += sizeof(Move))
			played_moves.push_back(binary_data.load<Move>(offset));

		size_t nb_of_states = json["nb_of_states"];
		for (size_t i = 0; i < nb_of_states; i++)
			addSearchData(SearchData(binary_data, offset));
	}

	GameConfig Game::getConfig() const noexcept
	{
		return GameConfig(getRules(), rows(), cols());
	}
	int Game::rows() const noexcept
	{
		return current_board.rows();
	}
	int Game::cols() const noexcept
	{
		return current_board.cols();
	}
	GameRules Game::getRules() const noexcept
	{
		return rules;
	}
	int Game::length() const noexcept
	{
		return played_moves.size();
	}
	void Game::beginGame()
	{
		search_data.clear();
		played_moves.clear();
		use_count.clear();
		current_board.clear();
		outcome = GameOutcome::UNKNOWN;
	}
	Sign Game::getSignToMove() const noexcept
	{
		return invertSign(getLastMove().sign);
	}
	Move Game::getLastMove() const noexcept
	{
		if (played_moves.empty())
			return Move(Sign::CIRCLE, 0, 0); // fake move so the first player starts with CROSS sign
		else
			return Move(played_moves.back());
	}
	const matrix<Sign>& Game::getBoard() const noexcept
	{
		return current_board;
	}
	void Game::loadOpening(const std::vector<Move> &moves)
	{
		beginGame();
		for (size_t i = 0; i < moves.size(); i++)
			makeMove(moves[i]);
	}
	void Game::undoMove(Move move)
	{
		assert(move.row >= 0 && move.row < rows());
		assert(move.col >= 0 && move.col < cols());
		assert(length() > 0);
		assert(move == getLastMove());
		assert(current_board.at(move.row, move.col) == move.sign);

		played_moves.pop_back();
		current_board.at(move.row, move.col) = Sign::NONE;
	}
	void Game::makeMove(Move move)
	{
		assert(move.row >= 0 && move.row < rows());
		assert(move.col >= 0 && move.col < cols());
		assert(move.sign == getSignToMove());
		assert(current_board.at(move.row, move.col) == Sign::NONE);

		played_moves.push_back(move);
		current_board.at(move.row, move.col) = move.sign;
	}
	void Game::addSearchData(const SearchData &state)
	{
		search_data.push_back(state);
		use_count.push_back(0);
	}
	void Game::resolveOutcome()
	{
		outcome = ag::getOutcome(rules, current_board);
		if (outcome != GameOutcome::UNKNOWN)
			for (size_t i = 0; i < search_data.size(); i++)
				search_data[i].setOutcome(outcome);
	}
	bool Game::isOver() const
	{
		return ag::getOutcome(rules, current_board) != GameOutcome::UNKNOWN;
	}
	bool Game::isDraw() const
	{
		return isBoardFull(current_board);
	}
	GameOutcome Game::getOutcome() const noexcept
	{
		return outcome;
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

	bool Game::isCorrect() const
	{
		if (outcome == GameOutcome::UNKNOWN)
			return false;
		for (size_t i = 0; i < search_data.size(); i++)
			if (search_data[i].isCorrect() == false)
				return false;
		return true;
	}
	int Game::getNumberOfSamples() const noexcept
	{
		return search_data.size();
	}
	const SearchData& Game::getSample(int index) const
	{
		if (index == -1)
			index = randomizeState();
		use_count[index]++;

		return search_data.at(index);
	}
	SearchData& Game::getSample(int index)
	{
		if (index == -1)
			index = randomizeState();
		use_count[index]++;
		return search_data.at(index);
	}
	int Game::randomizeState() const
	{
		float min = use_count.at(0) + 1.0f;
		int idx = 0;
		for (size_t i = 1; i < search_data.size(); i++)
		{
			float tmp = use_count.at(i) + randFloat();
			if (tmp < min)
			{
				min = tmp;
				idx = i;
			}
		}
		return idx;
	}

	Json Game::serialize(SerializedObject &binary_data) const
	{
		Json result;
		result["rows"] = rows();
		result["cols"] = cols();
		result["rules"] = toString(rules);
		result["outcome"] = toString(outcome);
		result["nb_of_moves"] = played_moves.size();
		result["nb_of_states"] = search_data.size();
		result["binary_offset"] = binary_data.size();
		for (size_t i = 0; i < played_moves.size(); i++)
			binary_data.save<Move>(played_moves[i]);
//			binary_data.save(played_moves[i].toShort());

		for (size_t i = 0; i < search_data.size(); i++)
			search_data[i].serialize(binary_data);
		return result;
	}
}

