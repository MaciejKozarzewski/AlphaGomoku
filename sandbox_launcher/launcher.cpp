/*
 * launcher.cpp
 *
 *  Created on: Feb 26, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/patterns/DefensiveMoveTable.hpp>
#include <alphagomoku/patterns/ThreatTable.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/search/alpha_beta/SharedHashTable.hpp>
#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/search/monte_carlo/NodeCache.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/os_utils.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/version.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/ab_search/MoveGenerator.hpp>
#include <alphagomoku/ab_search/MinimaxSearch.hpp>
#include <alphagomoku/ab_search/StaticSolver.hpp>
#include <alphagomoku/ab_search/AlphaBetaSearch.hpp>
#include <alphagomoku/ab_search/nnue/NNUE.hpp>
#include <minml/utils/ZipWrapper.hpp>
#include <alphagomoku/search/alpha_beta/ThreatSpaceSearch.hpp>

#include "minml/core/Device.hpp"
#include "minml/utils/json.hpp"
#include "minml/utils/serialization.hpp"

#include <cstddef>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <numeric>
#include <memory>
#include <functional>
#include <x86intrin.h>

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
//	size_t all_positions = 0;
//	size_t all_games = 0;
//	size_t sym_positions = 0;
//
//	matrix<Sign> board(12, 12);
//	for (int i = 0; i < counter; i++)
//	{
//		GameBuffer buffer(path + "buffer_" + std::to_string(i) + ".bin");
//
//		all_positions = 0;
//		for (int j = 0; j < buffer.size(); j++)
//			all_positions += buffer.getFromBuffer(j).getNumberOfSamples();
//		std::cout << i << " " << all_positions << " " << buffer.size() << '\n';

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
//	}
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
	std::shared_ptr<SharedHashTable> sht = std::make_shared<SharedHashTable>(game_config.rows, game_config.cols, 1048576);
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
	GameConfig game_config(GameRules::STANDARD, 12);

	TreeConfig tree_config;
	Tree tree(tree_config);

	SearchConfig search_config;
	search_config.max_batch_size = 8;
	search_config.exploration_constant = 1.25f;
	search_config.max_children = 32;
	search_config.solver_level = 2;
	search_config.solver_max_positions = 100;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
	device_config.device = ml::Device::cuda(0);
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/solver_2/checkpoint/network_1_opt.bin");

	SequentialHalvingSelector selector(16, 200);

	matrix<Sign> board(12, 12);
	board = Board::fromString(""
			" _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _\n"
			" O _ X _ _ _ _ _ _ _ _ _\n"
			" _ O _ _ _ X _ _ _ _ _ _\n"
			" _ _ X _ O _ O _ _ _ _ _\n"
			" _ X O _ _ _ _ _ _ _ _ _\n"
			" _ X _ O _ _ _ _ _ _ _ _\n"
			" _ X O _ _ _ _ _ _ _ _ _\n"
			" X X _ _ _ _ _ _ _ _ _ _\n");
	const Sign sign_to_move = Sign::CROSS;

//	board = Board::fromString(""
//	/*         a b c d e f g h i j k l m n o          */
//	/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */" _ _ _ _ O _ _ _ X _ _ _ _ _ _\n" /*  6 */
//			/*  7 */" _ _ _ X _ X O _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */" _ _ X O O O _ X _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */" _ _ _ _ O O X _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */" _ _ O _ X _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//	/*         a b c d e f g h i j k l m n o          */);
//	const Sign sign_to_move = Sign::CROSS;

	Search search(game_config, search_config);
	tree.setBoard(board, sign_to_move);
	tree.setEdgeSelector(SequentialHalvingSelector(search_config.max_children, 100));
//	tree.setEdgeSelector(NoisyPUCTSelector(1.0f, 0.5f));
	tree.setEdgeGenerator(BaseGenerator(search_config.max_children, true));

	search.setBatchSize(search_config.max_batch_size);
	int next_step = 0;
	for (int j = 0; j <= 100; j++)
	{
//		if (tree.getSimulationCount() >= next_step)
//		{
//			std::cout << tree.getSimulationCount() << " ..." << std::endl;
//			next_step += 10000;
//		}
		search.select(tree, 100);
		search.solve();
		search.scheduleToNN(nn_evaluator);
		nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
//		tree.printSubtree(5, false);

		if (tree.isRootProven() or tree.getSimulationCount() >= 101)
			break;
	}
	search.cleanup(tree);

//	tree.printSubtree();
//	return;
//	std::cout << search.getStats().toString() << '\n';
//	std::cout << "memory = " << ((tree.getMemory() + search.getMemory()) / 1048576.0) << "MB\n\n";
//	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
//	std::cout << tree.getNodeCacheStats().toString() << '\n';
//	std::cout << nn_evaluator.getStats().toString() << '\n';
//
	Node info = tree.getInfo( { });
	info.sortEdges();
//	std::cout << info.toString() << '\n';
//	for (int i = 0; i < info.numberOfEdges(); i++)
//		std::cout << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
//	std::cout << '\n';

	auto get_expectation = [](const Edge *edge)
	{
		switch (edge->getScore().getProvenValue())
		{
			case ProvenValue::LOSS:
			return Value::loss().getExpectation();
			case ProvenValue::DRAW:
			return Value::draw().getExpectation();
			default:
			case ProvenValue::UNKNOWN:
			return edge->getValue().getExpectation();
			case ProvenValue::WIN:
			return Value::win().getExpectation();
		}
	};

	int max_N = 0, sum_N = 0;
	float sum_v = 0.0f, sum_q = 0.0f, sum_p = 0.0f;
	for (Edge *edge = info.begin(); edge < info.end(); edge++)
		if (edge->getVisits() > 0 or edge->isProven())
		{
			sum_v += edge->getValue().getExpectation() * edge->getVisits();
			sum_q += edge->getPolicyPrior() * get_expectation(edge);
			sum_p += edge->getPolicyPrior();
			max_N = std::max(max_N, edge->getVisits());
			sum_N += edge->getVisits();
		}
	float V_mix = 0.0f;
	if (sum_N > 0) //or sum_p > 0.0f)
	{
		const float inv_N = 1.0f / sum_N;
		V_mix = (info.getValue().getExpectation() - sum_v * inv_N) + (1.0f - inv_N) / sum_p * sum_q;
	}
	else
		V_mix = info.getValue().getExpectation();
	std::cout << "V_mix = " << V_mix << '\n';
	std::cout << sum_v << " " << sum_q << " " << sum_p << " " << max_N << '\n';

	std::vector<float> asdf;
	for (int i = 0; i < info.numberOfEdges(); i++)
	{
		float Q;
		switch (info.getEdge(i).getScore().getProvenValue())
		{
			case ProvenValue::LOSS:
				Q = -100.0f + 0.1f * info.getEdge(i).getScore().getDistance();
				break;
			case ProvenValue::DRAW:
				Q = Value::draw().getExpectation();
				break;
			default:
			case ProvenValue::UNKNOWN:
				Q = (info.getEdge(i).getVisits() > 0) ? info.getEdge(i).getValue().getExpectation() : V_mix;
				break;
			case ProvenValue::WIN:
				Q = +101.0f - 0.1f * info.getEdge(i).getScore().getDistance();
				break;
		}
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

	std::cout << Board::toString(board, true);
	std::cout << info.toString() << '\n';
	for (int i = 0; i < info.numberOfEdges(); i++)
		std::cout << asdf[i] * inv_sum << " : " << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
	std::cout << '\n';
}

float cross_entropy(float output, float target) noexcept
{
	output = std::max(0.0f, std::min(1.0f, output));
	return -target * safe_log(output) - (1.0f - target) * safe_log(1.0f - output);
}

