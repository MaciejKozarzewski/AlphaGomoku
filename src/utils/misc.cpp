/*
 * misc.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <inttypes.h>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

namespace
{
	uint32_t get_random_int32()
	{
#ifdef NDEBUG
		thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
#else
		thread_local std::mt19937 generator(0);
#endif
		return generator();
	}
	uint64_t get_random_int64()
	{
#ifdef NDEBUG
		thread_local std::mt19937_64 generator(std::chrono::system_clock::now().time_since_epoch().count());
#else
		thread_local std::mt19937_64 generator(0);
#endif
		return generator();
	}
}

namespace ag
{
	float randFloat()
	{
		static float tmp = 1.0f / 4294967295;
		return get_random_int32() * tmp;
	}
	double randDouble()
	{
		static double tmp = 1.0f / (4294967296ull * 4294967296ull - 1);
		return get_random_int64() * tmp;
	}
	float randGaussian()
	{
		return 0;
	}
	int32_t randInt()
	{
		return get_random_int32();
	}
	int32_t randInt(int r)
	{
		return get_random_int32() % r;
	}
	int32_t randInt(int r0, int r1)
	{
		return r0 + get_random_int32() % (r1 - r0);
	}
	uint64_t randLong()
	{
		return get_random_int64();
	}
	bool randBool()
	{
		return (get_random_int32() & 1) == 0;
	}

	double getTime()
	{
		return std::chrono::system_clock::now().time_since_epoch().count() * 1.0e-9;
	}
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

	bool isBoardFull(const matrix<Sign> &board)
	{
		return std::none_of(board.begin(), board.end(), [](Sign s)
		{	return s == Sign::NONE;});
	}
	bool isBoardEmpty(const matrix<Sign> &board)
	{
		return std::all_of(board.begin(), board.end(), [](Sign s)
		{	return s == Sign::NONE;});
	}

	matrix<Sign> boardFromString(std::string str)
	{
		int height = std::count(str.begin(), str.end(), '\n');
		auto new_end = std::remove_if(str.begin(), str.end(), [](char c)
		{	return c == ' ' or c == '\n';});
		str.erase(new_end, str.end());
		int width = static_cast<int>(str.size()) / height;
		matrix<Sign> result(height, width);
		for (int i = 0; i < height * width; i++)
			switch (str.at(i))
			{
				case '_':
					result.data()[i] = Sign::NONE;
					break;
				case 'X':
					result.data()[i] = Sign::CROSS;
					break;
				case 'O':
					result.data()[i] = Sign::CIRCLE;
					break;
			}
		return result;
	}
	std::string boardToString(const matrix<Sign> &board, const Move &lastMove)
	{
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
			{
				if (lastMove.sign != Sign::NONE)
				{
					if (i == lastMove.row && j == lastMove.col)
						result += '>';
					else
					{
						if (i == lastMove.row && j == lastMove.col + 1)
							result += '<';
						else
							result += ' ';
					}
				}
				switch (board.at(i, j))
				{
					case Sign::NONE:
						result += " _";
						break;
					case Sign::CROSS:
						result += " X";
						break;
					case Sign::CIRCLE:
						result += " O";
						break;
				}
			}
			if (lastMove.sign != Sign::NONE)
			{
				if (i == lastMove.row && board.cols() - 1 == lastMove.col)
					result += '<';
				else
					result += ' ';
			}
			result += '\n';
		}
		return result;
	}
	std::string provenValuesToString(const matrix<Sign> &board, const matrix<ProvenValue> &pv)
	{
		assert(board.rows() == pv.rows());
		assert(board.cols() == pv.cols());
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
			{
				if (board.at(i, j) == Sign::NONE)
				{
					switch (pv.at(i, j))
					{
						case ProvenValue::UNKNOWN:
							result += " _ ";
							break;
						case ProvenValue::LOSS:
							result += ">L<";
							break;
						case ProvenValue::DRAW:
							result += ">D<";
							break;
						case ProvenValue::WIN:
							result += ">W<";
							break;
					}
				}
				else
					result += (board.at(i, j) == Sign::CROSS) ? " X " : " O ";
			}
			result += '\n';
		}
		return result;
	}
	std::string policyToString(const matrix<Sign> &board, const matrix<float> &policy)
	{
		assert(board.rows() == policy.rows());
		assert(board.cols() == policy.cols());
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
			{
				if (board.at(i, j) == Sign::NONE)
				{
					int t = (int) (1000 * policy.at(i, j));
					if (t == 0)
						result += "  _ ";
					else
					{
						if (t < 1000)
							result += ' ';
						if (t < 100)
							result += ' ';
						if (t < 10)
							result += ' ';
						result += std::to_string(t);
					}
				}
				else
					result += (board.at(i, j) == Sign::CROSS) ? "  X " : "  O ";
			}
			result += '\n';
		}
		return result;
	}
	std::string actionValuesToString(const matrix<Sign> &board, const matrix<Value> &actionValues)
	{
		assert(board.rows() == actionValues.rows());
		assert(board.cols() == actionValues.cols());
		std::string result;
		for (int i = 0; i < board.rows(); i++)
		{
			for (int j = 0; j < board.cols(); j++)
			{
				if (board.at(i, j) == Sign::NONE)
				{
					if (actionValues.at(i, j) == Value())
						result += "  _ ";
					else
					{
						int t = std::min(999, (int) (1000 * (actionValues.at(i, j).win + 0.5f * actionValues.at(i, j).draw)));
						result += ' ';
						if (t < 100)
							result += ' ';
						if (t < 10)
							result += ' ';
						result += std::to_string(t);
					}
				}
				else
					result += (board.at(i, j) == Sign::CROSS) ? "  X " : "  O ";
			}
			result += '\n';
		}
		return result;
	}

	std::string printStatistics(const char *name, uint64_t number, double time)
	{
		std::string result(name);
		double t = (number == 0) ? 0.0 : time / number;
		char unit = ' ';
		if (t < 1.0e-3)
		{
			t *= 1.0e6;
			unit = 'u';
		}
		else
		{
			if (t < 1.0)
			{
				t *= 1.0e3;
				unit = 'm';
			}
		}
		return result + " = " + std::to_string(time) + "s : " + std::to_string(number) + " : " + std::to_string(t) + ' ' + unit + "s\n";
	}

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy)
	{
		assert(board.rows() == policy.rows());
		assert(board.cols() == policy.cols());
		int count = 0;
		float sum = 0.0f;
		for (int i = 0; i < board.size(); i++)
			if (board.data()[i] == Sign::NONE)
			{
				count++;
				sum += policy.data()[i];
			}
			else
				policy.data()[i] = 0.0f;

		if (sum == 0.0f)
		{
			sum = 1.0f / count;
			for (int i = 0; i < board.size(); i++)
				if (board.data()[i] == Sign::NONE)
					policy.data()[i] = sum;
		}
		else
		{
			sum = 1.0f / sum;
			for (int i = 0; i < policy.size(); i++)
				policy.data()[i] *= sum;
		}
	}
	void addNoise(const matrix<Sign> &board, matrix<float> &policy, float noiseWeight)
	{
		assert(board.rows() == policy.rows());
		assert(board.cols() == policy.cols());
		if (noiseWeight == 0.0f)
			return;

		thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
		const float range = 1.0f / generator.max();

		std::vector<float> noise;
		noise.reserve(policy.size());
		float sum = 0.0f;
		for (int i = 0; i < policy.size(); i++)
			if (board.data()[i] == Sign::NONE)
			{
				noise.push_back(pow(generator() * range, 4) * (1.0f - sum));
				sum += noise.back();
			}
		std::random_shuffle(noise.begin(), noise.end());
		for (int i = 0, k = 0; i < board.size(); i++)
			if (board.data()[i] == Sign::NONE)
			{
				policy.data()[i] = (1.0f - noiseWeight) * policy.data()[i] + noiseWeight * noise[k];
				k++;
			}
	}

	void scaleArray(matrix<float> &array, float scale)
	{
		for (int i = 0; i < array.size(); i++)
			array.data()[i] *= scale;
	}

	void averageStats(std::vector<float> &stats)
	{
		if (stats[0] == 0.0f)
			std::fill(stats.begin(), stats.end(), 0.0f);
		else
			for (size_t i = 1; i < stats.size(); i++)
				stats[i] /= stats[0];
	}

	Move pickMove(const matrix<float> &policy)
	{
		auto idx = std::distance(policy.begin(), std::max_element(policy.begin(), policy.end()));
		return Move(idx / policy.rows(), idx % policy.rows());
	}
	Move randomizeMove(const matrix<float> &policy, float temperature)
	{
		float r = std::accumulate(policy.begin(), policy.end(), 0.0f);
		if (r == 0.0f) // special case when policy is completely zero, assumes empty board
		{
			int k = randInt(policy.size());
			return Move(k / policy.rows(), k % policy.rows());
		}
		r *= randFloat();
		float sum = 0.0f;
		int i = 0;
		for (; i < policy.size(); i++)
		{
			sum += policy.data()[i];
			if (r < sum)
				break;
		}
		return Move(i / policy.rows(), i % policy.cols());
	}
	void normalize(matrix<float> &policy)
	{
		float r = std::accumulate(policy.begin(), policy.end(), 0.0f);
		if (r == 0.0f)
			std::fill(policy.begin(), policy.end(), 1.0f / policy.size());
		else
		{
			r = 1.0f / r;
			for (int i = 0; i < policy.size(); i++)
				policy.data()[i] *= r;
		}
	}
	float max(const matrix<float> &policy)
	{
		return *std::max(policy.begin(), policy.end());
	}

	std::vector<int> permutation(int length)
	{
		std::vector<int> result(length);
		std::iota(result.begin(), result.end(), 0);
		std::random_shuffle(result.begin(), result.end());
		return result;
	}

	void generateOpeningMap(const matrix<Sign> &board, matrix<float> &dist)
	{
		assert(board.rows() == dist.rows());
		assert(board.cols() == dist.cols());
		if (isBoardEmpty(board))
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
			if (board.data()[i] != Sign::NONE)
				dist.data()[i] = 0.0f;
			else
				dist.data()[i] = 1.0e-6f;

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
	std::vector<Move> prepareOpening(GameConfig config, int minNumberOfMoves)
	{
		matrix<float> map_dist(config.rows, config.cols);
		matrix<Sign> board(config.rows, config.cols);

		while (true)
		{
			std::vector<Move> result;
			board.clear();
			Sign sign_to_move = Sign::CROSS;
			int opening_moves = std::max(minNumberOfMoves, randInt(6) + randInt(6) + randInt(6));
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
			if (getOutcome(config.rules, board) == GameOutcome::UNKNOWN)
				return result;
		}
	}

	void encodeInputTensor(float *dst, const matrix<Sign> &board, Sign signToMove)
	{
		assert(signToMove != Sign::NONE);
		int idx = 0;
		for (int i = 0; i < board.rows(); i++)
			for (int j = 0; j < board.cols(); j++, idx += 4)
			{
				if (board.at(i, j) == Sign::NONE)
				{
					dst[idx] = 1.0f;
					dst[idx + 1] = 0.0f;
					dst[idx + 2] = 0.0f;
				}
				else
				{
					dst[idx] = 0.0f;
					if (board.at(i, j) == signToMove)
					{
						dst[idx + 1] = 1.0f;
						dst[idx + 2] = 0.0f;
					}
					else
					{
						dst[idx + 1] = 0.0f;
						dst[idx + 2] = 1.0f;
					}
				}
				dst[idx + 3] = 1.0f;
			}
	}

	std::string moveToString(const ag::Move &m)
	{
		// coordinates in Gomocup protocol are inverted relative to those used internally here
		return std::to_string(m.col) + "," + std::to_string(m.row);
	}
	ag::Move moveFromString(const std::string &str, ag::Sign sign)
	{
		auto tmp = split(str, ',');
		assert(tmp.size() == 2u);
		// coordinates in Gomocup protocol are inverted relative to those used internally here
		int row = std::stoi(tmp[1]);
		int col = std::stoi(tmp[0]);
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
}
