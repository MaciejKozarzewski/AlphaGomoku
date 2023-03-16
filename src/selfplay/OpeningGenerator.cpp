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

		for (auto iter = workspace.begin(); iter < workspace.end(); iter++)
		{
			if (iter->is_scheduled_to_nn)
			{
				assert(iter->task.wasProcessedByNetwork());
				const float balance = std::fabs(iter->task.getValue().getExpectation() - 0.5f);
//				std::cout << "evaluation = " << iter->task.getValue().toString() << " balance = " << balance << '\n';
				if (balance < (0.05f + 0.01f * trials))
				{
					completed_openings.push_back(iter->moves);
					trials = 0;
				}
				else
					trials++;
				iter->reset();
			}

			if (not iter->is_scheduled_to_nn)
			{
				for (int i = 0; i < 100; i++)
				{
					iter->moves = prepareOpening(game_config, 1);
					Board::fromListOfMoves(board, iter->moves);
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

