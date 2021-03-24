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
	GameState::GameState(const SerializedObject &so, size_t &offset)
	{
		int rows = so.load<int>(offset);
		offset += sizeof(rows);
		int cols = so.load<int>(offset);
		offset += sizeof(cols);

		state = matrix<uint16_t>(rows, cols);
		so.load(state.data(), offset, state.sizeInBytes());
		offset += state.sizeInBytes();
		minimax_value = so.load<float>(offset);
		offset += sizeof(minimax_value);
		proven_value = static_cast<ProvenValue>(so.load<int>(offset));
		offset += sizeof(int);
		move = so.load<uint16_t>(offset);
		offset += sizeof(move);

//		for (int i = 0; i < state.size(); i++)
//			switch (state.data()[i] & 3)
//			{
//				case 0:
//					state.data()[i] = (state.data()[i] & 65532) | static_cast<int>(Sign::CIRCLE);
//					break;
//				case 1:
//					state.data()[i] = (state.data()[i] & 65532) | static_cast<int>(Sign::NONE);
//					break;
//				case 2:
//					state.data()[i] = (state.data()[i] & 65532) | static_cast<int>(Sign::CROSS);
//					break;
//			}
//		int x = ((move >> 2) & 127) - 64;
//		int y = ((move >> 9) & 127) - 64;
//		switch (move & 3)
//		{
//			case 0:
//				move = Move::move_to_short(x, y, Sign::CIRCLE);
//				break;
//			case 1:
//				move = Move::move_to_short(x, y, Sign::NONE);
//				break;
//			case 2:
//				move = Move::move_to_short(x, y, Sign::CROSS);
//				break;
//		}
	}
	GameState::GameState(const matrix<Sign> &board, const matrix<float> &policy, float minimax, Move m, ProvenValue pv) :
			state(board.rows(), board.cols()),
			minimax_value(minimax),
			proven_value(pv),
			move(m.toShort())
	{
		for (int i = 0; i < board.size(); i++)
			state.data()[i] = (static_cast<int>(board.data()[i]) & 3) | (static_cast<int>(policy.data()[i] * 16383) << 2);
	}
	GameState::GameState(Move m) :
			move(m.toShort())
	{
	}
	void GameState::copyTo(matrix<Sign> &board, matrix<float> &policy, Sign &signToMove) const
	{
		signToMove = Move::getSign(move);
		static const float scale = 1.0f / 16383.0f;
		for (int i = 0; i < board.size(); i++)
		{
			uint16_t tmp = state.data()[i];
			board.data()[i] = static_cast<Sign>(tmp & 3);
			policy.data()[i] = (tmp >> 2) * scale;
		}
	}
	void GameState::serialize(SerializedObject &binary_data) const
	{
		binary_data.save<int>(state.rows());
		binary_data.save<int>(state.cols());
		binary_data.save(state.data(), state.sizeInBytes());
		binary_data.save<float>(minimax_value);
		binary_data.save<int>(static_cast<int>(proven_value));
		binary_data.save<uint16_t>(move);
	}
	bool GameState::isCorrect() const noexcept
	{
		if (minimax_value < 0.0f || minimax_value > 1.0f)
			return false;

		Move m(move);
		if (m.row < 0 || m.row >= state.rows())
			return false;
		if (m.col < 0 || m.col >= state.cols())
			return false;
		if (m.sign != Sign::CROSS && m.sign != Sign::CIRCLE)
			return false;

		matrix<Sign> board(state.rows(), state.cols());
		matrix<float> policy(state.rows(), state.cols());
		Sign sign_to_move;
		copyTo(board, policy, sign_to_move);

		int count_X = 0, count_O = 0, count_empty = 0;
		for (int i = 0; i < state.rows(); i++)
			for (int j = 0; j < state.cols(); j++)
			{
				if (board.at(i, j) < Sign::CIRCLE || board.at(i, j) > Sign::CROSS)
					return false;
				if (board.at(i, j) == Sign::NONE)
					count_empty++;
				else
				{
					if (board.at(i, j) == Sign::CROSS)
						count_X++;
					if (board.at(i, j) == Sign::CIRCLE)
						count_O++;
				}
			}
		if ((count_O + count_X + count_empty) != state.size())
			return false;
		if (abs(count_O - count_X) > 1)
			return false;

		float policy_sum = 0.0f;
		for (int i = 0; i < state.rows(); i++)
			for (int j = 0; j < state.cols(); j++)
			{
				if (policy.at(i, j) < 0.0f || policy.at(i, j) > 1.0f)
					return false;
				policy_sum += policy.at(i, j);
			}
		if (abs(policy_sum - 1.0f) > 0.1f)
			return false;

		for (int i = 0; i < state.rows(); i++)
			for (int j = 0; j < state.cols(); j++)
				if (board.at(i, j) != Sign::NONE && policy.at(i, j) != 0.0)
				{
					std::cout << i << "," << j << " " << board.at(i, j) << " " << policy.at(i, j) << std::endl;
					std::cout << policyToString(board, policy) << '\n';
					board.clear();
					std::cout << boardToString(board) << '\n';
					return false;
				}

		return true;
	}

	Game::Game(GameConfig config) :
			states(),
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
		size_t nb_of_states = json["nb_of_states"];
		for (size_t i = 0; i < nb_of_states; i++)
			states.push_back(GameState(binary_data, offset));
	}
	Game::Game(const SerializedObject &so, size_t &offset)
	{
		int board_size;
		so.load(&board_size, offset, sizeof(int));
		offset += sizeof(int);

		so.load(&rules, offset, sizeof(int));
		rules = static_cast<GameRules>(static_cast<int>(rules) - 1);
		offset += sizeof(int);

		int out;
		so.load(&out, offset, sizeof(int));
		offset += sizeof(int);
		switch (out)
		{
			case -1:
				outcome = GameOutcome::CIRCLE_WIN;
				break;
			case 0:
				outcome = GameOutcome::DRAW;
				break;
			case 1:
				outcome = GameOutcome::CROSS_WIN;
				break;
		}
		int nb_of_states;
		so.load(&nb_of_states, offset, sizeof(int));
		offset += sizeof(int);

		use_count.resize(board_size * board_size);
		current_board = matrix<Sign>(board_size, board_size);

		for (int i = 0; i < nb_of_states; i++)
			states.push_back(GameState(so, offset));
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
		return states.size();
	}
	void Game::beginGame()
	{
		states.clear();
		use_count.clear();
		current_board.clear();
		outcome = GameOutcome::UNKNOWN;
		states.clear();
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
			return Move(states.back().move);
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
	void Game::makeMove(Move move)
	{
		assert(move.row >= 0 && move.row < rows());
		assert(move.col >= 0 && move.col < cols());
		assert(move.sign == sign_to_move);
		assert(current_board.at(move.row, move.col) == Sign::NONE);

		states.push_back(GameState(move));
		current_board.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(sign_to_move);
	}
	void Game::makeMove(Move move, const matrix<float> &policy, float minimax, ProvenValue pv)
	{
		assert(move.row >= 0 && move.row < rows());
		assert(move.col >= 0 && move.col < cols());
		assert(current_board.at(move.row, move.col) == Sign::NONE);
		assert(rows() == policy.rows());
		assert(cols() == policy.cols());
		assert(move.sign == sign_to_move);

		states.push_back(GameState(current_board, policy, minimax, move, pv));

		current_board.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(sign_to_move);
	}
	void Game::resolveOutcome()
	{
		outcome = ag::getOutcome(rules, current_board);
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
		for (size_t i = 0; i < states.size(); i++)
			if (states[i].isCorrect() == false)
				return false;
		return true;
	}
	int Game::getNumberOfSamples() const noexcept
	{
		return states.size();
	}
	void Game::getSample(matrix<Sign> &board, matrix<float> &policy, Sign &signToMove, GameOutcome &gameOutcome, int index)
	{
		if (index == -1)
			index = randomizeState();
		use_count[index]++;

		states[index].copyTo(board, policy, signToMove);
		gameOutcome = outcome;
	}
	int Game::randomizeState()
	{
		float min = use_count[0] + 1.0f;
		int idx = 0;
		for (size_t i = 1; i < states.size(); i++)
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

	void Game::printSample(int index) const
	{
		matrix<Sign> board(rows(), cols());
		matrix<float> policy(rows(), cols());
		Sign sign_to_move;
		states[index].copyTo(board, policy, sign_to_move);
		std::cout << "now moving " << Move(states[index].move).toString() << std::endl;
		std::cout << "minimax " << 1.0f - states[index].minimax_value << std::endl;
		std::cout << "outcome " << toString(outcome) << std::endl;
		std::cout << policyToString(board, policy);
	}

	Json Game::serialize(SerializedObject &binary_data) const
	{
		Json result;
		result["rows"] = rows();
		result["cols"] = cols();
		result["rules"] = toString(rules);
		result["outcome"] = toString(outcome);
		result["nb_of_states"] = states.size();
		result["binary_offset"] = binary_data.size();
		for (size_t i = 0; i < states.size(); i++)
			states[i].serialize(binary_data);
		return result;
	}
}

