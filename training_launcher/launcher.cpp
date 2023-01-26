/*
 * launcher.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/NNInputFeatures.hpp>
#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/ArgumentParser.hpp>
#include <alphagomoku/utils/ObjectPool.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/tss/ThreatSpaceSearch.hpp>

#include <minml/graph/Graph.hpp>
#include <minml/core/Device.hpp>
#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <iostream>
#include <numeric>
#include <string>
#include <fstream>
#include <istream>
#include <thread>
#include <filesystem>

#include <alphagomoku/utils/configs.hpp>

using namespace ag;

void benchmark_features()
{
	std::vector<matrix<Sign>> boards_15x15;
	std::vector<Sign> signs_to_move_15x15;
	boards_15x15.push_back(Board::fromString(" X X O X X X O X O X X _ O X _\n"
			" X _ _ _ O X O O X X X O _ _ X\n"
			" X O O _ O X X O X O _ X O _ O\n"
			" O X X O X X O O X O O X X _ O\n"
			" X X O X O O O X X O X O O O O\n"
			" _ X O X O X O O O X O X X X _\n"
			" _ X _ X _ X X O O O O X X _ X\n"
			" O O X O O _ X O X _ O X _ O O\n"
			" X _ X O X O O O O X X X _ O X\n"
			" O O O X O X X X X O O O O X X\n"
			" O X O O O O X O O X X O O X _\n"
			" X X O X X X X O _ O X X X O O\n"
			" _ X O _ X _ O X _ _ X O _ _ X\n"
			" _ O X O _ X O O X _ X X O O _\n"
			" X _ X O _ _ O X _ O O X O _ O\n"));
	signs_to_move_15x15.push_back(Sign::CROSS);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_15x15.push_back(Sign::CIRCLE);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n" // 0
					" _ _ _ _ _ O X X X X O X _ _ X\n"// 1
					" _ _ _ _ _ _ O _ O X _ O _ O _\n"// 2
					" _ _ _ _ _ _ _ X O _ X O O _ _\n"// 3
					" _ _ _ _ _ _ X O X O O O X X X\n"// 4
					" _ _ _ _ _ _ _ O _ X O _ _ O _\n"// 5
					" _ _ _ _ O _ X _ O X O X X _ _\n"// 6
					" _ _ _ _ _ X _ _ _ _ _ X O _ _\n"// 7
					" _ _ _ X X _ _ _ X O X O _ _ X\n"// 8
					" _ _ _ X _ O X X O O _ X O O _\n"// 9
					" _ _ O _ _ _ O O O X O X O _ _\n"// 10
					" _ _ _ _ _ _ X _ O _ X O X _ _\n"// 11
					" _ _ _ _ _ _ _ _ _ O O X X _ _\n"// 12
					" _ _ _ _ _ _ _ _ _ X O O O O X\n"// 13
					" _ _ _ _ _ _ _ _ _ _ _ X _ X _\n"));// 14
	signs_to_move_15x15.push_back(Sign::CROSS);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n" // 0
					" _ _ _ _ _ O X X X X O _ _ _ _\n"// 1
					" _ _ _ _ _ _ O _ O X _ O _ _ _\n"// 2
					" _ _ _ _ _ _ _ X O _ X _ O _ _\n"// 3
					" _ _ _ _ _ _ X O X O O O X X _\n"// 4
					" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n"// 5
					" _ _ _ _ _ _ _ _ O X O X _ _ _\n"// 6
					" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n"// 7
					" _ _ _ _ _ _ _ _ X O X _ _ _ _\n"// 8
					" _ _ _ _ _ _ X X O O _ X O _ _\n"// 9
					" _ _ _ _ _ _ O O _ X O X O _ _\n"// 10
					" _ _ _ _ _ _ X _ O _ X O X _ _\n"// 11
					" _ _ _ _ _ _ _ _ _ O O X X _ _\n"// 12
					" _ _ _ _ _ _ _ _ _ X O _ _ O _\n"// 13
					" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n"));// 14
	signs_to_move_15x15.push_back(Sign::CIRCLE);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n" // 0
					" _ _ _ _ _ _ _ _ O _ X _ _ _ _\n"// 1
					" _ _ _ _ _ _ _ _ _ X O O O _ _\n"// 2
					" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n"// 3
					" _ _ _ _ _ _ X _ O O O X O _ _\n"// 4
					" _ _ _ _ _ _ _ _ X O X X X X O\n"// 5
					" _ _ _ _ _ _ _ _ X O X _ _ _ _\n"// 6
					" _ _ _ _ _ _ _ _ _ _ X _ X _ _\n"// 7
					" _ _ _ _ _ _ _ _ _ X O O _ _ _\n"// 8
					" _ _ _ _ _ _ _ _ O X O _ _ _ _\n"// 9
					" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n"// 10
					" _ _ _ _ _ _ _ _ O _ O _ _ _ _\n"// 11
					" _ _ _ _ _ _ _ X _ _ _ X _ _ _\n"// 12
					" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"// 13
					" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));// 14
	signs_to_move_15x15.push_back(Sign::CIRCLE);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ O _ O O _ O _ _ _ _\n"
			" _ _ _ _ _ _ _ X O X _ _ _ _ _\n"
			" _ _ _ _ _ _ O X X O X _ _ _ _\n"
			" _ _ _ _ O X X X X O O X O _ _\n"
			" _ _ _ O X O _ X O X X X X O _\n"
			" X O X X X O X O X O O O X O _\n"
			" _ _ O O O X _ O X O X O O X _\n"
			" _ X O O O X O _ _ O X X X X O\n"
			" _ _ X _ _ O _ X O X O O X X _\n"
			" _ _ _ _ X _ X O X O _ _ _ O _\n"
			" _ _ _ _ _ _ X O _ _ _ O _ X _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_15x15.push_back(Sign::CROSS);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O _ _ O _ _ _ _\n"
			" _ _ _ _ _ _ _ X O X _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X X O X _ _ _ _\n"
			" _ _ _ _ _ _ _ X _ O O _ O _ _\n"
			" _ _ _ O X O _ _ O X _ X X _ _\n"
			" _ _ X X X O X O X O O O X O _\n"
			" _ _ _ O O X _ O X _ X O _ X _\n"
			" _ _ O O O X O _ _ O X X _ X _\n"
			" _ _ X _ _ O _ X O X O O _ X _\n"
			" _ _ _ _ X _ X O X O _ _ _ O _\n"
			" _ _ _ _ _ _ X O _ _ _ O _ X _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_15x15.push_back(Sign::CROSS);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ O X O _ _ _\n"
			" _ _ _ _ _ _ _ _ X O X X O _ _\n"
			" _ _ _ _ _ _ O _ X _ O X O _ _\n"
			" _ _ _ _ O X X X O O _ X _ _ _\n"
			" _ _ _ X _ O O O X X _ X _ _ _\n"
			" _ _ _ O _ X X O X _ O O _ _ _\n"
			" _ _ _ O X O _ O O X X _ _ _ _\n"
			" _ _ O O X O X O _ X _ O _ _ _\n"
			" _ _ X X X O X X O O _ X _ _ _\n"
			" _ _ X O O O O X _ _ _ _ _ _ _\n"
			" _ O X _ _ X X O X _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_15x15.push_back(Sign::CROSS);

	boards_15x15.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X _ O _ X _ O _ _ _\n"
			" _ _ _ _ _ _ O O X _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n"));
	signs_to_move_15x15.push_back(Sign::CIRCLE);

	std::vector<matrix<Sign>> boards_20x20;
	std::vector<Sign> signs_to_move_20x20;

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O X X _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X O O _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ X _ X X O _ X X _ O _ _ _ _ _ _ _\n"
			" _ _ _ X O O O _ X O O X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ X O O X _ O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ X _ _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O _ _ _ O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X _ O _ _ _ O _ X _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CROSS);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O X X _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O O _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ O X O O O X _ O _\n"
			" _ _ _ _ _ _ _ _ _ _ O X X _ X O X X _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ O X O O X _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ X _ _ O X X X _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ O _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CIRCLE);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X O O O _ _ O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X X X O _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O X O O X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O X X O O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O O X X _ O _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ O X X X _ O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ O X _ X _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CROSS);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ O _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X _ O X O O O X O X _ _ _ _ _\n"
			" _ _ _ _ _ _ X _ _ O X X X X O _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X O X O X O O O X _ _ _ _\n"
			" _ _ _ _ _ O _ O X _ O X X X O _ O _ _ _\n"
			" _ _ _ _ _ _ X O X O _ _ X O X X _ _ _ _\n"
			" _ _ _ _ _ X O X X O X O O O O _ X _ _ _\n"
			" _ _ _ _ _ _ O _ _ X _ O X O _ O _ O _ _\n"
			" _ _ _ _ X _ X O O X X _ O X _ _ _ _ _ _\n"
			" _ _ _ _ O O X X _ X O X _ O _ X _ _ _ _\n"
			" _ _ _ _ O X O O O X O O O X O _ _ _ _ _\n"
			" _ _ _ X O X X O X O X X O O X X _ _ _ _\n"
			" _ X _ O O X X X X O _ X X X O O _ _ _ _\n"
			" _ O X X X _ O X O X O X O _ _ _ _ _ _ _\n"
			" _ _ _ X _ O _ _ O X X O _ _ _ _ _ _ _ _\n"
			" _ _ O _ O _ _ _ O X _ O O _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ O _ O X X X O X _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CROSS);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X _ _ _ X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ O _ O _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X X O X O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X X _ O _ O X O _ _ _ _ _ _ _\n"
			" _ _ _ _ _ O X O O O O X O X _ _ _ _ _ _\n"
			" _ _ _ _ X O X _ O X O O _ X O _ _ _ _ _\n"
			" _ _ _ _ _ O _ X X O X O X X X X O _ _ _\n"
			" _ _ _ X X O O _ X _ _ O X O _ X _ _ _ _\n"
			" _ _ _ _ O X X O X X X X O X O O O _ _ _\n"
			" _ _ _ O _ O O X O _ O X O _ _ O X _ _ _\n"
			" _ _ X _ _ X O X X X O X O X X _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O O O X O X O _ O _ _ _ _\n"
			" _ _ _ _ _ _ _ _ X O X _ X O X _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ O O X O _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ X _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CIRCLE);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ O _ _ _ _ O _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X X O _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O O X X X _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ X O _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ X _ O _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CROSS);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O X X X X O _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X O O O O X O _ _ _ _ _ _\n"
			" _ _ _ _ _ _ X _ _ _ X _ O X X _ _ _ _ _\n"
			" _ _ _ _ _ O _ _ _ O _ X _ O O _ X _ X _\n"
			" _ _ _ _ _ _ _ _ _ X O X X X _ O _ O _ _\n"
			" _ _ _ _ _ _ _ _ _ _ O X _ O O X O _ _ _\n"
			" _ _ _ _ _ _ _ _ _ O X O X O X O _ X _ _\n"
			" _ _ _ _ _ _ _ _ _ O X O _ O O X O _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ X O X _ O _ O _ _\n"
			" _ _ _ _ _ _ _ _ _ X X O X O X _ X _ X _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X _ O _ _ _ O _ X _ X O _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CIRCLE);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ O _ _ _ _ O O _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O X _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ X X X _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X X _ X O _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O O _ O _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CROSS);

	boards_20x20.push_back(Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O _ X _\n"
			" _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ X O _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ O _ X X O _ _ _\n"
			" _ _ _ _ _ _ _ _ _ X _ _ _ X O O O X X _\n"
			" _ _ _ _ _ _ _ _ _ _ O O X O _ X O O _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ O X X X O X _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ X O O O X _ X _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ X O _ X _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"));
	signs_to_move_20x20.push_back(Sign::CROSS);

	std::vector<GameRules> rules = { GameRules::FREESTYLE, GameRules::STANDARD };
	std::cout << "15x15 board\n";
	for (size_t r = 0; r < rules.size(); r++)
	{
//		FeatureExtractor extractor_15x15(GameConfig(rules[r], 15, 15));

		double start = getTime();
		for (int i = 0; i < 100000; i++)
		{
//			for (size_t j = 0; j < boards_15x15.size(); j++)
//				extractor_15x15.setBoard(boards_15x15[j], signs_to_move_15x15[j]);
		}
		double stop = getTime();

		std::cout << toString(rules[r]) << " : " << (stop - start) << "s\n";
	}

	std::cout << "20x20 board\n";
	for (size_t r = 0; r < rules.size(); r++)
	{
//		FeatureExtractor extractor_20x20(GameConfig(rules[r], 20, 20));

		double start = getTime();
		for (int i = 0; i < 100000; i++)
		{
//			for (size_t j = 0; j < boards_20x20.size(); j++)
//				extractor_20x20.setBoard(boards_20x20[j], signs_to_move_20x20[j]);
		}
		double stop = getTime();

		std::cout << toString(rules[r]) << " : " << (stop - start) << "s\n";
	}
}

bool is_symmetric(const matrix<Sign> &board)
{
	matrix<Sign> copy(board.rows(), board.cols());

	for (int i = 1; i < 8; i++)
	{
		augment(copy, board, i);
		if (copy == board)
			return true;
	}
	return false;
}

void check_all_dataset(const std::string &path, int counter)
{
	size_t all_positions = 0;
	size_t all_games = 0;
	size_t sym_positions = 0;

	matrix<Sign> board(12, 12);
	for (int i = 0; i < counter; i++)
	{
		GameBuffer buffer(path + "buffer_" + std::to_string(i) + ".bin");

		all_positions = 0;
		for (int j = 0; j < buffer.size(); j++)
			all_positions += buffer.getFromBuffer(j).getNumberOfSamples();
		std::cout << i << " " << all_positions << " " << buffer.size() << '\n';

//		all_games += buffer.size();
//		for (int j = 0; j < buffer.size(); j++)
//		{
//			all_positions += buffer.getFromBuffer(j).getNumberOfSamples();
//			for (int k = 0; k < buffer.getFromBuffer(j).getNumberOfSamples(); k++)
//			{
//				buffer.getFromBuffer(j).getSample(k).getBoard(board);
//				if (is_symmetric(board))
//					sym_positions++;
//			}
//		}
//		std::cout << i << " " << sym_positions << " / " << all_positions << " in " << all_games << " games\n";
	}
}

void find_proven_positions(const std::string &path, int index)
{
//	size_t all_positions = 0;
//	size_t all_games = 0;
//	size_t proven_positions = 0;
//
//	GameConfig game_config(GameRules::STANDARD, 15, 15);
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	FeatureExtractor extractor(game_config);
//	VCFSolver solver(game_config);
//
//	std::vector<std::pair<uint16_t, float>> list_of_moves;
//	matrix<float> policy(board.rows(), board.cols());
//
//	GameBuffer buffer(path + "buffer_" + std::to_string(index) + ".bin");
//
//	SearchTask task1(game_config.rules);
//	SearchTask task2(game_config.rules);
//	task1.reset(board, Sign::CROSS);
//	task2.reset(board, Sign::CROSS);
//
//	extractor.solve(task1, 2);
//	solver.solve(task2, 2);
//
//	TimedStat t_extractor("extractor");
//	TimedStat t_solver("solver   ");
//
//	std::cout << "start\n";
//	for (int i = 0; i < buffer.size(); i++)
//	{
//		all_games++;
//		all_positions += buffer.getFromBuffer(i).getNumberOfSamples();
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			buffer.getFromBuffer(i).getSample(j).getBoard(board);
//			Sign sign_to_move = buffer.getFromBuffer(i).getSample(j).getMove().sign;
//
//			task1.reset(board, sign_to_move);
//			task2.reset(board, sign_to_move);
//
//			t_extractor.startTimer();
//			extractor.solve(task1, 2);
//			t_extractor.stopTimer();
////			solver.solve(task2, 2);
////
////			if (task1.isReady() != task2.isReady())
////			{
////				std::cout << i << " " << j << '\n';
////				std::cout << task1.toString() << '\n';
////				std::cout << "----------------------------------------\n";
////				std::cout << task2.toString() << '\n';
////				return;
////			}
//			if (task1.isReady())
//				proven_positions++;
//		}
//	}
//	std::cout << proven_positions << " / " << all_positions << " in " << all_games << " games\n";
//	std::cout << t_extractor.toString() << '\n';
//
//	proven_positions = 0;
//	for (int i = 0; i < buffer.size(); i++)
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			buffer.getFromBuffer(i).getSample(j).getBoard(board);
//			Sign sign_to_move = buffer.getFromBuffer(i).getSample(j).getMove().sign;
//
//			task2.reset(board, sign_to_move);
//			t_solver.startTimer();
//			solver.solve(task2, 2);
//			t_solver.stopTimer();
//
//			if (task2.isReady())
//				proven_positions++;
//		}
//	std::cout << proven_positions << " / " << all_positions << " in " << all_games << " games\n";
//	std::cout << t_solver.toString() << '\n';
}

//void test_evaluate()
//{
//	MasterLearningConfig config(FileLoader("/home/maciek/alphagomoku/run2022_15x15s2/config.json").getJson());
//	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);
//
//	SelfplayConfig cfg(config.evaluation_config.selfplay_options);
//	cfg.simulations_min = 1000;
//	cfg.simulations_max = 1000;
//	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/run2022_15x15s2/checkpoint/network_51_opt.bin", "retrain2021");
//	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/run2022_15x15s/checkpoint/network_124_opt.bin", "train2022");
//
//	manager.generate(1000);
//	std::string to_save;
//	for (int i = 0; i < manager.numberOfThreads(); i++)
//		to_save += manager.getGameBuffer(i).generatePGN();
//	std::ofstream file("/home/maciek/alphagomoku/test_2021vs2022.pgn", std::ios::out | std::ios::app);
//	file.write(to_save.data(), to_save.size());
//	file.close();
//}

void generate_openings(int number)
{
	GameConfig game_config(GameRules::STANDARD, 15);
	DeviceConfig device_config;
	device_config.batch_size = 256;
	device_config.device = ml::Device::cuda(0);

	NNEvaluator evaluator(device_config);
	evaluator.loadGraph("/home/maciek/alphagomoku/minml_test/minml3v7_10x128_opt.bin");

	ThreatSpaceSearch solver(game_config);
	std::shared_ptr<SharedHashTable<4>> sht = std::make_shared<SharedHashTable<4>>(game_config.rows, game_config.cols, 1048576);
	solver.setSharedTable(sht);

	OpeningGenerator generator(game_config, 10);

	for (int i = 0; i < 10; i++)
	{
		generator.generate(32, evaluator, solver);
		evaluator.evaluateGraph();
	}
}

void test_expand()
{
	GameConfig game_config(GameRules::STANDARD, 15);

	TreeConfig tree_config;
	Tree tree(tree_config);

	SearchConfig search_config;
	search_config.max_batch_size = 1;
	search_config.exploration_constant = 1.25f;
	search_config.max_children = 16;
	search_config.solver_level = 1;
	search_config.solver_max_positions = 100;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
	device_config.device = ml::Device::cpu();
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
	nn_evaluator.loadGraph("/home/maciek/alphagomoku/minml_test/minml3v7_10x128_opt.bin");

	SequentialHalvingSelector selector(16, 200);

	matrix<Sign> board(15, 15);
	board = Board::fromString(""
			" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ X X O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ O _ X _ X _ _ _ _ _\n"
			" _ _ O X O _ _ O X _ _ _ _ _ _\n"
			" _ _ _ X _ O _ O _ X _ _ _ _ _\n"
			" _ _ _ _ X X _ _ _ _ O _ _ _ _\n"
			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	const Sign sign_to_move = Sign::CIRCLE;

	Search search(game_config, search_config);
	tree.setBoard(board, sign_to_move);
//	tree.setEdgeSelector(SequentialHalvingSelector(32, 1000));
//	tree.setEdgeGenerator(SequentialHalvingGenerator(search_config.max_children));
	tree.setEdgeSelector(UCTSelector(1.0f, 0.5f));
	tree.setEdgeGenerator(SolverGenerator(search_config.max_children));

	int next_step = 0;
	for (int j = 0; j <= 1001; j++)
	{
//		if (tree.getSimulationCount() >= next_step)
//		{
//			std::cout << tree.getSimulationCount() << " ..." << std::endl;
//			next_step += 10000;
//		}
		search.select(tree, 1001);
		search.solve(true);
		search.scheduleToNN(nn_evaluator);
		nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);

		if (tree.isProven() or tree.getSimulationCount() >= 1001)
			break;
	}
	search.cleanup(tree);

//	tree.printSubtree(-1, true, -1);
//	std::cout << search.getStats().toString() << '\n';
//	std::cout << "memory = " << ((tree.getMemory() + search.getMemory()) / 1048576.0) << "MB\n\n";
//	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
//	std::cout << tree.getNodeCacheStats().toString() << '\n';
//	std::cout << nn_evaluator.getStats().toString() << '\n';
//
	Node info = tree.getInfo( { });
	info.sortEdges();

	auto get_expectation = [](const Edge *edge)
	{
		switch (edge->getProvenValue())
		{
			default:
			case ProvenValue::UNKNOWN:
			return edge->getValue().getExpectation();
			case ProvenValue::LOSS:
			return 0.0f;
			case ProvenValue::DRAW:
			return 0.5f;
			case ProvenValue::WIN:
			return 10.0f;
		}
	};

	int max_N = 0.0f;
	float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;
	for (Edge *edge = info.begin(); edge < info.end(); edge++)
		if (edge->getVisits() > 0)
		{
			sum_v += get_expectation(edge) * edge->getVisits();
			sum_q += edge->getPolicyPrior() * get_expectation(edge);
			sum_p += edge->getPolicyPrior();
			max_N = std::max(max_N, edge->getVisits());
		}
	const float inv_N = 1.0f / info.getVisits();
	float V_mix = info.getValue().getExpectation() - sum_v * inv_N + (1.0f - inv_N) / sum_p * sum_q;
	std::cout << "V_mix = " << V_mix << '\n';

	std::vector<float> asdf;
	for (int i = 0; i < info.numberOfEdges(); i++)
	{
		const float Q = info.getEdge(i).getVisits() > 0 ? get_expectation(info.begin() + i) : V_mix;
		const float sigma_Q = (50 + max_N) * Q;
		const float logit = safe_log(info.getEdge(i).getPolicyPrior());
		asdf.push_back(logit + sigma_Q);
	}
	const float shift = *std::max_element(asdf.begin(), asdf.end());
	float inv_sum = 0.0f;
	for (size_t i = 0; i < asdf.size(); i++)
	{
		asdf[i] = std::exp(asdf[i] - shift);
		inv_sum += asdf[i];
	}
	inv_sum = 1.0f / inv_sum;

	std::cout << info.toString() << '\n';
	for (int i = 0; i < info.numberOfEdges(); i++)
		std::cout << asdf[i] * inv_sum << " : " << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
	std::cout << '\n';
}

void run_training()
{
	ml::Device::setNumberOfThreads(1);
	GameBuffer buffer("/home/maciek/alphagomoku/standard_test/train_buffer/buffer_0.bin");
	std::cout << buffer.getStats().toString() << '\n';

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.augment_training_data = true;
	training_config.blocks = 10;
	training_config.filters = 128;
	training_config.device_config.batch_size = 64;
	training_config.l2_regularization = 5.0e-5f;

	AGNetwork network(game_config, training_config);
//	network.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3_5x64.bin");
	network.moveTo(ml::Device::cuda(1));
//	network.moveTo(ml::Device::cpu());
//	network.forward(2);
//	network.backward(2);
//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	SupervisedLearning sl(training_config);

	for (int e = 0; e < 100; e++)
	{
		if (e == 40)
			network.changeLearningRate(1.0e-4f);
		if (e == 80)
			network.changeLearningRate(1.0e-5f);
		sl.clearStats();
		sl.train(network, buffer, 1000);
//		sl.saveTrainingHistory("/home/maciek/alphagomoku/minml_test/");
		network.saveToFile(
				"/home/maciek/alphagomoku/minml_test/minml_btl_" + std::to_string(training_config.blocks) + "x"
						+ std::to_string(training_config.filters) + ".bin");
	}
	network.optimize();
	network.saveToFile(
			"/home/maciek/alphagomoku/minml_test/minml_btl_" + std::to_string(training_config.blocks) + "x" + std::to_string(training_config.filters)
					+ "_opt.bin");
}

void test_evaluate()
{
	MasterLearningConfig config(FileLoader("/home/maciek/Desktop/solver/config.json").getJson());
	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

	SelfplayConfig cfg(config.evaluation_config.selfplay_options);
	cfg.simulations = 1000;
	cfg.search_config.solver_level = 3;
	cfg.search_config.solver_max_positions = 100;
	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/minml_test/minml3v7_10x128_opt.bin", "res");
	cfg.search_config.solver_level = 3;

	cfg.simulations = 3000;
	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/minml_test/minml_btl_10x128_opt.bin", "btl_x3");

	manager.generate(1000);
	std::string to_save;
	for (int i = 0; i < manager.numberOfThreads(); i++)
		to_save += manager.getGameBuffer(i).generatePGN();
	std::ofstream file("/home/maciek/Desktop/solver/tests/btl_x3_vs_res.pgn", std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();
}

int main(int argc, char *argv[])
{
	std::cout << "BEGIN" << std::endl;
	std::cout << ml::Device::hardwareInfo() << '\n';

	test_expand();

//	TrainingManager tm("/home/maciek/alphagomoku/standard_test_2/");
//	for (int i = 0; i < 100; i++)
//		tm.runIterationRL();
//	return 0;
//	{
//		AGNetwork network;
//		network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/checkpoint/network_20_opt.bin");
//		network.setBatchSize(1);
//		network.moveTo(ml::Device::cpu());
//
//		matrix<Sign> board(15, 15);
////		board = Board::fromString(" O _ O X X O O X O _ _ X _ _ _\n"
////				" _ X O X O X O X X X O O _ _ _\n"
////				" X O X O X O X O X O X O X _ _\n"
////				" O O X O X O X O X O X O X _ _\n"
////				" O X O X X O O X X O O O X _ _\n"
////				" X X X X O X X X O X X X O _ _\n"
////				" O O X O X O O X X O O X O O _\n"
////				" O O X O X O X O O X O O X X _\n"
////				" O X O X O X O X X O X O X O _\n"
////				" O X O X O X O X O O X O X X _\n"
////				" X O X O X O O X X X O X O O _\n"
////				" X O X O X O X O X X O O X X _\n"
////				" O _ X O O X X O X O X X X O _\n"
////				" _ _ O X O O X X O O X O O _ _\n"
////				" _ X _ _ X X O X O _ O O X _ _\n");
//		network.packInputData(0, board, Sign::CIRCLE);
//		network.forward(1);
//
//		matrix<float> policy(15, 15);
//		matrix<Value> action_values(15, 15);
//		Value value;
//
//		network.unpackOutput(0, policy, action_values, value);
//		std::cout << '\n';
//		std::cout << "Value = " << value.toString() << '\n'; //<< " (" << value_target.toString() << ")\n";
////		std::cout << "Proven value = " << toString(buffer.getFromBuffer(0).getSample(20 + i).getProvenValue()) << "\n";
//		std::cout << Board::toString(board, policy) << '\n';
//
//		board.clear();
//		std::cout << '\n' << Board::toString(board, policy) << '\n';
//		return 0;
//	}

//	run_training();
//	test_evaluate();
//	generate_openings(32);
//	{
//		AGNetwork model;
//		model.loadFromFile("/home/maciek/alphagomoku/minml_test/minml_btl_10x128_opt.bin");
//		model.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3v7_10x128_opt.bin");
//		model.setBatchSize(256);
//		model.moveTo(ml::Device::cpu());
//		ml::Device::setNumberOfThreads(1);
//		model.moveTo(ml::Device::cuda(0));
//		model.convertToHalfFloats();
//		model.forward(1);
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//		const int repeats = 400;
//		const double start = getTime();
//		for (int i = 0; i < repeats; i++)
//			model.forward(8);
//		const double stop = getTime();
//		std::cout << 8 * repeats / (stop - start) << "n/s\n";
//	}

	std::cout << "END" << std::endl;
	return 0;

	GameBuffer buffer("/home/maciek/alphagomoku/standard_test/valid_buffer/buffer_0.bin");
//	GameBuffer buffer("/home/maciek/alphagomoku/standard_test_2/valid_buffer/buffer_19.bin");
	std::cout << buffer.getStats().toString() << '\n';

	AGNetwork network(GameConfig(GameRules::STANDARD, 15, 15));
//	network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/checkpoint/network_20_opt.bin");
	network.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3_10x128.bin");
	network.optimize();
//	network.convertToHalfFloats();

//	network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/network_int8.bin");
////	return 0;
////	network.optimize();
	network.setBatchSize(10);
	network.moveTo(ml::Device::cuda(0));
//
	matrix<Sign> board(15, 15);
	board = Board::fromString(""
			" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ X X O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ O O _ X _ X _ _ _ _ _\n"
			" _ _ O X O O O O X _ _ _ _ _ _\n"
			" _ _ _ X _ O _ O X X _ _ _ _ _\n"
			" _ _ _ _ X X _ _ _ X _ _ _ _ _\n"
			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//	board.at(7, 7) = Sign::CIRCLE;
	const Sign sign_to_move = Sign::CIRCLE;

	for (int i = 0; i < 1; i++)
	{
		const SearchData &sample = buffer.getFromBuffer(i).getSample(0);
//		sample.print();
//		return 0;
//		const Sign sign_to_move = sample.getMove().sign;
//		sample.getBoard(board);

		network.packInputData(i, board, sign_to_move);
	}

	network.forward(1);

	matrix<float> policy(15);
	matrix<Value> action_values(15);
	Value value;

	for (int i = 0; i < 1; i++)
	{
		const Value value_target = convertOutcome(buffer.getFromBuffer(i).getSample(0).getOutcome(),
				buffer.getFromBuffer(i).getSample(0).getMove().sign);
//		const Value minimax = buffer.getFromBuffer(i).getSample(0).getMinimaxValue();
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		network.unpackOutput(i, policy, action_values, value);
		std::cout << '\n';
		std::cout << Board::toString(board);
		std::cout << "Network value   = " << value.toString() << " (target = " << value_target.toString() << ")\n";
//		std::cout << "minimax = " << minimax.toString() << "\n";
//		std::cout << "Proven value = " << toString(buffer.getFromBuffer(i).getSample(0).getProvenValue()) << "\n";
		std::cout << "Network policy:\n" << Board::toString(board, policy) << '\n';
		std::cout << "Network action values:\n" << Board::toString(board, action_values) << '\n';
//		buffer.getFromBuffer(i).getSample(0).getPolicy(policy);
//		std::cout << "Target:\n" << Board::toString(board, policy) << '\n';
//		std::cout << buffer.getFromBuffer(i).getSample(0).getMove().text() << '\n';
	}

//	network.collectCalibrationStats();

//	GameBuffer buffer("/home/maciek/alphagomoku/standard_test_2/valid_buffer/buffer_0.bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	for (int i = 0; i < buffer.size(); i++)
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//			buffer.getFromBuffer(i).getSample(j).print();

	std::cout << "END" << std::endl;
	return 0;

//	TrainingManager tm("/home/maciek/alphagomoku/small_20x20f/", "/home/maciek/alphagomoku/run2022_20x20f/");
//	for (int i = 0; i < 22; i++)
//		tm.runIterationSL();
//	return 0;

//	test_evaluate();
//	return 0;
//	GameConfig game_config(GameRules::STANDARD, 15, 15);
//	TrainingConfig training_config;
//	training_config.blocks = 10;
//	training_config.filters = 128;
//	training_config.augment_training_data = true;
//	training_config.device_config.device = ml::Device::cuda(1);
//	training_config.device_config.batch_size = 256;
//
//	GameBuffer buffer;
//	for (int i = 105; i < 120; i++)
//	{
//		std::cout << "Loading buffer " << i << '\n';
//		buffer.load("/home/maciek/alphagomoku/standard_15x15/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	}
//	std::cout << buffer.getStats().toString() << '\n';
//
//	AGNetwork model(game_config, training_config);
//
//	SupervisedLearning sl(training_config);
//	std::string working_dir = "/home/maciek/alphagomoku/pretrain_15x15s/";
//
//	sl.saveTrainingHistory(working_dir);
//	model.saveToFile(working_dir + "model_0.bin");
//	for (int i = 0; i < 20; i++)
//	{
//		sl.train(model, buffer, 1000);
//		sl.saveTrainingHistory(working_dir);
//		sl.clearStats();
//		model.saveToFile(working_dir + "new_model_" + std::to_string(i + 1) + ".bin");
//	}
//
//	return 0;

//	std::cout << "Compiled on " << __DATE__ << " at " << __TIME__ << std::endl;
//	std::cout << ml::Device::hardwareInfo() << '\n';

//	FeatureTable ft(GameRules::FREESTYLE);
//	return 0;

//	check_all_dataset("/home/maciek/alphagomoku/test7_12x12_standard/train_buffer/", 35);
//	return 0;

//	test_expand();
//	return 0;

//	test_evaluate();
//	generate_openings(500);

//	benchmark_features();
//	find_proven_positions("/home/maciek/alphagomoku/standard_15x15/train_buffer/", 100);
//	return 0;

	std::string path;
	ArgumentParser ap;
	ap.addArgument("path", [&](const std::string &arg)
	{	path = arg;});
	ap.parseArguments(argc, argv);

	if (path.empty())
	{
		std::cout << "Path is empty, exiting" << std::endl;
		return 0;
	}

	if (std::filesystem::exists(path + "/config.json"))
	{
		TrainingManager tm(path);
		for (int i = 0; i < 200; i++)
			tm.runIterationRL();
	}
	else
	{
		if (not std::filesystem::exists(path))
		{
			std::filesystem::create_directory(path);
			std::cout << "Created working directory" << std::endl;
		}
		MasterLearningConfig cfg;
		FileSaver fs(path + "/config.json");
		fs.save(cfg.toJson(), SerializedObject(), 2, false);
		std::cout << "Created default configuration file, exiting." << std::endl;
	}
	return 0;

	/*
	 GameConfig game_config(GameRules::STANDARD, 15);

	 TreeConfig tree_config;
	 Tree_old tree(tree_config);


	 SearchConfig search_config;
	 search_config.max_batch_size = 16;
	 search_config.exploration_constant = 1.25f;
	 search_config.noise_weight = 0.0f;
	 search_config.expansion_prior_treshold = 1.0e-4f;
	 search_config.max_children = 30;
	 search_config.vcf_solver_level = 2;

	 ml::Device::cpu().setNumberOfThreads(1);
	 EvaluationQueue queue;
	 //	queue.loadGraph("/home/maciek/alphagomoku/test_10x10_standard/checkpoint/network_65_opt.bin", 32, ml::Device::cuda(0));
	 //	queue.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64wdl_opt.bin", 32, ml::Device::cuda(0));
	 queue.loadGraph("/home/maciek/Desktop/AlphaGomoku511/networks/standard_10x128.bin", 16, ml::Device::cuda(0), false);

	 Search_old search(game_config, search_config, tree, cache, queue);

	 Sign sign_to_move = Sign::CIRCLE;
	 matrix<Sign> board(game_config.rows, game_config.cols);

	 //	board = boardFromString(" X X O X X X O X O X X _ O X _\n"
	 //							" X _ _ _ O X O O X X X O _ _ X\n"
	 //							" X O O _ O X X O X O _ X O _ O\n"
	 //							" O X X O X X O O X O O X X _ O\n"
	 //							" X X O X O O O X X O X O O O O\n"
	 //							" _ X O X O X O O O X O X X X _\n"
	 //							" _ X _ X _ X X O O O O X X _ X\n"
	 //							" O O X O O _ X O X _ O X _ O O\n"
	 //							" X _ X O X O O O O X X X _ O X\n"
	 //							" O O O X O X X X X O O O O X X\n"
	 //							" O X O O O O X O O X X O O X _\n"
	 //							" X X O X X X X O _ O X X X O O\n"
	 //							" _ X O _ X _ O X _ _ X O _ _ X\n"
	 //							" _ O X O _ X O O X _ X X O O _\n"
	 //							" X _ X O _ _ O X _ O O X O _ O\n");
	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	board = boardFromString(" _ _ X _ _ _ _ _ _ _\n"
	 //							" _ X _ O X X _ _ _ _\n"
	 //							" _ _ O _ O X _ _ _ _\n"
	 //							" _ X _ O O O X X _ _\n"
	 //							" X O _ O O X O O O _\n"
	 //							" _ _ O X X O X X O _\n"
	 //							" _ _ X O O O X X _ _\n"
	 //							" _ _ _ O _ O X _ _ _\n"
	 //							" _ _ X _ X X X X O O\n"
	 //							" _ _ _ _ _ _ O _ _ _\n");

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n" // 0
	 //							" _ _ _ _ _ O X X X X O X _ _ X\n"// 1
	 //							" _ _ _ _ _ _ O _ O X _ O _ O _\n"// 2
	 //							" _ _ _ _ _ _ _ X O _ X O O _ _\n"// 3
	 //							" _ _ _ _ _ _ X O X O O O X X X\n"// 4
	 //							" _ _ _ _ _ _ _ O _ X O _ _ O _\n"// 5
	 //							" _ _ _ _ O _ X _ O X O X X _ _\n"// 6
	 //							" _ _ _ _ _ X _ _ _ _ _ X O _ _\n"// 7
	 //							" _ _ _ X X _ _ _ X O X O _ _ X\n"// 8
	 //							" _ _ _ X _ O X X O O _ X O O _\n"// 9
	 //							" _ _ O _ _ _ O O O X O X O _ _\n"// 10
	 //							" _ _ _ _ _ _ X _ O _ X O X _ _\n"// 11
	 //							" _ _ _ _ _ _ _ _ _ O O X X _ _\n"// 12
	 //							" _ _ _ _ _ _ _ _ _ X O O O O X\n"// 13
	 //							" _ _ _ _ _ _ _ _ _ _ _ X _ X _\n");// 14
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n" // 0
	 //							" _ _ _ _ _ O X X X X O _ _ _ _\n" // 1
	 //							" _ _ _ _ _ _ O _ O X _ O _ _ _\n" // 2
	 //							" _ _ _ _ _ _ _ X O _ X _ O _ _\n" // 3
	 //							" _ _ _ _ _ _ X O X O O O X X _\n" // 4
	 //							" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n" // 5
	 //							" _ _ _ _ _ _ _ _ O X O X _ _ _\n" // 6
	 //							" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" // 7
	 //							" _ _ _ _ _ _ _ _ X O X _ _ _ _\n" // 8
	 //							" _ _ _ _ _ _ X X O O _ X O _ _\n" // 9
	 //							" _ _ _ _ _ _ O O _ X O X O _ _\n" // 10
	 //							" _ _ _ _ _ _ X _ O _ X O X _ _\n" // 11
	 //							" _ _ _ _ _ _ _ _ _ O O X X _ _\n" // 12
	 //							" _ _ _ _ _ _ _ _ _ X O _ _ O _\n" // 13
	 //							" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n"); // 14

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n" // 0
	 //							" _ _ _ _ _ _ _ _ O _ X _ _ _ _\n"// 1
	 //							" _ _ _ _ _ _ _ _ _ X O O O _ _\n"// 2
	 //							" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n"// 3
	 //							" _ _ _ _ _ _ X _ O O O X O _ _\n"// 4
	 //							" _ _ _ _ _ _ _ _ X O X X X X O\n"// 5
	 //							" _ _ _ _ _ _ _ _ X O X _ _ _ _\n"// 6
	 //							" _ _ _ _ _ _ _ _ _ _ X _ X _ _\n"// 7
	 //							" _ _ _ _ _ _ _ _ _ X O O _ _ _\n"// 8
	 //							" _ _ _ _ _ _ _ _ O X O _ _ _ _\n"// 9
	 //							" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n"// 10
	 //							" _ _ _ _ _ _ _ _ O _ O _ _ _ _\n"// 11
	 //							" _ _ _ _ _ _ _ X _ _ _ X _ _ _\n"// 12
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"// 13
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");// 14
	 //	sign_to_move = Sign::CIRCLE;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ X _ O _ X _ O _ _ _\n"
	 //							" _ _ _ _ _ _ O O X _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n");
	 //	sign_to_move = Sign::CIRCLE;

	 //	board = boardFromString(" _ X _ O _ _ _ _ _ X O X _ O _\n"
	 //			" X X _ O O O X O X O O O X X _\n"
	 //			" _ O O X X O X _ X _ O _ X X _\n"
	 //			" _ O _ X O O O O X X X X O O O\n"
	 //			" X O X X X X O X O O O O X X O\n"
	 //			" O X O O X X O _ X O _ O _ _ X\n"
	 //			" O X X X X O O O O X X X X O O\n"
	 //			" X X X O _ X _ X O X O O X O O\n"
	 //			" X O O _ O X O O O X X O O O O\n"
	 //			" X O X O O X X X X O X O O X O\n"
	 //			" O O X X X O X O O X X O O X X\n"
	 //			" X O X _ X O X X X O X X X X O\n"
	 //			" _ X O X O O _ X O _ O X O O X\n"
	 //			" O X O O _ X O X _ X O X O X _\n"
	 //			" X _ O X X X X O _ X O O O O X\n");
	 //	board.at(4, 5) = Sign::CROSS;
	 //	board.at(6, 5) = Sign::CIRCLE;
	 //	board.at(1, 2) = Sign::CROSS;
	 //	board.at(6, 6) = Sign::CIRCLE;
	 //	board.at(3, 3) = Sign::CROSS;
	 //	board.at(4, 6) = Sign::CIRCLE;
	 //	board.at(3, 8) = Sign::CROSS;
	 //	board.at(7, 6) = Sign::CIRCLE;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ O X X _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X O O _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ X _ X X O _ X X _ O _ _ _ _ _ _ _\n"
	 //							" _ _ _ X O O O _ X O O X _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ X O O X _ O _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ X _ _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ O _ _ _ O _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ X _ O _ _ _ O _ X _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O X X _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ O X O O O X _ O _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ O X X _ X O X X _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ O X O O X _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ X _ _ O X X X _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CIRCLE;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ X O O O _ _ O _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ X X X O _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ O X O O X _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ O X X O O _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ O O X X _ O _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ O X X X _ O _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ O X _ X _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" O O O O O O O _ _ _ _ _ _ _ _\n"
	 //			" X _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
	 //			" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" X _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
	 //			" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ O _ _ X _ _ _ _\n"
	 " _ _ _ _ _ _ _ X X X O _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ O O O X _ _ _\n"
	 " _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ O X _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 sign_to_move = Sign::CIRCLE;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ O _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ X _ O X O O O X O X _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ X _ _ O X X X X O _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X O X O X O O O X _ _ _ _\n"
	 //							" _ _ _ _ _ O _ O X _ O X X X O _ O _ _ _\n"
	 //							" _ _ _ _ _ _ X O X O _ _ X O X X _ _ _ _\n"
	 //							" _ _ _ _ _ X O X X O X O O O O _ X _ _ _\n"
	 //							" _ _ _ _ _ _ O _ _ X _ O X O _ O _ O _ _\n"
	 //							" _ _ _ _ X _ X O O X X _ O X _ _ _ _ _ _\n"
	 //							" _ _ _ _ O O X X _ X O X _ O _ X _ _ _ _\n"
	 //							" _ _ _ _ O X O O O X O O O X O _ _ _ _ _\n"
	 //							" _ _ _ X O X X O X O X X O O X X _ _ _ _\n"
	 //							" _ X _ O O X X X X O _ X X X O O _ _ _ _\n"
	 //							" _ O X X X _ O X O X O X O _ _ _ _ _ _ _\n"
	 //							" _ _ _ X _ O _ _ O X X O _ _ _ _ _ _ _ _\n"
	 //							" _ _ O _ O _ _ _ O X _ O O _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ O _ O X X X O X _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X _ _ _ X _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ O _ O _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X X O X O _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ X X _ O _ O X O _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ O X O O O O X O X _ _ _ _ _ _\n"
	 //							" _ _ _ _ X O X _ O X O O _ X O _ _ _ _ _\n"
	 //							" _ _ _ _ _ O _ X X O X O X X X X O _ _ _\n"
	 //							" _ _ _ X X O O _ X _ _ O X O _ X _ _ _ _\n"
	 //							" _ _ _ _ O X X O X X X X O X O O O _ _ _\n"
	 //							" _ _ _ O _ O O X O _ O X O _ _ O X _ _ _\n"
	 //							" _ _ X _ _ X O X X X O X O X X _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ O O O X O X O _ O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ X O X _ X O X _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O O X O _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ X _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CIRCLE;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O _ _ _ _ O _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X X O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O O X X X _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ X O _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ X _ O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ O X X X X O _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X O O O O X O _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ X _ _ _ X _ O X X _ _ _ _ _\n"
	 //							" _ _ _ _ _ O _ _ _ O _ X _ O O _ X _ X _\n"
	 //							" _ _ _ _ _ _ _ _ _ X O X X X _ O _ O _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ O X _ O O X O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O X O X O X O _ X _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O X O _ O O X O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ X O X _ O _ O _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ X X O X O X _ X _ X _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ X _ O _ _ _ O _ X _ X O _ _ _\n");
	 //	sign_to_move = Sign::CIRCLE;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O _ _ _ _ O O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O X _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ X X X _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X X _ X O _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O O _ O _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ O _ O O _ O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ X O X _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ O X X O X _ _ _ _\n"
	 //							" _ _ _ _ O X X X X O O X O _ _\n"
	 //							" _ _ _ O X O _ X O X X X X O _\n"
	 //							" X O X X X O X O X O O O X O _\n"
	 //							" _ _ O O O X _ O X O X O O X _\n"
	 //							" _ X O O O X O _ _ O X X X X O\n"
	 //							" _ _ X _ _ O _ X O X O O X X _\n"
	 //							" _ _ _ _ X _ X O X O _ _ _ O _\n"
	 //							" _ _ _ _ _ _ X O _ _ _ O _ X _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ O _ _ O _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ X O X _ _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ X X O X _ _ _ _\n"
	 //			" _ _ _ _ _ _ _ X _ O O _ O _ _\n"
	 //			" _ _ _ O X O _ _ O X _ X X _ _\n"
	 //			" _ _ X X X O X O X O O O X O _\n"
	 //			" _ _ _ O O X _ O X _ X O _ X _\n"
	 //			" _ _ O O O X O _ _ O X X _ X _\n"
	 //			" _ _ X _ _ O _ X O X O O _ X _\n"
	 //			" _ _ _ _ X _ X O X O _ _ _ O _\n"
	 //			" _ _ _ _ _ _ X O _ _ _ O _ X _\n"
	 //			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O _ X _\n"
	 //							" _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ X O _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ O _ X X O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ X _ _ _ X O O O X X _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ O O X O _ X O O _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ O X X X O X _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ X O O O X _ X _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ X O _ X _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O X O _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ X O X X O _ _\n"
	 //							" _ _ _ _ _ _ O _ X _ O X O _ _\n"
	 //							" _ _ _ _ O X X X O O _ X _ _ _\n"
	 //							" _ _ _ X _ O O O X X _ X _ _ _\n"
	 //							" _ _ _ O _ X X O X _ O O _ _ _\n"
	 //							" _ _ _ O X O _ O O X X _ _ _ _\n"
	 //							" _ _ O O X O X O _ X _ O _ _ _\n"
	 //							" _ _ X X X O X X O O _ X _ _ _\n"
	 //							" _ _ X O O O O X _ _ _ _ _ _ _\n"
	 //							" _ O X _ _ X X O X _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 //	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ O _ O _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ O _ X _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ X _ X _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ X O X O X _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ X _ O O X _ X _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ O _ O X _ X X O O _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ O _ X _ O _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
	 //							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	 //	sign_to_move = Sign::CROSS;

	 FeatureExtractor extractor(game_config);
	 extractor.setBoard(board, sign_to_move);
	 extractor.printAllThreats();

	 //	std::vector<std::pair<uint16_t, float>> list_of_moves;
	 //	matrix<float> qwer(board.rows(), board.cols());
	 //	double t0 = getTime();
	 //	for (int i = 0; i < 1000; i++)
	 //	{
	 //		ProvenValue asdf = extractor.solve(qwer, list_of_moves);
	 //	}
	 //	std::cout << "time = " << (getTime() - t0) << "ms\n";
	 //	std::cout << toString(asdf) << '\n';
	 //	return 0;

	 double start = getTime();
	 tree.getRootNode().setMove( { 0, 0, invertSign(sign_to_move) });
	 search.setBoard(board);

	 matrix<float> policy(board.rows(), board.cols());
	 for (int i = 0; i <= 10; i++)
	 {
	 while (search.getSimulationCount() < i * 10000)
	 {
	 search.simulate(i * 10000);
	 queue.evaluateGraph();
	 search.handleEvaluation();
	 if (tree.isProven())
	 break;
	 }
	 tree.getPlayoutDistribution(tree.getRootNode(), policy);
	 normalize(policy);

	 std::cout << queue.getStats().toString();
	 std::cout << search.getStats().toString();
	 std::cout << tree.getStats().toString();
	 std::cout << cache.getStats().toString() << '\n';
	 std::cout << "Memory : cache = " << cache.getMemory() / 1048576 << "MB, tree = " << tree.getMemory() / 1048576 << "MB\n";
	 std::cout << '\n' << tree.getPrincipalVariation().toString() << '\n';
	 std::cout << policyToString(board, policy);
	 std::cout << tree.getRootNode().toString() << '\n';

	 if (tree.isProven())
	 break;
	 }
	 std::cout << "Policy priors\n";
	 tree.getPolicyPriors(tree.getRootNode(), policy);
	 normalize(policy);
	 std::cout << policyToString(board, policy);
	 std::cout << "-----------------------------------------------------------------------------------------\n";

	 matrix<ProvenValue> proven_values(board.rows(), board.cols());
	 tree.getProvenValues(tree.getRootNode(), proven_values);
	 tree.getPlayoutDistribution(tree.getRootNode(), policy);
	 normalize(policy);
	 std::cout << "Final playout distribution\n";
	 for (int i = 0; i < board.rows(); i++)
	 {
	 for (int j = 0; j < board.cols(); j++)
	 {
	 if (board.at(i, j) == Sign::NONE)
	 {
	 switch (proven_values.at(i, j))
	 {
	 case ProvenValue::UNKNOWN:
	 {
	 int t = (int) (1000 * policy.at(i, j));
	 if (t == 0)
	 std::cout << "  _ ";
	 else
	 {
	 std::cout << ' ';
	 if (t < 100)
	 std::cout << ' ';
	 if (t < 10)
	 std::cout << ' ';
	 std::cout << std::to_string(t);
	 }
	 break;
	 }
	 case ProvenValue::LOSS:
	 std::cout << " >L<";
	 break;
	 case ProvenValue::DRAW:
	 std::cout << " >D<";
	 break;
	 case ProvenValue::WIN:
	 std::cout << " >W<";
	 break;
	 }
	 }
	 else
	 std::cout << ((board.at(i, j) == Sign::CROSS) ? "  X " : "  O ");
	 }
	 std::cout << '\n';
	 }
	 std::cout << "\n----------------------------------------------------------------------------------\n";
	 tree.printSubtree(tree.getRootNode(), 1, true);
	 std::cout << "\n----------------------------------------------------------------------------------\n";

	 search.printSolverStats();

	 std::cout << "total time = " << (getTime() - start) << "s\n";

	 //	return 0;
	 //	matrix<ProvenValue> proven_values(15, 15);
	 //	matrix<Value> action_values(15, 15);
	 //	tree.getPlayoutDistribution(tree.getRootNode(), policy);
	 //	tree.getProvenValues(tree.getRootNode(), proven_values);
	 //	tree.getActionValues(tree.getRootNode(), action_values);
	 //	normalize(policy);
	 //
	 //	Move move = pickMove(policy);
	 //	move.sign = sign_to_move;
	 //
	 //	SearchData state(policy.rows(), policy.cols());
	 //	state.setBoard(board);
	 //	state.setActionProvenValues(proven_values);
	 //	state.setPolicy(policy);
	 //	state.setActionValues(action_values);
	 //	state.setMinimaxValue(tree.getRootNode().getValue());
	 //	state.setProvenValue(tree.getRootNode().getProvenValue());
	 //	state.setMove(move);
	 //
	 //	state.print();

	 //	std::string path = "/home/maciek/alphagomoku/";
	 //	std::string name = "standard_15x15_correct";
	 //	int number = 99;
	 //	for (int i = 0; i <= number; i++)
	 //	{
	 //		std::cout << "train buffer " << i << "/" << number << '\n';
	 //		GameBuffer buff(path + name + "/train_buffer/buffer_" + std::to_string(i) + ".bin");
	 //		buff.save("/home/maciek/gomoku_datasets/" + name + "/train_buffer/buffer_" + std::to_string(i) + ".zip");
	 //	}
	 //
	 //	for (int i = 0; i <= number; i++)
	 //	{
	 //		std::cout << "valid buffer " << i << "/" << number << '\n';
	 //		GameBuffer buff(path + name + "/valid_buffer/buffer_" + std::to_string(i) + ".bin");
	 //		buff.save("/home/maciek/gomoku_datasets/" + name + "/valid_buffer/buffer_" + std::to_string(i) + ".zip");
	 //	}

	 //	GameBuffer buff("/home/maciek/Desktop/test_buffer.bin");
	 //	for (int i = 0; i < buff.getFromBuffer(0).length(); i++)
	 //		buff.getFromBuffer(0).printSample(i);

	 std::cout << "END" << std::endl;
	 return 0;
	 */

}