void run_training()
{
	ml::Device::setNumberOfThreads(1);
	GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/train_buffer/buffer_34.bin");
	buffer.removeRange(1024, buffer.size());
	std::cout << buffer.getStats().toString() << '\n';

	SearchDataPack pack(15, 15);
	buffer.getGameData(0).getSample(pack, 0);

//	std::cout << "\n\n----------------------------------------------\n\n";
//	pack.print();
//	return;

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.augment_training_data = false;
	training_config.blocks = 10;
	training_config.filters = 64;
	training_config.device_config.batch_size = 1024;
	training_config.l2_regularization = 1.0e-5f;

	AGNetwork network(game_config, training_config);
//	network.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3_5x64.bin");
	network.moveTo(ml::Device::cuda(1));
//	network.moveTo(ml::Device::cpu());
//	network.forward(2);
//	network.backward(2);
//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	SupervisedLearning sl(training_config);

	for (int e = 0; e < 10; e++)
	{
		if (e == 2)
			network.changeLearningRate(1.0e-4f);
		if (e == 6)
			network.changeLearningRate(1.0e-5f);
		sl.clearStats();
		sl.train(network, buffer, 500);
//		sl.saveTrainingHistory("/home/maciek/alphagomoku/new_runs_2023/");
//		network.saveToFile("/home/maciek/alphagomoku/new_runs_2023/network_overfit.bin");
	}
//	network.optimize();

//	for (int i = 0; i < 8; i++)
//	{
//		buffer.getGameData(0).getSample(pack, i);
//		network.packInputData(0, pack.board, pack.played_move.sign);
//		network.forward(1);
//
//		matrix<float> policy(15, 15);
//		matrix<Value> action_values(15, 15);
//		Value value;
//
//		network.unpackOutput(0, policy, action_values, value);
//		std::cout << '\n';
//		std::cout << "Value = " << value.toString() << '\n'; //<< " (" << value_target.toString() << ")\n";
//		std::cout << Board::toString(pack.board, policy, true) << '\n';
//		std::cout << Board::toString(pack.board, action_values, true) << '\n';
//	}

	network.saveToFile("/home/maciek/alphagomoku/new_runs_2023/network_overfit.bin");
//	network.loadFromFile("/home/maciek/alphagomoku/new_runs_2023/network_overfit.bin");

	matrix<float> policy(15, 15);
	matrix<Value> action_values(15, 15);
	Value value;

	float loss = 0.0f;
	int counter = 0;
//	for (int g = 0; g < 32; g++)
//	{
//		for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
//		{
//			buffer.getGameData(g).getSample(pack, i);
//			network.packInputData(i, pack.board, pack.played_move.sign);
//		}
//
//		network.forward(buffer.getGameData(g).numberOfSamples());
//
//		for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
//		{
//			buffer.getGameData(g).getSample(pack, i);
//			network.unpackOutput(i, policy, action_values, value);
//
//			const Value t = convertOutcome(pack.game_outcome, pack.played_move.sign);
//			loss += cross_entropy(value.win_rate, t.win_rate);
//			loss += cross_entropy(value.draw_rate, t.draw_rate);
//			loss += cross_entropy(value.loss_rate(), t.loss_rate());
//			counter++;
//		}
//	}
//	std::cout << loss << " " << counter << '\n';
//	std::cout << "final loss = " << loss / counter << " (" << counter << " samples)\n";

	network.optimize();
	network.saveToFile("/home/maciek/alphagomoku/new_runs_2023/network_overfit_opt.bin");
	network.unloadGraph();
	network.loadFromFile("/home/maciek/alphagomoku/new_runs_2023/network_overfit_opt.bin");
	loss = 0.0f;
	counter = 0;

	for (int g = 0; g < 1024; g++)
	{
//		for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
//		{
//			buffer.getGameData(g).getSample(pack, i);
//			network.packInputData(i, pack.board, pack.played_move.sign);
//		}
//
//		network.forward(buffer.getGameData(g).numberOfSamples());
//
//		for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
//		{
//			buffer.getGameData(g).getSample(pack, i);
//			network.unpackOutput(i, policy, action_values, value);
//
//			const Value t = convertOutcome(pack.game_outcome, pack.played_move.sign);
//			loss += cross_entropy(value.win_rate, t.win_rate);
//			loss += cross_entropy(value.draw_rate, t.draw_rate);
//			loss += cross_entropy(value.loss_rate(), t.loss_rate());
//			counter++;
//		}

		bool flag = true;
		for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
		{
			buffer.getGameData(g).getSample(pack, i);

			network.packInputData(0, pack.board, pack.played_move.sign);
			network.forward(1);
			network.unpackOutput(0, policy, action_values, value);

			const Value t = convertOutcome(pack.game_outcome, pack.played_move.sign);
			loss += cross_entropy(value.win_rate, t.win_rate);
			loss += cross_entropy(value.draw_rate, t.draw_rate);
			loss += cross_entropy(value.loss_rate(), t.loss_rate());
			counter++;

			if (std::fabs(value.getExpectation() - t.getExpectation()) > 0.1f and flag)
			{
				std::cout << g << " " << i << " : " << value.toString() << "  vs  " << t.toString() << ",  Minimax: Q="
						<< pack.minimax_value.toString() << ", S=" << pack.minimax_score.toString() << '\n';
				flag = false;
			}
		}
	}
	std::cout << loss << " " << counter << '\n';
	std::cout << "loaded loss = " << loss / counter << " (" << counter << " samples)\n";
}

//// no inline, required by [replacement.functions]/3
//void* operator new(std::size_t sz)
//{
//	std::printf("1) new(size_t), size = %zu\n", sz);
//	if (sz == 0)
//		++sz; // avoid std::malloc(0) which may return nullptr on success
//
//	if (void *ptr = std::malloc(sz))
//		return ptr;
//
//	throw std::bad_alloc { }; // required by [new.delete.single]/3
//}
//
//// no inline, required by [replacement.functions]/3
//void* operator new[](std::size_t sz)
//{
//	std::printf("2) new[](size_t), size = %zu\n", sz);
//	if (sz == 0)
//		++sz; // avoid std::malloc(0) which may return nullptr on success
//
//	if (void *ptr = std::malloc(sz))
//		return ptr;
//
//	throw std::bad_alloc { }; // required by [new.delete.single]/3
//}
//
//void operator delete(void *ptr) noexcept
//{
//	std::puts("3) delete(void*)");
//	std::free(ptr);
//}
//
//void operator delete(void *ptr, std::size_t size) noexcept
//{
//	std::printf("4) delete(void*, size_t), size = %zu\n", size);
//	std::free(ptr);
//}
//
//void operator delete[](void *ptr) noexcept
//{
//	std::puts("5) delete[](void* ptr)");
//	std::free(ptr);
//}
//
//void operator delete[](void *ptr, std::size_t size) noexcept
//{
//	std::printf("6) delete[](void*, size_t), size = %zu\n", size);
//	std::free(ptr);
//}

std::string get_BOARD_command(const matrix<Sign> &board, Sign signToMove)
{
	std::vector<Move> moves_cross;
	std::vector<Move> moves_circle;
	for (int r = 0; r < board.rows(); r++)
		for (int c = 0; c < board.cols(); c++)
			switch (board.at(r, c))
			{
				default:
					break;
				case Sign::CROSS:
					moves_cross.push_back(Move(r, c, Sign::CROSS));
					break;
				case Sign::CIRCLE:
					moves_circle.push_back(Move(r, c, Sign::CIRCLE));
					break;
			}

	std::string result = "BOARD\n";
	if (moves_cross.size() == moves_circle.size()) // I started the game as first player (CROSS)
	{
		for (size_t i = 0; i < moves_cross.size(); i++)
		{
			result += std::to_string(moves_cross[i].col) + ',' + std::to_string(moves_cross[i].row) + ",1\n";
			result += std::to_string(moves_circle[i].col) + ',' + std::to_string(moves_circle[i].row) + ",2\n";
		}
	}
	else
	{
		if (moves_cross.size() == moves_circle.size() + 1) // opponent started the game as first player (CROSS)
		{
			for (size_t i = 0; i < moves_circle.size(); i++)
			{
				result += std::to_string(moves_cross[i].col) + ',' + std::to_string(moves_cross[i].row) + ",2\n";
				result += std::to_string(moves_circle[i].col) + ',' + std::to_string(moves_circle[i].row) + ",1\n";
			}
			result += std::to_string(moves_cross.back().col) + ',' + std::to_string(moves_cross.back().row) + ",2\n";
		}
		else
			throw ProtocolRuntimeException("Invalid position - too many stones either color");
	}

	return result + "DONE\n";
}

void train_simple_evaluation()
{
////	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 0; i < 17; i++)
//#else
//	for (int i = 0; i < 1; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator calculator(game_config);
//	nnue::TrainingNNUE evaluator(game_config);
//
//	float learning_rate = 0.001f;
//	const float weight_decay = 0.0e-5f;
//	const int batch_size = 32;
//	TimedStat forward("forward");
//
////	for (int i = 0; i < 10; i++)
////		std::cout << buffer.getFromBuffer(i).getNumberOfSamples() << '\n';
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	for (int epoch = 0; epoch < 100; epoch++)
//	{
//		std::vector<int> ordering = permutation(buffer.size());
//		float avg_loss = 0.0f;
//		for (int step = 0; step < buffer.size(); step += batch_size)
//		{
//			for (int batch = 0; batch < batch_size; batch++)
//			{
////				std::cout << epoch << ", " << step << ", " << batch << " : " << avg_loss << "\n";
//				const int idx = ordering[(step + batch) % ordering.size()];
//				const SearchData &sample = buffer.getFromBuffer(idx).getSample();
////				const SearchData &sample = buffer.getFromBuffer(step % 5).getSample();
//
//				sample.getBoard(board);
//				augment(board, randInt(8));
//				const Sign sign_to_move = sample.getMove().sign;
//
////				Value target = convertOutcome(sample.getOutcome(), sign_to_move);
//				Value target = sample.getMinimaxValue();
//
//				target.win_rate = std::max(0.0f, std::min(1.0f, target.win_rate));
//				target.draw_rate = std::max(0.0f, std::min(1.0f - target.win_rate, target.draw_rate));
//
//				calculator.setBoard(board, sign_to_move);
//				forward.startTimer();
//				const Value output = evaluator.forward(calculator);
//				forward.stopTimer();
////				std::cout << output.toString() << " vs " << target.toString() << '\n';
//				avg_loss += evaluator.backward(calculator, target);
//			}
//			evaluator.learn(learning_rate, weight_decay);
//			if (std::isnan(avg_loss))
//			{
//				std::cout << "NaN loss encountered, exiting\n";
//				return;
//			}
//			if (epoch == 30 or epoch == 60)
//				learning_rate *= 0.1f;
//		}
//		std::cout << "epoch " << epoch << ", avg loss = " << avg_loss / buffer.size() << '\n';
////		std::cout << "epoch " << epoch << ", avg loss = " << avg_loss << '\n';
//	}
//	nnue::NNUEWeights w = evaluator.dump();
//	SerializedObject so;
//	Json json = w.save(so);
//	FileSaver("/home/maciek/Desktop/standard_nnue_32x8x1.bin").save(json, so);
////	FileSaver("/home/maciek/Desktop/freestyle_nnue_32x8x1.bin").save(json, so);
////	nnue::InferenceNNUE nn(game_config, w);
////	nn.refresh(calculator);
////
////	calculator.addMove(Move("Oh5"));
////	nn.update(calculator);
////	calculator.print();
////	std::cout << "baseline  = " << evaluator.forward(calculator).win << '\n';
////	std::cout << "optimized = " << nn.forward() << '\n';
//
////	double t0, t1;
////	t0 = getTime();
////	for (int i = 0; i < 1000000; i++)
////		nn.refresh(calculator);
////	t1 = getTime();
////	std::cout << "nn.refresh() = " << (t1 - t0) << "us\n";
////
////	t0 = getTime();
////	for (int i = 0; i < 1000000; i++)
////		nn.update(calculator);
////	t1 = getTime();
////	std::cout << "nn.update() = " << (t1 - t0) << "us\n";
////
////	t0 = getTime();
////	float tmp = 0.0f;
////	for (int i = 0; i < 1000000; i++)
////		tmp += nn.forward();
////	t1 = getTime();
////	std::cout << "nn.forward() = " << (t1 - t0) << "us\n";
////
////	std::cout << nn.forward() << '\n';
////	std::cout << tmp << '\n';
//	std::cout << "END" << std::endl;
}

