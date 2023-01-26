/*
 * OpeningGenerator.cpp
 *
 *  Created on: Jan 20, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/tss/ThreatSpaceSearch.hpp>
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
	Move get_move(const SearchTask &task)
	{
		if (task.mustDefend())
		{
			const int r = randInt(task.getPriorEdges().size());
			return task.getPriorEdges()[r].getMove();
		}
		else
			return randomizeMove(task.getPolicy());
	}
	void generate_opening_map(const matrix<Sign> &board, matrix<float> &dist)
	{
		assert(equalSize(board, dist));
		for (int i = 0; i < board.size(); i++)
			dist[i] *= 0.5f;
		if (Board::isEmpty(board))
		{
			for (int i = 0; i < board.rows(); i++)
				for (int j = 0; j < board.cols(); j++)
				{
					const float d = std::hypot(0.5 + i - 0.5 * board.rows(), 0.5 + j - 0.5 * board.cols()) - 1;
					dist.at(i, j) += 0.5f * pow(1.5f, -d);
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
								dist.at(i, j) += 0.5f * pow(tmp, -d);
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
	void OpeningGenerator::generate(size_t batchSize, NNEvaluator &evaluator, ThreatSpaceSearch &solver)
	{
		if (batchSize != workspace.size())
		{
			if (batchSize < workspace.size())
				workspace.erase(workspace.begin() + batchSize, workspace.end());
			else
				for (size_t i = workspace.size(); i < batchSize; i++)
					workspace.push_back(OpeningUnderConstruction(game_config.rules));
		}

		matrix<Sign> board(game_config.rows, game_config.cols);

		for (auto iter = workspace.begin(); iter < workspace.end(); iter++)
		{
			if (iter->task.isReadyNetwork())
			{
				assert(iter->is_scheduled_to_nn);
				generate_opening_map(iter->task.getBoard(), iter->task.getPolicy());

				const Move move(iter->task.getSignToMove(), get_move(iter->task));
				iter->moves.push_back(move);
				iter->is_scheduled_to_nn = false;
			}

			if (not iter->is_scheduled_to_nn)
			{ // this opening was not scheduled for evaluation
				if (static_cast<int>(iter->moves.size()) == iter->desired_length)
				{ // the opening is completed
					completed_openings.push_back(iter->moves);
					iter->reset();
					iter->desired_length = randInt(average_length) + randInt(average_length);
				}
				else
				{
					fill_board_with_moves(board, iter->moves);
					iter->task.set(board, get_sign_to_move(iter->moves));
					solver.solve(iter->task, TssMode::RECURSIVE, 50);
					if (iter->task.isReadySolver())
						iter->reset(); // opening cannot have a proved score
					else
					{
						evaluator.addToQueue(iter->task);
						iter->task.markAsReadyNetwork();
						iter->is_scheduled_to_nn = true;
					}
				}
			}
		}
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

