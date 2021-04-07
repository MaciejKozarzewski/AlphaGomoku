/*
 * Game.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <libml/utils/json.hpp>

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
		for (size_t i = 0; i < nb_of_moves; i++, offset += sizeof(uint16_t))
			played_moves.push_back(Move(binary_data.load<uint16_t>(offset)));

		size_t nb_of_states = json["nb_of_states"];
		for (size_t i = 0; i < nb_of_states; i++)
			search_data.push_back(SearchData(binary_data, offset));
	}

	GameConfig Game::getConfig() const noexcept
	{
		GameConfig result;
		result.rows = rows();
		result.cols = cols();
		result.rules = getRules();
		return result;
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
		sign_to_move = Sign::CROSS;
	}
	Sign Game::getSignToMove() const noexcept
	{
		return sign_to_move;
	}
	Move Game::getLastMove() const noexcept
	{
		if (length() == 0)
			return Move(0, 0, invertSign(sign_to_move));
		else
			return Move(played_moves.back());
	}
	const matrix<Sign>& Game::getBoard() const noexcept
	{
		return current_board;
	}
	void Game::setBoard(const matrix<Sign> &other, Sign signToMove)
	{
		this->current_board = other;
		this->sign_to_move = signToMove;
	}
	void Game::loadOpening(const std::vector<Move> &moves)
	{
		beginGame();
		sign_to_move = (moves.size() == 0) ? Sign::CROSS : moves[0].sign;
		for (size_t i = 0; i < moves.size(); i++)
			makeMove(moves[i]);
	}
	void Game::undoMove(Move move)
	{
		assert(move.row >= 0 && move.row < rows());
		assert(move.col >= 0 && move.col < cols());
		assert(move.sign == invertSign(sign_to_move));
		assert(current_board.at(move.row, move.col) != Sign::NONE);

		current_board.at(move.row, move.col) = Sign::NONE;
		sign_to_move = invertSign(sign_to_move);
	}
	void Game::makeMove(Move move)
	{
		assert(move.row >= 0 && move.row < rows());
		assert(move.col >= 0 && move.col < cols());
		assert(move.sign == sign_to_move);
		assert(current_board.at(move.row, move.col) == Sign::NONE);

		current_board.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(sign_to_move);
	}
	void Game::addSearchData(const SearchData &state)
	{
		search_data.push_back(state);
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
	const SearchData& Game::getSample(int index)
	{
		if (index == -1)
			index = randomizeState();
		use_count[index]++;

		return search_data[index];
	}
	int Game::randomizeState()
	{
		float min = use_count[0] + 1.0f;
		int idx = 0;
		for (size_t i = 1; i < search_data.size(); i++)
		{
			float tmp = use_count[i] + randFloat();
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
			binary_data.save(played_moves[i].toShort());
		for (size_t i = 0; i < search_data.size(); i++)
			search_data[i].serialize(binary_data);
		return result;
	}
}