void test_pattern_calculator()
{
////	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	PatternTable::get(game_config.rules);
//	ThreatTable::get(game_config.rules);
//	DefensiveMoveTable::get(game_config.rules);
//
//	GameDataBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 20; i <= 24; i++)
//#else
//	for (int i = 24; i <= 24; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/new_runs_2023/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator extractor_new(game_config);
//	PatternCalculator extractor_new2(game_config);
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//
//	int count = 0;
//	for (int i = 0; i < buffer.size(); i++)
//	{
//		if (i % (buffer.size() / 10) == 0)
//			std::cout << i << " / " << buffer.size() << '\n';
//
//		buffer.getGameData(i).getSample(0).getBoard(board);
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			const SearchData &sample = buffer.getGameData(i).getSample(j);
//			sample.getBoard(board);
//			extractor_new.setBoard(board, sample.getMove().sign);
//
////			if (extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::FIVE).size() > 0
////					and (extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::OPEN_4).size() > 0
////							or extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::FORK_4x4).size() > 0))
////				count++;
////			if (extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::FIVE).size() > 0
////					and (extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::OPEN_4).size() > 0
////							or extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::FORK_4x4).size() > 0))
////				count++;
//
//			for (int k = 0; k < 100; k++)
//			{
//				const int x = randInt(game_config.rows);
//				const int y = randInt(game_config.cols);
//				if (board.at(x, y) == Sign::NONE)
//				{
//					Sign sign = static_cast<Sign>(randInt(1, 3));
//					extractor_new.addMove(Move(x, y, sign));
//					board.at(x, y) = sign;
//				}
//				else
//				{
//					extractor_new.undoMove(Move(x, y, board.at(x, y)));
//					board.at(x, y) = Sign::NONE;
//				}
//			}
//
////			extractor_new2.setBoard(board, sample.getMove().sign);
////			for (int x = 0; x < game_config.rows; x++)
////				for (int y = 0; y < game_config.cols; y++)
////				{
////					for (Direction dir = 0; dir < 4; dir++)
////					{
////						if (extractor_new2.getNormalPatternAt(x, y, dir) != extractor_new.getNormalPatternAt(x, y, dir))
////						{
////							std::cout << "Raw pattern mismatch\n";
////							std::cout << "Single step\n";
////							extractor_new2.printRawFeature(x, y);
////							std::cout << "incremental\n";
////							extractor_new.printRawFeature(x, y);
////							exit(-1);
////						}
////						if (extractor_new2.getPatternTypeAt(Sign::CROSS, x, y, dir) != extractor_new.getPatternTypeAt(Sign::CROSS, x, y, dir)
////								or extractor_new2.getPatternTypeAt(Sign::CIRCLE, x, y, dir)
////										!= extractor_new.getPatternTypeAt(Sign::CIRCLE, x, y, dir))
////						{
////							std::cout << "Pattern type mismatch\n";
////							std::cout << "Single step\n";
////							extractor_new2.printRawFeature(x, y);
////							std::cout << "incremental\n";
////							extractor_new.printRawFeature(x, y);
////							exit(-1);
////						}
////					}
////					if (extractor_new2.getThreatAt(Sign::CROSS, x, y) != extractor_new.getThreatAt(Sign::CROSS, x, y)
////							or extractor_new2.getThreatAt(Sign::CIRCLE, x, y) != extractor_new.getThreatAt(Sign::CIRCLE, x, y))
////					{
////						std::cout << "Threat type mismatch\n";
////						std::cout << "Single step\n";
////						extractor_new2.printThreat(x, y);
////						std::cout << "incremental\n";
////						extractor_new.printThreat(x, y);
////						exit(-1);
////					}
////
////					for (int i = 0; i < 10; i++)
////					{
////						if (extractor_new2.getThreatHistogram(Sign::CROSS).get((ThreatType) i).size()
////								!= extractor_new.getThreatHistogram(Sign::CROSS).get((ThreatType) i).size())
////						{
////							std::cout << "Threat histogram mismatch for cross\n";
////							std::cout << "Single step\n";
////							extractor_new2.getThreatHistogram(Sign::CROSS).print();
////							std::cout << "incremental\n";
////							extractor_new.getThreatHistogram(Sign::CROSS).print();
////							exit(-1);
////						}
////						if (extractor_new2.getThreatHistogram(Sign::CIRCLE).get((ThreatType) i).size()
////								!= extractor_new.getThreatHistogram(Sign::CIRCLE).get((ThreatType) i).size())
////						{
////							std::cout << "Threat histogram mismatch for circle\n";
////							std::cout << "Single step\n";
////							extractor_new2.print();
////							extractor_new2.getThreatHistogram(Sign::CIRCLE).print();
////							std::cout << "incremental\n";
////							extractor_new.print();
////							extractor_new.getThreatHistogram(Sign::CIRCLE).print();
////							exit(-1);
////						}
////					}
////				}
//		}
//	}
//
//	std::cout << "count  = " << count << '\n';
//	std::cout << "New extractor\n";
//	extractor_new.print_stats();
}

void test_proven_positions(int pos)
{
////	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
////	GameConfig game_config(GameRules::RENJU, 15);
////	GameConfig game_config(GameRules::CARO5, 15);
////	GameConfig game_config(GameRules::CARO6, 20);
//
//	PatternTable::get(game_config.rules);
//	ThreatTable::get(game_config.rules);
//	DefensiveMoveTable::get(game_config.rules);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 20; i <= 24; i++)
//#else
//	for (int i = 24; i <= 24; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/new_runs_2023/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	ThreatSpaceSearch ts_search(game_config, pos);
//	std::shared_ptr<SharedHashTable> sht = std::make_shared<SharedHashTable>(game_config.rows, game_config.cols, 1 << 23);
//	ts_search.setSharedTable(sht);
//
//	SearchTask task(game_config);
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	int total_samples = 0;
//	int solved_count = 0;
//
//	for (int i = 0; i < buffer.size(); i++)
////	int i = 9;
//	{
//		if (i % (buffer.size() / 10) == 0)
//			std::cout << i << " / " << buffer.size() << " " << solved_count << "/" << total_samples << '\n';
//
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
////		int j = 0;
//		{
////			std::cout << i << " " << j << '\n';
//			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
//			sample.getBoard(board);
//			const Sign sign_to_move = sample.getMove().sign;
//
//			task.set(board, sign_to_move);
//			sht->increaseGeneration();
//			ts_search.solve(task, TssMode::RECURSIVE, pos);
//			solved_count += task.getScore().isProven();
//		}
//	}
//
//	std::cout << "TSS\n";
//	std::cout << "solved " << solved_count << " samples (" << 100.0f * solved_count / total_samples << "%)\n";
//	ts_search.print_stats();
}

void test_solver(int pos)
{
////	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 0; i < 17; i++)
//#else
//	for (int i = 0; i < 1; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	int total_samples = 0;
//
//	std::fstream output("/home/maciek/Desktop/win_all.txt", std::fstream::out);
//
//	for (int i = 0; i < buffer.size(); i++)
//	{
//		if (i % (buffer.size() / 10) == 0)
//			std::cout << i << " / " << buffer.size() << '\n';
//
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
//			sample.getBoard(board);
//			const Sign sign_to_move = sample.getMove().sign;
//			output << get_BOARD_command(board, sign_to_move) << "nodes 0\n";
//		}
//	}
}

void test_forbidden_moves()
{
//	GameConfig game_config(GameRules::RENJU, 15);
//
//	GameBuffer buffer;
////#ifdef NDEBUG
//	for (int i = 0; i <= 21; i++)
////#else
////		for (int i = 0; i < 1; i++)
////#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator calc(game_config);
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	int total_samples = 0;
//
//	const double start = getTime();
//	for (int i = 0; i < buffer.size(); i++)
////	int i = 143634;
//	{
//		if (i % (buffer.size() / 100) == 0)
//			std::cout << i << " / " << buffer.size() << '\n';
//
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
////		int j = 0;
//		{
//			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
//			sample.getBoard(board);
//			const Sign sign_to_move = sample.getMove().sign;
//
//			calc.setBoard(board, sign_to_move);
//			for (int r = 0; r < game_config.rows; r++)
//				for (int c = 0; c < game_config.cols; c++)
//				{
//					const bool b0 = isForbidden(board, Move(r, c, Sign::CROSS));
//					const bool b1 = calc.isForbidden(Sign::CROSS, r, c);
//					if (b0 != b1)
//					{
//						std::cout << "mismatch\n";
//						return;
//					}
//				}
//
////			bool flag = false;
////			IsOverline is_overline(game_config.rules, Sign::CIRCLE);
////			for (int r = 0; r < game_config.rows; r++)
////			{
////				if (flag == false)
////					for (int c = 0; c < game_config.cols; c++)
////						for (int d = 0; d < 4; d++)
////							if (is_overline(Pattern(11, calc.getNormalPatternAt(r, c, d))))
////							{
////								flag = true;
////								break;
////							}
////			}
//		}
//	}
//
//	std::cout << "time " << (getTime() - start) << ", samples = " << total_samples << '\n';
}

void test_search()
{
//	GameConfig game_config(GameRules::CARO, 15);
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::FREESTYLE, 20);

	TreeConfig tree_config;
	Tree tree(tree_config);

	SearchConfig search_config;
	search_config.max_batch_size = 32;
	search_config.exploration_constant = 1.25f;
	search_config.max_children = 32;
	search_config.solver_level = 0;
	search_config.solver_max_positions = 200;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
//#ifdef NDEBUG
	device_config.device = ml::Device::cuda(0);
//#else
//	device_config.device = ml::Device::cpu();
//#endif
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(true);
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/network_test2_6x64s_opt.bin");
	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/checkpoint/network_33_opt.bin");

	Sign sign_to_move;
	matrix<Sign> board(15, 15);

