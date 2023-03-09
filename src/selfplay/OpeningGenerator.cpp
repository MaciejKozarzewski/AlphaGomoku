/*
 * OpeningGenerator.cpp
 *
 *  Created on: Jan 20, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/alpha_beta/ThreatSpaceSearch.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	using namespace ag;
	void fill_board_with_moves(matrix<Sign> &board, const std::vector<Move> &moves)
	{
		board.clear();
		for (size_t i = 0; i < moves.size(); i++)
			Board::putMove(board, moves[i]);
	}
	Sign get_sign_to_move(const std::vector<Move> &moves) noexcept
	{
		if (moves.size() == 0)
			return Sign::CROSS;
		else
			return invertSign(moves.back().sign);
	}
	Move get_move(SearchTask &task)
	{
		if (task.wasProcessedBySolver())
		{ // mask out provably losing moves
			for (int i = 0; i < task.getPolicy().size(); i++)
				if (task.getActionScores()[i].isLoss())
					task.getPolicy()[i] = 0.0f;
		}
		return randomizeMove(task.getPolicy());
	}
	void generate_opening_map(const matrix<Sign> &board, matrix<float> &dist, float noiseWeight)
	{
		assert(equalSize(board, dist));
		for (int i = 0; i < board.size(); i++)
			dist[i] *= (1.0f - noiseWeight);
		if (Board::isEmpty(board))
		{
			for (int i = 0; i < board.rows(); i++)
				for (int j = 0; j < board.cols(); j++)
				{
					const float d = std::hypot(0.5 + i - 0.5 * board.rows(), 0.5 + j - 0.5 * board.cols()) - 1;
					dist.at(i, j) += noiseWeight * pow(1.5f, -d);
				}
			return;
		}

		for (int i = 0; i < board.size(); i++)
			if (board.data()[i] != Sign::NONE)
				dist[i] = 0.0f;
			else
				dist[i] = std::max(dist[i], 1.0e-3f);

		const float tmp = 2.0f + randFloat();
		for (int k = 0; k < board.rows(); k++)
			for (int l = 0; l < board.cols(); l++)
				if (board.at(k, l) != Sign::NONE)
				{
					for (int i = 0; i < board.rows(); i++)
						for (int j = 0; j < board.cols(); j++)
							if (board.at(i, j) == Sign::NONE)
							{
								const float d = std::hypot(i - k, j - l) - 1;
								dist.at(i, j) += noiseWeight * pow(tmp, -d);
							}
				}
	}
}

namespace ag
{

	OpeningGenerator::OpeningGenerator(const GameConfig &gameOptions, int averageLength) :
			game_config(gameOptions),
			average_length(averageLength)
	{
	}
	OpeningGenerator::Status OpeningGenerator::generate(size_t batchSize, NNEvaluator &evaluator, ThreatSpaceSearch &solver)
	{
		if (batchSize != workspace.size())
		{
			if (batchSize < workspace.size())
				workspace.erase(workspace.begin() + batchSize, workspace.end());
			else
				for (size_t i = workspace.size(); i < batchSize; i++)
				{
					workspace.push_back(OpeningUnderConstruction(game_config));
					workspace.back().desired_length = randInt(average_length) + randInt(average_length);
				}
		}
		for (auto iter = workspace.begin(); iter < workspace.end(); iter++)
			if (iter->is_scheduled_to_nn and not iter->task.wasProcessedByNetwork())
				return OpeningGenerator::TASKS_NOT_READY;

		matrix<Sign> board(game_config.rows, game_config.cols);

		float most_balanced = 1.0f;
		std::vector<Move> best;
		for (auto iter = workspace.begin(); iter < workspace.end(); iter++)
		{
			if (iter->is_scheduled_to_nn)
			{
				assert(iter->task.wasProcessedByNetwork());
				const float balance = std::fabs(iter->task.getValue().getExpectation() - 0.5f);
				if (balance < most_balanced)
				{
					most_balanced = balance;
					best = iter->moves;
				}
				iter->reset();
			}

			if (not iter->is_scheduled_to_nn)
			{
				for (int i = 0; i < 100; i++)
				{
					iter->moves = prepareOpening(game_config, 1);
					fill_board_with_moves(board, iter->moves);
					const Sign sign_to_move = (iter->moves.size() == 0) ? Sign::CROSS : invertSign(iter->moves.back().sign);
					iter->task.set(board, sign_to_move);

					solver.solve(iter->task, TssMode::RECURSIVE, 1000);
					if (not iter->task.getScore().isProven())
					{
						evaluator.addToQueue(iter->task);
						iter->is_scheduled_to_nn = true;
						break;
					}
				}
			}
		}
		if (most_balanced != 1.0f)
			completed_openings.push_back(best);

		return OpeningGenerator::OK;
	}
	std::vector<Move> OpeningGenerator::pop()
	{
		if (this->isEmpty())
			throw std::logic_error("OpeningGenerator::pop() : there are no completed openings to return");
		std::vector<Move> result = completed_openings.back();
		completed_openings.pop_back();
		return result;
	}
	bool OpeningGenerator::isEmpty() const noexcept
	{
		return completed_openings.empty();
	}

} /* namespace ag */

