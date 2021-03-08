/*
 * Game.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/EvaluationRequest.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/utils/misc.hpp>
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
		move = so.load<uint16_t>(offset);
		offset += sizeof(move);

		for (int i = 0; i < state.size(); i++)
			switch (state.data()[i] & 3)
			{
				case 0:
					state.data()[i] = (state.data()[i] & 65532) | static_cast<int>(Sign::CIRCLE);
					break;
				case 1:
					state.data()[i] = (state.data()[i] & 65532) | static_cast<int>(Sign::NONE);
					break;
				case 2:
					state.data()[i] = (state.data()[i] & 65532) | static_cast<int>(Sign::CROSS);
					break;
			}
		int x = ((move >> 2) & 127) - 64;
		int y = ((move >> 9) & 127) - 64;
		switch (move & 3)
		{
			case 0:
				move = Move::move_to_short(x, y, Sign::CIRCLE);
				break;
			case 1:
				move = Move::move_to_short(x, y, Sign::NONE);
				break;
			case 2:
				move = Move::move_to_short(x, y, Sign::CROSS);
				break;
		}
	}
	GameState::GameState(const matrix<Sign> &board, const matrix<float> &policy, float minimax, Move m) :
			minimax_value(minimax),
			move(m.toShort())
	{
		for (int i = 0; i < board.size(); i++)
			state.data()[i] = (static_cast<int>(board.data()[i]) & 3) | (static_cast<int>(policy.data()[i] * 16383) << 2);
	}
	void GameState::copyTo(EvaluationRequest &request) const
	{
		request.setSignToMove(Move::getSign(move));
		static const float scale = 1.0f / 16383.0f;
		for (int i = 0; i < request.getPolicy().size(); i++)
		{
			uint16_t tmp = state.data()[i];
			request.getBoard().data()[i] = static_cast<Sign>(tmp & 3);
			request.getPolicy().data()[i] = (tmp >> 2) * scale;
		}
	}
	void GameState::serialize(SerializedObject &binary_data) const
	{
		binary_data.save<int>(state.rows());
		binary_data.save<int>(state.cols());
		binary_data.save(state.data(), state.sizeInBytes());
		binary_data.save<float>(minimax_value);
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

		EvaluationRequest request(state.rows(), state.cols());
		copyTo(request);

		int count_X = 0, count_O = 0, count_empty = 0;
		for (int i = 0; i < state.rows(); i++)
			for (int j = 0; j < state.cols(); j++)
			{
				if (request.getBoard().at(i, j) < Sign::CIRCLE || request.getBoard().at(i, j) > Sign::CROSS)
					return false;
				if (request.getBoard().at(i, j) == Sign::NONE)
					count_empty++;
				else
				{
					if (request.getBoard().at(i, j) == Sign::CROSS)
						count_X++;
					if (request.getBoard().at(i, j) == Sign::CIRCLE)
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
				if (request.getPolicy().at(i, j) < 0.0f || request.getPolicy().at(i, j) > 1.0f)
					return false;
				policy_sum += request.getPolicy().at(i, j);
			}
		if (abs(policy_sum - 1.0f) > 0.1f)
			return false;

		for (int i = 0; i < state.rows(); i++)
			for (int j = 0; j < state.cols(); j++)
				if (request.getBoard().at(i, j) != Sign::NONE && request.getPolicy().at(i, j) != 0.0)
				{
					std::cout << i << "," << j << " " << request.getBoard().at(i, j) << " " << request.getPolicy().at(i, j) << std::endl;
					std::cout << policyToString(request.getBoard(), request.getPolicy()) << '\n';
					request.getBoard().clear();
					std::cout << boardToString(request.getBoard()) << '\n';
					return false;
				}

		return true;
	}

	Game::Game(GameRules rules, int rows, int cols) :
			states(),
			current_board(rows, cols),
			use_count(current_board.size(), 0),
			rules(rules)
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
		std::cout << offset << " " << nb_of_states << std::endl;
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
		if (randBool())
			sign_to_move = Sign::CROSS;
		else
			sign_to_move = Sign::CIRCLE;
	}
	void Game::setBoard(const matrix<Sign> &other, Sign signToMove)
	{
		this->current_board = other;
		this->sign_to_move = signToMove;
	}
	bool Game::prepareOpening()
	{
		matrix<float> map_dist(current_board.rows(), current_board.cols());
		bool flag = true;
		int opening_moves = 0;
		while (flag)
		{
			current_board.clear();
			opening_moves = randInt(6) + randInt(6) + randInt(6);
			for (int i = 0; i < opening_moves; i++)
			{
				generateOpeningMap(current_board, map_dist);
				Move move = randomizeMove(map_dist);
				assert(current_board.at(move.row, move.col) == Sign::NONE);
				current_board.at(move.row, move.col) = sign_to_move;
				sign_to_move = invertSign(sign_to_move);
			}
			flag = isOver();
		}
		return (opening_moves > 1);
	}
	void Game::makeMove(Move move, const matrix<float> &policy, float minimax)
	{
		assert(move.row >= 0 && move.row < current_board.rows());
		assert(move.col >= 0 && move.col < current_board.cols());
		assert(current_board.at(move.row, move.col) == Sign::NONE);
		assert(current_board.rows() == policy.rows());
		assert(current_board.cols() == policy.cols());
		assert(move.sign == sign_to_move);

		states.push_back(GameState(current_board, policy, minimax, move));

		current_board.at(move.row, move.col) = move.sign;
		sign_to_move = invertSign(sign_to_move);
	}
	void Game::resolveOutcome()
	{
		outcome = ag::getOutcome(rules, current_board);
	}
	bool Game::isOver() const
	{
//		return isGameOver(rules, current_board);
	}
	bool Game::isDraw() const
	{
		return isBoardFull(current_board);
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
	int Game::getNumberOfSamples() const
	{
		return states.size();
	}
//	void Game::getSample(NNRequest &request, int index)
//	{
//		if (index == -1)
//			//			index = ml::randInt(states.size());
//			index = randomizeState();
//		use_count[index]++;
//		request.clear();
//		request.value = outcome;
//		states[index]->fill(request);
//		request.is_ready = true;
//	}
	int Game::randomizeState()
	{
		float min = use_count[0] + randFloat();
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
		EvaluationRequest req(current_board.rows(), current_board.cols());
		states[index].copyTo(req);
		std::cout << "now moving " << Move(states[index].move).toString() << std::endl;
		std::cout << "minimax " << 1.0f - states[index].minimax_value << std::endl;
		std::cout << "outcome " << outcomeToString(outcome) << std::endl;
		std::cout << policyToString(req.getBoard(), req.getPolicy());
	}

	Json Game::saveOpening() const
	{
//		SerializedObject so;
//		so.save<int>(sign_to_move);
//		so += current_board.serialize();
//		return so;
	}
	void Game::loadOpening(SerializedObject &so, bool invert_color)
	{
//		sign_to_move = so.load<int>();
//		current_board = Array2D<char>(so);
//		if (invert_color)
//		{
//			sign_to_move = -sign_to_move;
//			for (int i = 0; i < current_board.size(); i++)
//				current_board.data()[i] = -current_board.data()[i];
//		}
	}

	Json Game::serialize(SerializedObject &binary_data) const
	{
		Json result;
		result["rows"] = current_board.rows();
		result["cols"] = current_board.cols();
		result["rules"] = rulesToString(rules);
		result["outcome"] = outcomeToString(outcome);
		result["nb_of_states"] = states.size();
		result["binary_offset"] = binary_data.size();
		for (size_t i = 0; i < states.size(); i++)
			states[i].serialize(binary_data);
		return result;
	}
}