//	// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ O _ _ O _ _ X _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ X X O _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ X O O O X _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ O X _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */); // @formatter:on
//	board = Board::fromString(" _ _ _ X O O O O X _ _ _ _ _ _\n"
//			" X _ O O _ X O _ _ _ _ _ _ _ _\n"
//			" X X O X X X X O _ _ _ _ _ _ _\n"
//			" X X O X X X O _ X _ _ _ _ _ _\n"
//			" O O O X O X O _ _ O _ _ O _ _\n"
//			" _ X O O X O X X _ X _ X _ _ _\n"
//			" _ _ O O X O X O _ O O _ _ _ _\n"
//			" _ _ X O O O X _ _ O _ _ _ _ _\n"
//			" _ _ O O X X _ X O O O _ _ _ _\n"
//			" _ O X X X X O X X X _ _ _ _ _\n"
//			" _ _ X O O X X O X O _ _ _ _ _\n"
//			" O X X X X O O O X _ _ _ _ _ _\n"
//			" O _ _ O _ _ _ X X _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
//			" _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n");
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o          */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*        a b c d e f g h i j k l m n o          */);  // @formatter:on
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ O _ _ O _ _ X _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ X X O _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ X O O O X\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ X _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ O X\n"/* 11 */
//			/*        a b c d e f g h i j k l         */); // @formatter:on
// @formatter:off
//	board = Board::fromString(
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ O O O X _ _ _ _ _ _\n"
//			" _ _ _ X O X X _ _ O _ _ _ _ _\n"
//			" _ _ _ _ X O O _ X _ _ _ _ _ _\n"
//			" _ _ _ _ _ X X O _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"); // @formatter:on
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ O X O O O _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ X O X X _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ X _ _ _ X _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);      // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" O X X X X _ X _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" O X X O _ O X _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" X _ _ O X _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" O _ _ _ O _ _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" X _ _ _ O O _ _ _ _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);         // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ X _ _ O _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" O X X X X _ X _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" O X _ O X _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" X X O _ X _ X _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" O O X O O _ O _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" X _ _ _ O _ _ _ _ _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);          // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" X O O O O X _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ X O X X O X _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ X O O O _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ O _ X _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ X O X X _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ O O _ _ _ _ _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);          // @formatter:on
//	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ O X _ _ _ _ _ O _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ X _ _ O X X _ X _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ X _ _ _ X _ _ O _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ O _ O O O O X _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ X _ O X _ _ O X _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ X X _ O X O O _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ O X X X _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ _ _ _ _ O _ O X _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);          // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ O O _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ O _ O X X X _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ X O X X _ O X _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ X X O O O O X _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ O _ _ O O X O X X _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ _ O _ X X X O _ O _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ X _ _ O _ X _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ O X O O _ X O _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ O X _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ X O X X X O\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ X _ X _ O _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);          // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//	std::cout << get_BOARD_command(board, sign_to_move);
//	return;
//
//	// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o          */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ O X O _ _ _\n" /*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ X O X X O _ _\n" /*  4 */
//			/*  5 */" _ _ _ _ _ _ O _ X _ O X O _ _\n" /*  5 */
//			/*  6 */" _ _ _ _ O X X X O O _ X _ _ _\n" /*  6 */
//			/*  7 */" _ _ _ X _ O O O X X _ X _ _ _\n" /*  7 */
//			/*  8 */" _ _ _ O _ X X O X _ O O _ _ _\n" /*  8 */
//			/*  9 */" _ _ _ O X O _ O O X X _ _ _ _\n" /*  9 */
//			/* 10 */" _ _ O O X O X O _ X _ O _ _ _\n" /* 10 */
//			/* 11 */" _ _ X X X O X X O O _ X _ _ _\n" /* 11 */
//			/* 12 */" _ _ X O O O O X _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ O X _ _ X X O X _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ O O_ _ _ _ _ _ _ _ _ _ \n"/*  4 */
//			/*  5 */" _ O O X O _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ X O X X _ _ _ _ _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */); // @formatter:on
//	sign_to_move = Sign::CROSS;
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ \n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ X O O O _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ X _ X _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */); // @formatter:on
//	sign_to_move = Sign::CROSS;
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ O X X _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ O _ _ X _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ O _ X _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ X O O _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ X X _ _ X _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */); // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ X X _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ O O X O _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ O X X X O _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ O X O _ O _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ O X X X _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);                          // @formatter:on
//	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString( // sure loss if played Om12
//			/*        a b c d e f g h i j k l m n o p q r s t         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"/*  5 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ O O O X _ X _ _\n"/*  6 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ ! _ _ X O _ _ _\n"/*  7 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ X O X _ O _\n"/*  8 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ X O O O X _ _\n"/*  9 */
//			/* 15 */" _ _ _ _ _ _ _ _ _ _ X _ O O O X _ X X _\n"/* 10 */
//			/* 16 */" _ _ _ _ _ _ _ _ _ _ _ _ X O X X X X O O\n"/* 11 */
//			/* 17 */" _ _ _ _ _ _ _ _ _ _ _ _ O O X O _ O _ _\n"/* 12 */
//			/* 18 */" _ _ _ _ _ _ _ _ _ _ _ X _ O X _ _ _ _ _\n"/* 13 */
//			/* 19 */" _ _ _ _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o p q r s t         */); // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ O O O X _ _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ _ O X _ _ O _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ _ _ _ X O X X _ X _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ X O _ O _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ X O X X _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ O X X _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ O _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);  // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(	" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								" _ _ _ _ _ O _ O _ _ _ _ _ _ _\n"
//								" _ _ _ _ _ X X X _ _ O _ _ _ _\n"
//								" X _ _ X _ _ O O _ X _ _ _ _ _\n"
//								" _ O _ _ O _ X O O _ _ O O _ _\n"
//								" _ _ X _ X O X O O O X X O _ _\n"
//								" _ _ _ X _ O O X X O X X _ _ O\n"
//								" _ _ _ _ X X _ O O X X X X O _\n"
//								" _ _ _ O X O O X X X O X X _ _\n"
//								" _ _ X O O O X O X O O O _ _ _\n"
//								" X _ O _ X X _ O X O O X _ O _\n"
//								" _ O O X X X X O O X _ X X _ _\n"
//								" O X X O O O O X X O X O _ _ _\n"
//								" _ O _ X _ _ _ X X _ O O _ _ _\n"
//								" _ _ _ O X X _ _ _ O _ X X _ _\n"); // @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(	/*         a b c d e f g h i j k l m n o        */
//								/*  0 */ " _ _ _ _ _ _ _ _ _ X _ _ _ _ O\n" /*  0 */
//								/*  1 */ " _ _ _ _ _ _ _ _ _ X _ X ! X _\n" /*  1 */
//								/*  2 */ " _ _ _ _ _ _ _ _ _ O O O X _ _\n" /*  2 */
//								/*  3 */ " _ _ _ _ _ _ _ _ O O _ X _ _ _\n" /*  3 */
//								/*  4 */ " _ _ O _ _ _ _ _ O O X X X _ _\n" /*  4 */
//								/*  5 */ " _ _ _ _ _ _ _ X _ O _ _ _ _ _\n" /*  5 */
//								/*  6 */ " _ _ _ _ _ _ O _ _ X _ _ _ _ _\n" /*  6 */
//								/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//								/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//								/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//								/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//								/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//								/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//								/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//								/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//								/*         a b c d e f g h i j k l m n o          */);   // @formatter:on
//	sign_to_move = Sign::CROSS;

//// @formatter:off
//	board = Board::fromString(	/*         a b c d e f g h i j k l m n o        */
//								/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//								/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//								/*  2 */ " _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//								/*  3 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  3 */
//								/*  4 */ " _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /*  4 */
//								/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//								/*  6 */ " _ O O _ O _ O O _ _ _ _ _ _ _\n" /*  6 */
//								/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//								/*  8 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  8 */
//								/*  9 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  9 */
//								/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//								/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//								/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//								/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//								/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//								/*         a b c d e f g h i j k l m n o          */); // @formatter:on

//// @formatter:off
//	board = Board::fromString(	/*         a b c d e f g h i j k l m n o        */
//								/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//								/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//								/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//								/*  3 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  3 */
//								/*  4 */ " _ _ _ _ _ _ _ _ O X X _ X _ _\n" /*  4 */
//								/*  5 */ " _ _ _ _ _ _ _ _ _ X O _ _ _ _\n" /*  5 */
//								/*  6 */ " _ _ _ _ _ _ _ _ _ X O _ X _ _\n" /*  6 */
//								/*  7 */ " _ _ _ _ _ _ _ _ _ _ O _ X _ _\n" /*  7 */
//								/*  8 */ " _ _ _ _ _ _ _ _ _ _ X O O _ _\n" /*  8 */
//								/*  9 */ " _ _ _ _ _ _ _ _ _ O _ X _ _ _\n" /*  9 */
//								/* 10 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /* 10 */
//								/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//								/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//								/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//								/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//								/*         a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;
//// @formatter:off
//	board = Board::fromString(	/*         a b c d e f g h i j k l m n o        */
//								/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//								/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//								/*  2 */ " _ _ _ _ _ X _ _ X _ _ _ _ _ _\n" /*  2 */
//								/*  3 */ " _ _ _ _ _ _ O O X _ _ _ _ _ O\n" /*  3 */
//								/*  4 */ " _ _ _ _ _ _ _ X O O O _ _ O _\n" /*  4 */
//								/*  5 */ " _ _ _ _ O _ _ _ X X O X X _ O\n" /*  5 */
//								/*  6 */ " _ _ _ _ _ _ O X X X O O O _ X\n" /*  6 */
//								/*  7 */ " _ _ _ _ _ _ X O O O O X X _ X\n" /*  7 */
//								/*  8 */ " _ _ _ _ _ O _ X X _ X O _ O X\n" /*  8 */
//								/*  9 */ " _ _ _ _ _ _ _ _ _ O O X X X _\n" /*  9 */
//								/* 10 */ " _ _ _ _ _ _ O X X _ O X X _ _\n" /* 10 */
//								/* 11 */ " _ _ _ _ _ _ _ X _ O X X O _ _\n" /* 11 */
//								/* 12 */ " _ _ _ _ _ _ _ _ O X O _ O _ _\n" /* 12 */
//								/* 13 */ " _ _ _ _ _ _ _ X O _ _ _ _ _ _\n" /* 13 */
//								/* 14 */ " _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 14 */
//								/*         a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CROSS;

