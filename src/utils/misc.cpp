/*
 * misc.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/misc.hpp>
#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <bits/types/struct_tm.h>
#include <bits/types/time_t.h>
#include <stddef.h>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>
#include <random>
#include <vector>

namespace
{
	uint32_t get_random_int32()
	{
		thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
		return generator();
	}
	uint64_t get_random_int64()
	{
		thread_local std::mt19937_64 generator(std::chrono::system_clock::now().time_since_epoch().count());
		return generator();
	}
}

namespace ag
{
	float randFloat()
	{
		static float tmp = 1.0f / ((1u << 31) - 1);
		return get_random_int32() * tmp;
	}
	double randDouble()
	{
		static double tmp = 1.0f / ((1ull << 63) - 1);
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

	bool isBoardFull(const matrix<Sign> &board)
	{
		for (int i = 0; i < board.size(); i++)
			if (board.data()[i] == Sign::NONE)
				return false;
		return true;
	}
	bool isBoardEmpty(const matrix<Sign> &board)
	{
		for (int i = 0; i < board.size(); i++)
			if (board.data()[i] != Sign::NONE)
				return false;
		return true;
	}

	matrix<Sign> boardFromString(const std::string &str)
	{
		int height = std::count(str.begin(), str.end(), '\n');
		int width = std::count(str.begin(), str.end(), ' ');
		matrix<Sign> result(height, width);
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				switch (str.at(i * (width + 1) + 1 + j))
				{
					case '_':
						result.at(i, j) = Sign::NONE;
						break;
					case 'X':
						result.at(i, j) = Sign::CROSS;
						break;
					case 'O':
						result.at(i, j) = Sign::CIRCLE;
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
	std::string policyToString(const matrix<Sign> &board, const matrix<float> &policy, const Move &lastMove)
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
				if (board.at(i, j) == Sign::NONE)
				{
					int t = (int) (1000 * policy.at(i, j));
					if (t == 0)
						result += " _ ";
					else
					{
						if (t < 100)
							result += " ";
						if (t < 10)
							result += " ";
						result += std::to_string(t);
					}
				}
				else
				{
					if (board.at(i, j) == Sign::CROSS)
						result += " X ";
					else
						result += " O ";
				}
			}
			if (lastMove.sign != Sign::NONE)
			{
				if (i == lastMove.row && board.cols() == lastMove.col + 1)
					result += '<';
				else
					result += ' ';
			}
			result += '\n';
		}
		return result;
	}

	int parseLine(char *line)
	{
		//TODO
		return 0;
	}
	std::string spaces(int number)
	{
		//TODO
		return "";
	}
	void print_statistics(const char *name, long number, double time)
	{
		if (number == 0)
		{
			std::cout << name << " = " << time << "s" << std::endl;
			return;
		}
		double t = time / number;
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
		std::cout << name << " = " << time << "s : " << (float) t << " " << unit << "s" << std::endl;
	}

	void maskIllegalMoves(const matrix<Sign> &board, matrix<float> &policy)
	{
		int count = 0;
		float sum = 0.0f;
		for (int i = 0; i < board.size(); i++)
			if (board.data()[i] != Sign::NONE)
				policy.data()[i] = 0.0f;
			else
			{
				count++;
				sum += policy.data()[i];
			}
		if (sum == 0.0f)
			std::fill(policy.begin(), policy.end(), 1.0f / count);
		else
		{
			sum = 1.0f / sum;
			for (int i = 0; i < board.size(); i++)
				policy.data()[i] *= sum;
		}
	}
	void addNoise(const matrix<Sign> &board, matrix<float> &policy, float noiseWeight)
	{
		if (noiseWeight == 0.0f)
			return;

		thread_local std::mt19937 generator(std::chrono::system_clock::now().time_since_epoch().count());
		const float range = 1.0f / generator.max();

		std::vector<float> noise(policy.size());
		float sum = 0.0f;
		for (size_t i = 0; i < noise.size(); i++)
			if (board.data()[i] == Sign::NONE)
			{
				noise[i] = pow(generator() * range, 4) * (1.0f - sum);
				sum += noise[i];
			}
		std::random_shuffle(noise.begin(), noise.end());
		for (size_t i = 0; i < noise.size(); i++)
			if (board.data()[i] == Sign::NONE)
				policy.data()[i] = (1.0f - noiseWeight) * policy.data()[i] + noiseWeight * noise[i];
	}

	void scaleArray(matrix<float> &array, float scale)
	{
		for (int i = 0; i < array.size(); i++)
			array.data()[i] *= scale;
	}

	float convertOutcome(Sign outcome, Sign signToMove)
	{
		if (outcome == Sign::NONE)
			return 0.5f;
		else
		{
			if (outcome == signToMove)
				return 1.0f;
			else
				return 0.0f;
		}
	}
	void averageStats(std::vector<float> &stats)
	{
		if (stats[0] == 0.0f)
			std::fill(stats.begin(), stats.end(), 0.0f);
		else
			for (size_t i = 1; i < stats.size(); i++)
				stats[i] /= stats[0];
	}

	const std::string currentDateTime()
	{
		time_t now = time(0);
		struct tm tstruct;
		char buf[80];
		tstruct = *localtime(&now);
		// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
		// for more information about date/time format
		strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
		return buf;
	}
	void logToFile(const char *file, const char *msg1, const char *msg2)
	{
		std::ofstream output;
		output.open(file, std::ios::app);
		output << currentDateTime() << " ";
		if (msg1 != NULL)
			output << msg1;
		if (msg2 != NULL)
			output << msg2;
		output << std::endl;
		output.close();
	}

	Move pickMove(const matrix<float> &policy)
	{
		auto idx = std::distance(policy.begin(), std::max_element(policy.begin(), policy.end()));
		return Move(idx / policy.rows(), idx % policy.rows());
	}
	Move randomizeMove(const matrix<float> &policy)
	{
		float r = std::accumulate(policy.begin(), policy.end(), 0.0f);

		if (r == 0.0f)		//special case when policy is completely zero, assumes empty board
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
}
