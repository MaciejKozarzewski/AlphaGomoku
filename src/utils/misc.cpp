/*
 * misc.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/game/Board.hpp>

#include <cinttypes>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

namespace ag
{
	std::string currentDateTime()
	{
		time_t now = time(0);
		struct tm tstruct;
		char buf[80];
		tstruct = *localtime(&now);
		// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
		// for more information about date/time format
		strftime(buf, sizeof(buf), "%Y_%m_%d_%H%M%S", &tstruct);
		return buf;
	}

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy)
	{
		assert(board.rows() == policy.rows());
		assert(board.cols() == policy.cols());
		int count = 0;
		float sum = 0.0f;
		for (int i = 0; i < board.size(); i++)
			if (board[i] == Sign::NONE)
			{
				count++;
				sum += policy[i];
			}
			else
				policy[i] = 0.0f;

		if (sum == 0.0f)
		{
			sum = 1.0f / count;
			for (int i = 0; i < board.size(); i++)
				if (board[i] == Sign::NONE)
					policy[i] = sum;
		}
		else
		{
			sum = 1.0f / sum;
			for (int i = 0; i < policy.size(); i++)
				policy[i] *= sum;
		}
	}

	std::vector<float> averageStats(std::vector<float> &stats)
	{
		std::vector<float> result(stats.size() - 1, 0.0f);
		if (stats[0] != 0.0f)
			for (size_t i = 0; i < result.size(); i++)
				result[i] = stats[1 + i] / stats[0];
		return result;
	}

	Move pickMove(const matrix<float> &policy)
	{
		auto idx = std::distance(policy.begin(), std::max_element(policy.begin(), policy.end()));
		return Move(idx / policy.rows(), idx % policy.rows());
	}
	Move randomizeMove(const matrix<float> &policy)
	{
		float r = std::accumulate(policy.begin(), policy.end(), 0.0f);
		if (r == 0.0f) // special case when policy is completely zero, assumes empty board
		{
			const int k = randInt(policy.size());
			return Move(k / policy.rows(), k % policy.rows());
		}
		r *= randFloat();
		float sum = 0.0f;
		int i = 0;
		for (; i < policy.size(); i++)
		{
			sum += policy[i];
			if (r < sum)
				break;
		}
		return Move(i / policy.rows(), i % policy.cols());
	}
	float max(const matrix<float> &policy)
	{
		return *std::max(policy.begin(), policy.end());
	}

	void generateOpeningMap(const matrix<Sign> &board, matrix<float> &dist)
	{
		assert(equalSize(board, dist));
		if (Board::isEmpty(board))
		{
			for (int i = 0; i < board.rows(); i++)
				for (int j = 0; j < board.cols(); j++)
				{
					float d = std::hypot(0.5 + i - 0.5 * board.rows(), 0.5 + j - 0.5 * board.cols()) - 1;
					dist.at(i, j) += pow(1.5f, -d);
				}
			return;
		}

		for (int i = 0; i < board.size(); i++)
			if (board[i] != Sign::NONE)
				dist[i] = 0.0f;
			else
				dist[i] = 1.0e-6f;

		float tmp = 2.0f + randFloat();
		for (int k = 0; k < board.rows(); k++)
			for (int l = 0; l < board.cols(); l++)
				if (board.at(k, l) != Sign::NONE)
				{
					for (int i = 0; i < board.rows(); i++)
						for (int j = 0; j < board.cols(); j++)
							if (board.at(i, j) == Sign::NONE)
							{
								float d = std::hypot(i - k, j - l) - 1;
								dist.at(i, j) += pow(tmp, -d);
							}
				}
	}
	std::vector<Move> prepareOpening(const GameConfig &config, int minNumberOfMoves)
	{
		matrix<float> map_dist(config.rows, config.cols);
		matrix<Sign> board(config.rows, config.cols);

		while (true)
		{
			std::vector<Move> result;
			board.clear();
			Sign sign_to_move = Sign::CROSS;
			int opening_moves = std::max(minNumberOfMoves, randInt(6) + randInt(6) + randInt(6));
			if (randInt(1000) == 0)
				opening_moves = 0;
			for (int i = 0; i < opening_moves; i++)
			{
				generateOpeningMap(board, map_dist);
				Move move = randomizeMove(map_dist);
				move.sign = sign_to_move;
				result.push_back(move);
				assert(board.at(move.row, move.col) == Sign::NONE);
				board.at(move.row, move.col) = sign_to_move;
				sign_to_move = invertSign(sign_to_move);
			}
			if (result.empty())
				return result;
			if (getOutcome(config.rules, board, result.back()) == GameOutcome::UNKNOWN)
				return result;
		}
	}
	int get_simulations_for_move(float drawRate, int maxSimulations, int minSimulations) noexcept
	{
		assert(0.0f <= drawRate && drawRate <= 1.0f);
		assert(maxSimulations >= minSimulations);

		constexpr float draw_threshold = 0.75f;
		const float reduction_fraction = clip((drawRate - draw_threshold) / (1.0f - draw_threshold), 0.0f, 1.0f);
		return maxSimulations - reduction_fraction * (maxSimulations - minSimulations);
	}

	std::string zfill(int value, int length)
	{
		std::string result = std::to_string(std::abs(value));
		while (static_cast<int>(result.size()) < length)
			result = '0' + result;
		return result;
	}
	std::string sfill(int value, int length, bool isSigned)
	{
		std::string result = std::to_string(std::abs(value));
		if (isSigned)
		{
			if (value == 0)
				result = ' ' + result;
			else
				result = ((value > 0) ? '+' : '-') + result;
		}
		while (static_cast<int>(result.size()) < length)
			result = ' ' + result;
		return result;
	}
	std::string moveToString(const ag::Move &m)
	{
		// coordinates in Gomocup protocol are inverted relative to those used internally here
		return std::to_string(m.col) + "," + std::to_string(m.row);
	}
	ag::Move moveFromString(const std::string &str, ag::Sign sign)
	{
		std::vector<std::string> tmp = split(str, ',');
		if (tmp.size() != 2u)
			throw std::runtime_error("Incorrect move '" + str + "' was passed");

		// coordinates in Gomocup protocol are inverted relative to those used internally here
		int row = std::stoi(tmp[1]);
		int col = std::stoi(tmp[0]);
		if (row < 0 or row > 128 or col < 0 or col > 128)
			throw std::logic_error("Invalid move '" + str + "'");
		return ag::Move(row, col, sign);
	}
	bool startsWith(const std::string &line, const std::string &prefix)
	{
		if (prefix.length() > line.length())
			return false;
		else
		{
			if (prefix.length() == line.length())
				return line == prefix;
			else
				return line.substr(0, prefix.length()) == prefix;
		}
	}
	std::vector<std::string> split(const std::string &str, char delimiter)
	{
		if (str.empty())
			return std::vector<std::string>();
		int count = std::count(str.begin(), str.end(), delimiter) + 1;
		std::vector<std::string> result(count);
		size_t pos = 0;
		for (int i = 0; i < count; i++)
		{
			auto tmp = std::min(str.length(), str.find(delimiter, pos));
			result[i] = str.substr(pos, tmp - pos);
			pos = tmp + 1;
		}
		return result;
	}
	void toLowerCase(std::string &s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
		{	return std::tolower(c);});
	}
}