//// @formatter:off
//	board = Board::fromString(	/*        a b c d e f g h i j k l m n o          */
//								/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//								/*  1 */" _ _ _ _ _ _ _ _ X O O _ O _ _\n" /*  1 */
//								/*  2 */" _ _ _ _ _ _ _ O _ O X X _ _ _\n" /*  2 */
//								/*  3 */" _ _ _ _ ! _ X O _ O X X X _ _\n" /*  3 */
//								/*  4 */" _ _ _ _ _ X X O X X _ O X O _\n" /*  4 */
//								/*  5 */" _ _ _ _ _ _ X X O O O X _ O _\n" /*  5 */
//								/*  6 */" _ _ _ _ _ O O X O X O _ X _ X\n" /*  6 */
//								/*  7 */" _ _ _ _ _ O _ X O X O O O X _\n" /*  7 */
//								/*  8 */" _ _ _ _ _ _ _ _ X _ X _ _ _ _\n" /*  8 */
//								/*  9 */" _ _ _ _ _ _ _ O _ O X _ _ _ _\n" /*  9 */
//								/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//								/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//								/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//								/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//								/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//								/*        a b c d e f g h i j k l m n o          */); // Xe3 is winning
//// @formatter:on
//	sign_to_move = Sign::CROSS;

//// @formatter:off
//	board = Board::fromString(	/*         a b c d e f g h i j k l m n o          */
//								/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//								/*  1 */ " _ _ _ _ _ _ O _ X O O X O O _\n" /*  1 */
//								/*  2 */ " _ _ _ O _ O X O _ O X X O _ _\n" /*  2 */
//								/*  3 */ " _ _ _ _ X _ X O _ O X X X X O\n" /*  3 */
//								/*  4 */ " _ _ O X X X X O X X O O X O _\n" /*  4 */
//								/*  5 */ " _ _ _ _ _ _ X X O O O X X O _\n" /*  5 */
//								/*  6 */ " _ _ _ _ _ O O X O X O _ X _ X\n" /*  6 */
//								/*  7 */ " _ _ _ _ _ O _ X O X O O O X _\n" /*  7 */
//								/*  8 */ " _ _ _ _ _ _ _ _ X _ X _ _ _ X\n" /*  8 */
//								/*  9 */ " _ _ _ _ _ _ _ O _ O X _ _ _ _\n" /*  9 */
//								/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//								/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//								/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//								/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//								/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//								/*         a b c d e f g h i j k l m n o          */); // Xe5 is winning
//// @formatter:on
//	sign_to_move = Sign::CROSS;

//	board = Board::fromString(	" _ _ _ _ _ _ _ X _ _ _ X _ _ _\n"
//								" _ _ _ _ _ _ O O X O O O O X _\n"
//								" _ _ _ O O O X O _ O X X X _ _\n"
//								" _ _ _ _ X _ X O O O X X X _ _\n"
//								" _ _ O X X X X O X X X O X O _\n"
//								" _ _ _ _ X _ X X O O O X _ O _\n"
//								" _ _ _ _ _ O O X O X O _ X _ X\n"
//								" _ _ _ _ _ O _ X O X O O O X _\n"
//								" _ _ _ _ _ _ _ _ X _ X _ X _ O\n"
//								" _ _ _ _ _ _ _ O _ O X X _ O _\n"
//								" _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n"
//								" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//	sign_to_move = Sign::CIRCLE;
//
//	// @formatter:off
//	board = Board::fromString(
//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ _ X O X X _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ X X O X O _ _ X _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ X X O O _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ O _ _ _ O _ O _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ X _ _ O _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ X X _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ X _ _ _ _ X _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ O O O X _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ X _ _ X _ X _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(
//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ O _ X _ O X _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ O X X X _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ O X O _ _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ _ O X X X O _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ O X O _ O _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ O X X X _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CROSS;

//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o          */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ _ _ _ X _ X _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */" _ _ _ X _ _ X O O _ O _ _ _ _\n" /*  4 */
//			/*  5 */" _ _ _ _ O _ X _ _ O _ _ _ _ _\n" /*  5 */
//			/*  6 */" _ _ _ O X O O O O X _ _ _ _ _\n" /*  6 */
//			/*  7 */" _ _ _ _ _ X O X X _ _ _ _ _ _\n" /*  7 */
//			/*  8 */" _ _ _ _ _ X O X _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */" _ _ _ _ _ _ O _ _ _ X _ _ _ _\n" /*  9 */
//			/* 10 */" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*        a b c d e f g h i j k l m n o          */); // after Xe3 it is a win for circle
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o          */
//			/*  0 */" _ _ _ _ O _ _ ! _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ _ _ _ _ O ! _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" _ _ _ _ _ ! ! _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ _ _ _ X _ X ? _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */" _ _ _ X _ O X _ ? _ _ _ _ _ _\n" /*  4 */
//			/*  5 */" _ _ ! _ _ _ X _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//	// @formatter:off
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ X _ _ _ _ O _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " X _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ X X _ O _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ X X _ _ _ _ O _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ O _ _ _ _ O _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//	// @formatter:off
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ O X _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ O O X _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ X X O _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ X _ O _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ X _ _ X _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//	// @formatter:off
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ O _ _ _ X X X O _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ O O _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ _ _ _ O _ O _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ _ _ _ _ _ _ O O _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//// @formatter:on
//	sign_to_move = Sign::CROSS;

//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o          */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" X X _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ O _ X O O _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */" _ O O _ _ X _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */" _ O X O X X X O O X _ _ _ _ _\n" /*  5 */
//			/*  6 */" _ X X O O O O X X O _ _ _ _ _\n" /*  6 */
//			/*  7 */" _ _ X X O X _ O X O X X _ O _\n" /*  7 */
//			/*  8 */" _ _ X O X _ _ O X O O O O X _\n" /*  8 */
//			/*  9 */" _ _ O _ _ _ _ _ X _ _ O _ _ _\n" /*  9 */
//			/* 10 */" _ X _ _ _ _ O _ O X X O X _ _\n" /* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ X _ X _ _\n" /* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*        a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ X O X _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ O X O X X _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ X O _ O _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ O X X X O _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
//									" _ _ _ X _ X _ O _ _ _ _ _ _ _\n"
//									" _ _ _ O O X O _ _ _ _ O _ _ _\n"
//									" _ _ _ _ X X X O _ _ X _ _ _ _\n"
//									" _ _ _ O X O O O O X _ _ _ _ _\n"
//									" _ _ _ _ _ X O X X _ _ _ _ _ _\n"
//									" _ _ _ _ _ X O X _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ O _ _ _ X _ _ _ _\n"
//									" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								/*    a b c d e f g h i j k l m n o        */); // Oh2 or Of2 is a win
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
////// @formatter:off
////	board = Board::fromString(	" O _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
////								" X O _ _ X _ _ _ _ _ _ _ _ _ _\n"
////								" X X _ _ _ _ O _ _ _ X _ _ _ _\n"
////								" X O _ _ O X O _ O O _ _ _ _ _\n"
////								" X O _ O X X X X O _ _ _ _ _ _\n"
////								" O X X O X O O O _ X _ _ _ _ _\n"
////								" _ X O X O X X X O O _ O _ _ _\n"
////								" _ _ O X O O X O X _ _ _ _ _ _\n"
////								" _ _ _ O X X O X _ _ _ O _ _ _\n"
////								" _ _ _ _ O X O X O _ X X _ _ _\n"
////								" _ _ _ _ _ X X X O _ _ O _ _ _\n"
////								" _ _ _ _ O _ _ O X _ _ _ _ _ _\n"
////								" _ _ _ _ _ O _ _ _ X X _ _ _ _\n"
////								" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
////								" _ _ _ _ _ _ _ _ _ _ _ O _ _ _\n"); // Ok3 is w win
////// @formatter:on
////	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ O O _ _ _ _ _ _ _ _\n"
//									" _ _ X _ O _ O O X _ _ _ _ _ _\n"
//									" _ _ _ O _ X _ O _ _ _ O _ _ _\n"
//									" _ _ _ X O _ X X X O X _ _ _ _\n"
//									" _ _ _ O X O O O O X X _ _ _ _\n"
//									" _ _ _ _ _ X O X X _ _ _ _ _ _\n"
//									" _ _ _ _ _ X O X _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ O _ _ _ X _ _ _ _\n"
//									" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								/*    a b c d e f g h i j k l m n o          */); // position is a loss
//// @formatter:on
//	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 0
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 1
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 2
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 3
//									" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" // 4
//									" _ _ _ _ O _ _ _ X _ _ _ _ _ _\n" // 5
//									" _ _ _ O _ O X O _ _ _ _ _ _ _\n" // 6
//									" _ _ _ _ _ X O _ X _ _ _ _ _ _\n" // 7
//									" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" // 8
//									" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" // 9
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //10
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //11
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //12
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //13
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //14
//								/*    a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
//									" _ _ _ _ _ _ _ _ O X _ _ _ _ _\n" // 0
//									" _ _ _ _ _ _ _ _ _ _ _ _ X O _\n" // 1
//									" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" // 2
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 3
//									" _ _ _ _ _ _ _ _ _ O _ X _ _ O\n" // 4
//									" _ _ _ _ _ _ _ _ _ _ X O _ _ _\n" // 5
//									" _ _ _ _ _ _ X _ _ O O X X X _\n" // 6
//									" _ _ _ _ _ _ _ _ _ X O _ O _ _\n" // 7
//									" _ _ _ _ _ _ _ _ _ _ X O _ _ _\n" // 8
//									" _ _ _ _ _ _ _ _ _ _ O _ X _ _\n" // 9
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //10
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //11
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //12
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //13
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //14
//								/*    a b c d e f g h i j k l m n o          */);
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;
//
////// @formatter:off
////	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ X _ _ _ ! _ _ _ _ _ _ _ _\n"
////									" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ X O _ X _ _ _ _ _ _ _ _\n"
////									" _ _ _ O X O O O _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"
////									" _ _ _ _ _ X O X _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////								/*    a b c d e f g h i j k l m n o          */); // position is a win for circle
////// @formatter:on
////	sign_to_move = Sign::CIRCLE;
//
//// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o         */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
//			/*  6 */" _ _ _ _ _ O _ X X _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ _ O X O _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ O X X X O _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ O X O _ O _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ O X X X _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */);
//// @formatter:on
//	sign_to_move = Sign::CROSS;
//
//// @formatter:off
//	board = Board::fromString(
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ X O _ _ O X _ _ _ _ _ _\n"
//			" _ _ _ O _ O X O X _ _ _ _ _ _\n"
//			" _ _ _ _ X X O _ X _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ X O _ _ _ _ _ _\n"
//			" _ _ _ _ X O O X _ _ X _ _ _ _\n"
//			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//	sign_to_move = Sign::CROSS;

