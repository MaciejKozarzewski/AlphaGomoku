/*
 * OpeningGenerator.hpp
 *
 *  Created on: Jan 20, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_OPENINGGENERATOR_HPP_
#define ALPHAGOMOKU_SELFPLAY_OPENINGGENERATOR_HPP_

#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>

namespace ag
{
	class NNEvaluator;
	class ThreatSpaceSearch;
}

namespace ag
{

	class OpeningGenerator
	{
			struct OpeningUnderConstruction
			{
					std::vector<Move> moves;
					SearchTask task;
					int desired_length = 0;
					bool is_scheduled_to_nn = false;
					OpeningUnderConstruction(GameConfig config) :
							task(config)
					{
					}
					void reset()
					{
						moves.clear();
						is_scheduled_to_nn = false;
						desired_length = 0;
					}
			};

			GameConfig game_config;

			std::vector<std::vector<Move>> completed_openings;

			std::vector<OpeningUnderConstruction> workspace;
			int average_length;
			int trials = 0;
		public:
			enum Status
			{
				OK,
				TASKS_NOT_READY
			};

			OpeningGenerator(const GameConfig &gameOptions, int averageLength);
			/*
			 * \brief Performs single step of opening generation.
			 * Reads completed requests from NNEvaluator, processes them and schedules more if necessary.
			 */
			Status generate(size_t batchSize, NNEvaluator &evaluator, ThreatSpaceSearch &solver);
			/*
			 * \brief Returns one generated opening.
			 */
			std::vector<Move> pop();
			/*
			 * \brief Returns true if there are no completed openings.
			 */
			bool isEmpty() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_OPENINGGENERATOR_HPP_ */