//// @formatter:off
//	board = Board::fromString(
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ O X _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ X X _ X O _ X X\n"
//			" _ _ _ _ _ _ _ O _ O O X _ X _\n"
//			" _ _ _ _ _ _ X _ X X O O X O _\n"
//			" _ _ _ _ _ _ _ _ O X _ O X _ _\n"
//			" _ _ _ _ _ _ X O O O O X O _ O\n"
//			" _ _ _ _ _ O _ _ X O X X O X _\n"
//			" _ _ _ _ _ _ X X O X X O O _ _\n"
//			" _ _ _ _ _ _ _ X O O X O X _ _\n"
//			" _ _ _ _ _ _ _ _ X O X X O _ _\n"
//			" _ _ _ _ _ _ _ _ _ O O X O _ _\n"
//			" _ _ _ _ _ O X O O O X O X _ _\n"
//			" _ _ _ _ _ _ _ _ _ X _ _ X _ _\n");
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

// @formatter:off
	board = Board::fromString(
			/*         a b c d e f g h i j k l m n o        */
			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
			/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
			/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
			/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
			/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
			/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
			/*  9 */ " X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
			/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
			/* 11 */ " _ O X O X X O _ _ _ _ _ _ _ _\n" /* 11 */
			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
			/*         a b c d e f g h i j k l m n o        */);
// @formatter:on
	sign_to_move = Sign::CIRCLE;

//	board = Board::fromString(""
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ X O _ _ O X _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ O O O X O X _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ X _ X X O _ X _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ O X _ X X O _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ X O O X _ _ X _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ X _ O _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//	sign_to_move = Sign::CROSS;

	GameConfig cfg(GameRules::STANDARD, 15, 15);
	ThreatSpaceSearch ts_search(cfg, 1000000);
	std::shared_ptr<SharedHashTable> sht = std::make_shared<SharedHashTable>(cfg.rows, cfg.cols, 1048576);
	ts_search.setSharedTable(sht);

	SearchTask task(game_config);
//	task.set(board, sign_to_move);
//	vcf_solver.solve(task, 5);
//	vcf_solver.print_stats();
//	std::cout << "\n\n\n" << task.toString() << '\n';

//	SearchTask task2(game_config);
//	task2.set(board, sign_to_move);
//	ts_search.solve(task2, TssMode::RECURSIVE, 100000);
//	std::cout << "\n\n\n";
//	ts_search.print_stats();
//	std::cout << "\n\n\n" << task2.toString() << '\n';
//	return;

	Search search(game_config, search_config);
	tree.setBoard(board, sign_to_move);
	tree.setEdgeSelector(PUCTSelector(1.25f));
//	tree.setEdgeSelector(UctSelector(0.05f));
	tree.setEdgeGenerator(BaseGenerator(search_config.max_children, true));

	int next_step = 0;
	for (int j = 0; j <= 400; j++)
	{
		if (tree.getSimulationCount() >= next_step)
		{
			std::cout << tree.getSimulationCount() << " ..." << std::endl;
			next_step += 10000;
		}
		search.select(tree, 400);
		search.solve();
		search.scheduleToNN(nn_evaluator);
		nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
//		search.tune();

//		std::cout << "\n\n\n";
//		tree.printSubtree(10, true, 5);
//		if (tree.isRootProven())
//		break;
	}
	search.cleanup(tree);

//	tree.printSubtree(1, true, 10);
	std::cout << search.getStats().toString() << '\n';
	std::cout << "memory = " << ((tree.getMemory() + search.getMemory()) / 1048576.0) << "MB\n\n";
	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
	std::cout << tree.getNodeCacheStats().toString() << '\n';
	std::cout << nn_evaluator.getStats().toString() << '\n';

	Node info = tree.getInfo( { });
	info.sortEdges();
	std::cout << info.toString() << '\n';
	for (int i = 0; i < info.numberOfEdges(); i++)
		std::cout << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
	std::cout << '\n';

	matrix<float> policy(game_config.rows, game_config.cols);
	matrix<ProvenValue> proven_values(game_config.rows, game_config.cols);
	proven_values.fill(ProvenValue::UNKNOWN);
	for (int i = 0; i < info.numberOfEdges(); i++)
	{
		const Edge &edge = info.getEdge(i);
		Move move = edge.getMove();
		policy.at(move.row, move.col) = edge.getVisits();
		proven_values.at(move.row, move.col) = edge.getProvenValue();
	}
	float r = std::accumulate(policy.begin(), policy.end(), 0.0f);
	if (r > 0.0f)
	{
		r = 1.0f / r;
		for (int i = 0; i < policy.size(); i++)
			policy.data()[i] *= r;
	}

	std::cout << Board::toString(board) << '\n';

	std::string result;
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
						break;
					}
					case ProvenValue::LOSS:
						result += " >L<";
						break;
					case ProvenValue::DRAW:
						result += " >D<";
						break;
					case ProvenValue::WIN:
						result += " >W<";
						break;
				}
			}
			else
				result += (board.at(i, j) == Sign::CROSS) ? "  X " : "  O ";
		}
		result += '\n';
	}
	std::cout << result;

	tree.setEdgeSelector(BestEdgeSelector());
	tree.select(task);
	tree.cancelVirtualLoss(task);

	std::cout << task.toString() << '\n';

	for (int i = 0; i < task.visitedPathLength(); i++)
	{
		if (i < 10 and task.visitedPathLength() >= 10)
			std::cout << " ";
		if (i < 100 and task.visitedPathLength() >= 100)
			std::cout << " ";
		std::cout << i << " : " << task.getPair(i).edge->getMove().toString() << " : " << task.getPair(i).edge->toString() << '\n';
	}

//	task.getLastPair().node->sortEdges();
//	std::cout << task.getLastPair().node->toString() << '\n';
//	for (int i = 0; i < task.getLastPair().node->numberOfEdges(); i++)
//		std::cout << task.getLastPair().node->getEdge(i).getMove().toString() << " : " << task.getLastPair().node->getEdge(i).toString() << '\n';
//	std::cout << '\n';

	std::cout << "END" << std::endl;

//	Board::putMove(board, Move(Sign::CIRCLE, 0, 0));
//	Board::putMove(board, Move(Sign::CROSS, 0, 1));
//
//	std::cout << Board::toString(board) << '\n';
//
//	tree.setBoard(board, Sign::CIRCLE);
//	tree.setEdgeSelector(PuctSelector(1.25f));
//	std::cout << tree.getNodeCacheStats().toString() << '\n';
//
//	search = Search(game_config, search_config);
//	for (int i = 0; i <= 1000; i++)
//	{
//		search.select(tree, 1000);
//		search.tryToSolve();
//
//		search.scheduleToNN(nn_evaluator);
//		nn_evaluator.evaluateGraph();
//
//		search.generateEdges(tree);
//		search.expand(tree);
//		search.backup(tree);
//
//		if (tree.isProven())
//			break;
//	}
//	tree.printSubtree(0, true);
//	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
//	std::cout << tree.getNodeCacheStats().toString() << '\n';
}

void time_manager()
{
//	std::string path = "/home/maciek/Desktop/test_tm/";
//	MasterLearningConfig config(FileLoader(path + "config.json").getJson());
//
//	GeneratorManager manager(config.game_config, config.generation_config);
////	manager.generate(path + "freestyle_10x128.bin", 10000, 0);
//
//	const GameBuffer &buffer = manager.getGameBuffer();
//	buffer.save(path + "buffer_1000f.bin");
//
////	GameBuffer buffer(path + "buffer.bin");
//
//	Json stats(JsonType::Array);
//	matrix<Sign> board(config.game_config.rows, config.game_config.cols);
//
//	for (int i = 0; i < buffer.size(); i++)
//	{
////		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		int nb_moves = Board::numberOfMoves(board);
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			Value value = buffer.getFromBuffer(i).getSample(j).getMinimaxValue();
////			ProvenValue pv = buffer.getFromBuffer(i).getSample(j).getProvenValue();
//
//			Json entry;
//			entry["move"] = nb_moves + j;
//			entry["winrate"] = value.win;
//			entry["drawrate"] = value.draw;
////			entry["proven value"] = toString(pv);
//			stats[i][j] = entry;
//		}
//	}
//
//	FileSaver fs(path + "stats_1000f.json");
//	fs.save(stats, SerializedObject());
//	std::cout << "END" << std::endl;
}

void test_nnue()
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 0; i < 17; i++)
//#else
//	for (int i = 0; i < 1; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator calculator(game_config);
//	nnue::InferenceNNUE incremental(game_config);
//	nnue::InferenceNNUE one_step(game_config);
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	const SearchData &sample = buffer.getFromBuffer(0).getSample(0);
//
////	sample.getBoard(board);
//	const Sign sign_to_move = sample.getMove().sign;
//	calculator.setBoard(board, sign_to_move);
//
//	std::vector<Move> moves;
//	while (moves.size() < 10u)
//	{
//		int x = randInt(game_config.rows);
//		int y = randInt(game_config.cols);
//		if (board.at(x, y) == Sign::NONE)
//		{
//			const Sign sign = moves.empty() ? sign_to_move : invertSign(moves.back().sign);
//			moves.push_back(Move(x, y, sign));
//			board.at(x, y) = sign;
//		}
//	}
////	sample.getBoard(board);
//	calculator.setBoard(board, sign_to_move);
//	incremental.refresh(calculator);
//	one_step.refresh(calculator);
//
//	for (auto iter = moves.begin(); iter < moves.end(); iter++)
//	{
//		std::cout << '\n';
//		calculator.addMove(*iter);
//		incremental.update(calculator);
//
//		one_step.refresh(calculator);
//	}
//	std::cout << '\n';
//	for (auto iter = moves.end() - 1; iter >= moves.begin(); iter--)
//	{
//		std::cout << '\n';
//		calculator.undoMove(*iter);
//		incremental.update(calculator);
//
//		one_step.refresh(calculator);
//	}
//	calculator.addMove(Move(0, 0, sign_to_move));
//	incremental.update(calculator);
//
//	one_step.refresh(calculator);
}

void test_evaluate()
{
	const std::string path = "/home/maciek/alphagomoku/tests_2023/";
	MasterLearningConfig config(FileLoader(path + "config.json").getJson());
	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

	SelfplayConfig cfg(config.evaluation_config.selfplay_options);
	cfg.simulations = 1000;
	manager.setFirstPlayer(cfg, path + "base_2k/checkpoint/network_29_opt.bin", "base_2k_29");

	manager.setSecondPlayer(cfg, path + "base_new_target/checkpoint/network_21_opt.bin", "base_new_target_21");

	const double start = getTime();
	manager.generate(1000);
	const double stop = getTime();
	std::cout << "generated in " << (stop - start) << '\n';

	const std::string to_save = manager.getPGN();
	std::ofstream file(path + "rating.pgn", std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();
}

int main(int argc, char *argv[])
{
	std::cout << "BEGIN" << std::endl;
	std::cout << ml::Device::hardwareInfo() << '\n';

//	run_training();
//	return 0;

//	test_search();
//	test_evaluate();

//	TrainingManager tm("/home/maciek/alphagomoku/tests_2023/test/");
//	for (int i = 0; i < 29; i++)
//		tm.runIterationRL();

	return 0;

	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/train_buffer/buffer_34.bin");
//		for (int i = 0; i < buffer.getGameData(1).numberOfSamples(); i++)
//			buffer.getGameData(1)[i].print();

//		for (int i = 0; i < 100; i++)
//			buffer.getFromBuffer(i).getSample(0).print();
	}

//	test_expand();
//	return 0;

	{
		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/train_buffer/buffer_34.bin");
		std::cout << buffer.getStats().toString() << '\n';

		GameConfig game_config(GameRules::STANDARD, 15, 15);
		TrainingConfig cfg;
		cfg.blocks = 10;
		cfg.filters = 128;
		AGNetwork network; //(game_config, cfg);
//		network.optimize();
//		network.convertToHalfFloats();
		network.loadFromFile("/home/maciek/alphagomoku/new_runs_2023/network_overfit_opt.bin");
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/checkpoint/network_33_opt.bin");
//		network.optimize();
		network.convertToHalfFloats();
		network.setBatchSize(1024);
		network.moveTo(ml::Device::cuda(0));

		matrix<Sign> board(15, 15);
//		board = Board::fromString(""
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" X _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ X _ O X _ _ _ _ _ _ _ _ _ _\n"
//				" _ O X X _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ X O O X _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ X X _ _ _ _ _ _ _ _ _ _ _\n"
//				" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");

//		board = Board::fromString(""
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");

//		board = Board::fromString(" O _ O X X O O X O _ _ X _ _ _\n"
//				" _ X O X O X O X X X O O _ _ _\n"
//				" X O X O X O X O X O X O X _ _\n"
//				" O O X O X O X O X O X O X _ _\n"
//				" O X O X X O O X X O O O X _ _\n"
//				" X X X X O X X X O X X X O _ _\n"
//				" O O X O X O O X X O O X O O _\n"
//				" O O X O X O X O O X O O X X _\n"
//				" O X O X O X O X X O X O X O _\n"
//				" O X O X O X O X O O X O X X _\n"
//				" X O X O X O O X X X O X O O _\n"
//				" X O X O X O X O X X O O X X _\n"
//				" O _ X O O X X O X O X X X O _\n"
//				" _ _ O X O O X X O O X O O _ _\n"
//				" _ X _ _ X X O X O _ O O X _ _\n");
//		network.packInputData(0, board, Sign::CROSS);

//		network.packInputData(0, pack.board, pack.played_move.sign);

		board = Board::fromString(""
		/*         a b c d e f g h i j k l m n o        */
		/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
				/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
				/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
				/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
				/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
				/*  9 */" X _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
				/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */" _ O X O X X O _ _ _ _ _ _ _ _\n" /* 11 */
				/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
				/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
				/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
		/*         a b c d e f g h i j k l m n o        */);

		const Sign sign_to_move = Sign::CIRCLE;
//		network.packInputData(0, board, sign_to_move);

		SearchDataPack pack(15, 15);
		const int game_index = 0;
		const int sample_index = 0;

		matrix<float> policy(15, 15);
		matrix<Value> action_values(15, 15);
		Value value;

		float loss = 0.0f;
		int counter = 0;

//		for (int g = 0; g < buffer.size(); g++)
		for (int g = 0; g < 512; g++)
		{
			for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
			{
				buffer.getGameData(g).getSample(pack, i);
				network.packInputData(i, pack.board, pack.played_move.sign);
			}

			network.forward(buffer.getGameData(g).numberOfSamples());

			for (int i = 0; i < buffer.getGameData(g).numberOfSamples(); i++)
			{
				buffer.getGameData(g).getSample(pack, i);
				network.unpackOutput(i, policy, action_values, value);

				const Value t = convertOutcome(pack.game_outcome, pack.played_move.sign);
				loss += cross_entropy(value.win_rate, t.win_rate);
				loss += cross_entropy(value.draw_rate, t.draw_rate);
				loss += cross_entropy(value.loss_rate(), t.loss_rate());
				counter++;
			}
//			if ((g + 1) % 10 == 0)
//				std::cout << "avg loss = " << loss / counter << " (" << counter << " samples)\n";
		}
		std::cout << "ext loss = " << loss / counter << " (" << counter << " samples)\n";

		return 0;

		network.packInputData(0, pack.board, pack.played_move.sign);
		network.forward(1);
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

//		const double start = getTime();
//		int repeats = 0;
//		for (; repeats < 1000; repeats++)
//		{
//			network.forward(network.getBatchSize());
//			if ((getTime() - start) > 10.0)
//				break;
//		}
//		const double stop = getTime();
//		const double time = stop - start;
//
//		std::cout << "time = " << time << "s, repeats = " << repeats << '\n';
//		std::cout << "n/s = " << network.getBatchSize() * repeats / time << '\n';
//		return 0;

		network.unpackOutput(0, policy, action_values, value);
		std::cout << "\n\n--------------------------------------------------------------\n\n";
		std::cout << "Value = " << value.toString() << '\n'; //<< " (" << value_target.toString() << ")\n";
		//		std::cout << Board::toString(board, policy, true) << '\n';
		//		std::cout << Board::toString(board, action_values, true) << '\n';
//		std::cout << Board::toString(board, policy, true) << '\n';
		std::cout << Board::toString(pack.board, action_values, true) << '\n';

		return 0;
	}

//	run_training();
//	test_evaluate();
//	generate_openings(32);
//	{
//		GameConfig gc(GameRules::STANDARD, 15);
//		TrainingConfig tc;
//		tc.device_config.batch_size = 2;
//		AGNetwork model(gc, tc);
//		model.optimize();
//		matrix<Sign> asdf(15, 15);
//		model.packInputData(0, asdf, Sign::CROSS);
//		model.forward(2);
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

//	for (int i = 0; i < 1; i++)
//	{
//		const SearchData &sample = buffer.getFromBuffer(i).getSample(0);
////		sample.print();
////		return 0;
////		const Sign sign_to_move = sample.getMove().sign;
////		sample.getBoard(board);
//
//		network.packInputData(i, board, sign_to_move);
//	}
//
//	network.forward(1);
//
//	matrix<float> policy(15);
//	matrix<Value> action_values(15);
//	Value value;

//	for (int i = 0; i < 1; i++)
//	{
//		const Value value_target = convertOutcome(buffer.getFromBuffer(i).getSample(0).getOutcome(),
//				buffer.getFromBuffer(i).getSample(0).getMove().sign);
////		const Value minimax = buffer.getFromBuffer(i).getSample(0).getMinimaxValue();
////		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		network.unpackOutput(i, policy, action_values, value);
//		std::cout << '\n';
//		std::cout << Board::toString(board);
//		std::cout << "Network value   = " << value.toString() << " (target = " << value_target.toString() << ")\n";
////		std::cout << "minimax = " << minimax.toString() << "\n";
////		std::cout << "Proven value = " << toString(buffer.getFromBuffer(i).getSample(0).getProvenValue()) << "\n";
//		std::cout << "Network policy:\n" << Board::toString(board, policy) << '\n';
//		std::cout << "Network action values:\n" << Board::toString(board, action_values) << '\n';
////		buffer.getFromBuffer(i).getSample(0).getPolicy(policy);
////		std::cout << "Target:\n" << Board::toString(board, policy) << '\n';
////		std::cout << buffer.getFromBuffer(i).getSample(0).getMove().text() << '\n';
//	}

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

//	{
//		GameConfig game_config(GameRules::STANDARD, 15);
//		AGNetwork network(game_config);
//		network.loadFromFile("/home/maciek/cpp_workspace/AlphaGomoku/Release/networks/minml3v3_10x128.bin");
////		network.moveTo(ml::Device::cuda(1));
//		//	network.moveTo(ml::Device::cpu());
//		network.optimize();
//		network.saveToFile("/home/maciek/cpp_workspace/AlphaGomoku/Release/networks/minml3v3_10x128_opt.bin");
//		return 0;
//	}
//	{
//		std::map<GameRules, std::string> tmp;
//		tmp.insert( { GameRules::FREESTYLE, "/home/maciek/Desktop/freestyle_nnue_32x8x1.bin" });
//		tmp.insert( { GameRules::STANDARD, "/home/maciek/Desktop/standard_nnue_32x8x1.bin" });
//		tmp.insert( { GameRules::RENJU, "/home/maciek/Desktop/standard_nnue_32x8x1.bin" });
//		tmp.insert( { GameRules::CARO5, "/home/maciek/Desktop/standard_nnue_32x8x1.bin" });
//		tmp.insert( { GameRules::CARO6, "/home/maciek/Desktop/standard_nnue_32x8x1.bin" });
//		nnue::NNUEWeights::setPaths(tmp);
//	}
//	test_nnue();
//	test_pattern_calculator();
	test_proven_positions(100);
//	ab_search_test();
//	test_search();
//	test_evaluate();
//	test_search_with_solver(10000);
//	train_simple_evaluation();
//	test_static_solver();
//	test_forbidden_moves();
//	std::cout << "END" << std::endl;
	return 0;
//	experimental::WeightTable::combineAndStore("/home/maciek/Desktop/");

//	GameConfig game_config(GameRules::STANDARD, 10);
//	// @formatter:off
//	matrix<Sign> board = Board::fromString(	/*       0 1 2 3 4 5 6 7 8 9        */
//											/* 0 */" _ _ _ _ _ _ _ _ _ _\n"/* 0 */
//											/* 1 */" _ _ O O O _ _ _ _ _\n"/* 1 */
//											/* 2 */" _ _ _ _ _ _ _ _ _ _\n"/* 2 */
//											/* 3 */" _ _ _ _ _ _ X _ _ O\n"/* 3 */
//											/* 4 */" _ _ _ _ _ _ X _ X _\n"/* 4 */
//											/* 5 */" _ O _ _ _ _ _ _ O _\n"/* 5 */
//											/* 6 */" _ O _ _ _ _ _ _ O _\n"/* 6 */
//											/* 7 */" _ X _ _ _ _ _ _ O _\n"/* 7 */
//											/* 8 */" _ O _ _ X O O O _ _\n"/* 8 */
//											/* 9 */" _ _ _ _ _ _ _ _ _ _\n"/* 9 */
//											/*       0 1 2 3 4 5 6 7 8 9        */);  // @formatter:on
//	// @formatter:off
//	matrix<Sign> board = Board::fromString(	/*       0 1 2 3 4 5 6 7 8 9        */
//											/* 0 */" _ _ _ _ _ _ _ _ _ _\n"/* 0 */
//											/* 1 */" _ _ _ _ _ _ X X X O\n"/* 1 */
//											/* 2 */" _ _ _ _ _ _ _ _ _ _\n"/* 2 */
//											/* 3 */" _ _ _ _ _ O O _ _ _\n"/* 3 */
//											/* 4 */" _ _ _ _ _ O X O _ _\n"/* 4 */
//											/* 5 */" _ _ _ _ _ O _ _ _ _\n"/* 5 */
//											/* 6 */" _ _ _ _ _ X _ _ _ _\n"/* 6 */
//											/* 7 */" _ _ _ _ _ _ _ _ _ _\n"/* 7 */
//											/* 8 */" _ _ _ _ _ _ _ _ _ _\n"/* 8 */
//											/* 9 */" _ _ _ _ _ _ _ _ _ _\n"/* 9 */
//											/*       0 1 2 3 4 5 6 7 8 9        */);  	// @formatter:on
//	// @formatter:off
//	matrix<Sign> board = Board::fromString(	/*       0 1 2 3 4 5 6 7 8 9        */
//											/* 0 */" _ _ _ _ _ _ _ _ _ _\n"/* 0 */
//											/* 1 */" _ _ _ _ _ _ _ _ _ _\n"/* 1 */
//											/* 2 */" _ _ _ _ _ _ _ _ _ _\n"/* 2 */
//											/* 3 */" _ _ _ _ _ _ _ _ _ _\n"/* 3 */
//											/* 4 */" _ X _ O _ _ _ _ _ _\n"/* 4 */
//											/* 5 */" _ _ _ O O _ _ _ _ _\n"/* 5 */
//											/* 6 */" _ _ O _ _ _ _ _ _ _\n"/* 6 */
//											/* 7 */" _ _ O _ X _ _ _ _ _\n"/* 7 */
//											/* 8 */" _ _ _ _ _ X _ _ _ _\n"/* 8 */
//											/* 9 */" _ _ _ _ _ _ O _ _ _\n"/* 9 */
//											/*       0 1 2 3 4 5 6 7 8 9        */);  	// @formatter:on

//	GameConfig game_config(GameRules::STANDARD, 15);
// @formatter:off
//	matrix<Sign> board = Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ O _ _ O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ X X X _ X _ _ _ _ _ _\n"
//			" _ _ _ X _ X X O O _ _ _ _ _ _\n"
//			" _ _ _ O _ O X O _ _ O _ _ _ _\n"
//			" _ _ _ X O X X O O _ X _ _ _ _\n"
//			" _ _ O X O O O X X _ X O _ _ _\n"
//			" _ _ _ _ _ X _ X O O _ _ _ _ _\n"
//			" _ _ _ _ _ _ O X _ X _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");  	// @formatter:on

// @formatter:off
//	matrix<Sign> board = Board::fromString( " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ O _ O _ _ _ _ _ _ _\n"
//			" _ _ _ _ O _ X _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ X _ X _ _ _ _ _ _ _\n"
//			" _ _ _ _ X O X O X _ _ _ _ _ _\n"
//			" _ _ _ X _ O O X _ X _ _ _ _ _\n"
//			" _ _ O _ O X _ X X O O _ _ _ _\n"
//			" _ _ _ _ _ O _ X _ O _ _ _ _ _\n"
//			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");// @formatter:on
//	// @formatter:off
//	matrix<Sign> board = Board::fromString(	/*       0 1 2 3 4 5 6 7 8 9 a b c d e        */
//											/* 0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 0 */
//											/* 1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 1 */
//											/* 2 */" _ X _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 2 */
//											/* 3 */" _ _ O _ _ _ O _ _ _ _ _ _ _ _\n"/* 3 */
//											/* 4 */" _ _ _ X _ _ X _ _ _ _ _ _ _ _\n"/* 4 */
//											/* 5 */" _ _ O _ X O X X _ _ _ _ _ _ _\n"/* 5 */
//											/* 6 */" _ _ _ X X X O X _ _ _ _ _ _ _\n"/* 6 */
//											/* 7 */" _ X O _ O _ O _ _ _ _ _ _ _ _\n"/* 7 */
//											/* 8 */" _ X O O O O X _ _ _ _ _ _ _ _\n"/* 8 */
//											/* 9 */" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"/* 9 */
//											/*10 */" _ _ _ O O _ _ _ _ _ _ _ _ _ _\n"/*10 */
//											/*11 */" _ _ X _ X _ _ _ _ _ _ _ _ _ _\n"/*11 */
//											/*12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*12 */
//											/*13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*13 */
//											/*14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*14 */
//											/*       0 1 2 3 4 5 6 7 8 9 a b c d e        */);  // @formatter:on
// @formatter:off
//	matrix<Sign> board = Board::fromString(	" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//											" _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n"
//											" _ _ _ O _ _ O _ _ _ _ _ _ _ _\n"
//											" _ _ _ _ X X X _ X _ _ _ _ _ _\n"
//											" _ _ _ X _ X X O O _ _ _ _ _ _\n"
//											" _ _ _ O _ O X O _ _ O _ _ _ _\n"
//											" _ _ _ X O X X O O _ X _ _ _ _\n"
//											" _ _ O X O O O X X _ X O _ _ _\n"
//											" _ _ _ _ _ X _ X O O _ _ _ _ _\n"
//											" _ _ _ _ _ _ O X _ X _ _ _ _ _\n"
//											" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
//											" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//											" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//											" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//											" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");  // @formatter:on
//
//	ag::experimental::VCTSolver solver(game_config, 1000);
//	SearchTask task(game_config.rules);
//	task.set(board, Sign::CIRCLE);
//	solver.solve(task, 2);
//	solver.print_stats();
//
//	task.markAsReady();
//	std::cout << task.toString() << std::endl;
//	std::cout << get_BOARD_command(board, Sign::CIRCLE);

//	std::cout << "----------------------------------------------------------------\n";
//	task.set(board, Sign::CROSS);
//	ag::VCTSolver solver_old(game_config, 100000);
//	solver_old.solve(task, 2);
//	std::cout << task.toString() << '\n';
//	solver_old.print_stats();

//	network_only_player("value", argc, argv);
//	ag::experimental::PatternCalculator calc(GameConfig(GameRules::STANDARD, 15, 15));
//	PatternTable table_f(GameRules::FREESTYLE);
//	PatternTable table_s(GameRules::STANDARD);
//	PatternTable table_r(GameRules::RENJU);
//	PatternTable table_c5(GameRules::CARO5);
//	PatternTable table_c6(GameRules::CARO6);
//	PatternTable tt(GameRules::FREESTYLE);
//	return 0;

//	ag::experimental::FastHashing<uint32_t> hashing(15, 15);
//	auto hash = hashing.getHash(board);
//	std::cout << static_cast<uint32_t>(hash) << '\n';
//	hashing.updateHash(hash, Move(0, 0, Sign::CROSS));
//	std::cout << static_cast<uint32_t>(hash) << '\n';
//	hashing.updateHash(hash, Move(0, 0, Sign::CROSS));
//	std::cout << static_cast<uint32_t>(hash) << '\n';
//	hash = 1ull;
//	std::cout << static_cast<uint32_t>(hash) << '\n';
//
//	ag::experimental::LocalHashTable<uint32_t, int8_t, 4> local_table;
//	local_table.insert(hash, 111);
//	std::cout << (int) local_table.get(hash) << std::endl;
//
//	ag::experimental::SharedHashTable<4> shared_table;
//	local_table.insert(hash, 111);
//	std::cout << (int) local_table.get(hash) << std::endl;

//	SearchTask task(GameRules::FREESTYLE);
//	matrix<Sign> board(15, 15);
//	board.at(7, 7) = Sign::CIRCLE;
//	board.at(6, 6) = Sign::CIRCLE;
//	task.set(board, Sign::CROSS);
//	BaseGenerator base_generator;
//	CenterOnlyGenerator generator(7, base_generator);
//	task.markAsReady();
//	generator.generate(task);
//
//	for (auto iter = task.getEdges().begin(); iter < task.getEdges().end(); iter++)
//		board.at(iter->getMove().row, iter->getMove().col) = Sign::CROSS;
//	std::cout << Board::toString(board);
//	return 0;
	return -1;
}
