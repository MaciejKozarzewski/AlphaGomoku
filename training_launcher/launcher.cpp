/*
 * launcher.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>
#include <alphagomoku/vcf_solver/FeatureTable.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/mcts/Move.hpp>
#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/selfplay/AGNetwork.hpp>
#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/evaluation/EvaluationManager.hpp>
#include <alphagomoku/utils/ThreadPool.hpp>
#include <libml/graph/Graph.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <istream>
#include <thread>

using namespace ag;

int main(int argc, char *argv[])
{
	std::cout << "Compiled on " << __DATE__ << " at " << __TIME__ << std::endl;
	std::cout << ml::Device::hardwareInfo() << '\n';

	std::string path = (argc == 2) ? argv[1] : "/home/maciek/alphagomoku/freestyle_20x20/";

//	TrainingManager tm(path);
//	for (int i = 0; i < 200; i++)
//		tm.runIterationRL();
//	return 0;

	GameConfig game_config;
	game_config.rules = GameRules::STANDARD;
	game_config.rows = 15;
	game_config.cols = 15;

	TreeConfig tree_config;
	tree_config.max_number_of_nodes = 500000000;
	tree_config.bucket_size = 1000000;
	Tree tree(tree_config);

	CacheConfig cache_config;
	cache_config.min_cache_size = 1024576;
	cache_config.update_from_search = false;
	Cache cache(game_config, cache_config);

	SearchConfig search_config;
	search_config.batch_size = 8;
	search_config.exploration_constant = 1.25f;
	search_config.noise_weight = 0.0f;
	search_config.expansion_prior_treshold = 1.0e-4f;
	search_config.max_children = 30;
	search_config.use_endgame_solver = true;
	search_config.use_vcf_solver = true;

	ml::Device::cpu().setNumberOfThreads(1);
	EvaluationQueue queue;
//	queue.loadGraph("/home/maciek/alphagomoku/test_10x10_standard/checkpoint/network_65_opt.bin", 32, ml::Device::cuda(0));
//	queue.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64wdl_opt.bin", 32, ml::Device::cuda(0));
	queue.loadGraph("/home/maciek/repos/AlphaGomoku/Release/networks/standard_6x64.bin", 8, ml::Device::cpu(), true);

	Search search(game_config, search_config, tree, cache, queue);

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
//					" _ _ _ _ _ _ _ _ O _ X _ _ _ _\n"// 1
//					" _ _ _ _ _ _ _ _ _ X O O O _ _\n"// 2
//					" _ _ _ _ _ _ _ _ _ X O _ _ _ _\n"// 3
//					" _ _ _ _ _ _ X _ O O O X O _ _\n"// 4
//					" _ _ _ _ _ _ _ _ X O X X X X O\n"// 5
//					" _ _ _ _ _ _ _ _ X O X _ _ _ _\n"// 6
//					" _ _ _ _ _ _ _ _ _ _ X _ X _ _\n"// 7
//					" _ _ _ _ _ _ _ _ _ X O O _ _ _\n"// 8
//					" _ _ _ _ _ _ _ _ O X O _ _ _ _\n"// 9
//					" _ _ _ _ _ _ _ _ _ O X _ _ _ _\n"// 10
//					" _ _ _ _ _ _ _ _ O _ O _ _ _ _\n"// 11
//					" _ _ _ _ _ _ _ X _ _ _ X _ _ _\n"// 12
//					" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"// 13
//					" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");// 14
//	sign_to_move = Sign::CIRCLE;

//	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ X _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ X _ O _ X _ O _ _ _\n"
//			" _ _ _ _ _ _ O O X _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n");
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
//							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ X O O O _ _ O _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ X X X O _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ O X O O X _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ O X X O O _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ O O X X _ O _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ O X X X _ O _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ O X _ X _ _ _ _ _ _ _ _ _ _ _\n"
//							" _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n");
//	sign_to_move = Sign::CROSS;

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

	board = boardFromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
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
							" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
	sign_to_move = Sign::CROSS;

	FeatureExtractor extractor(game_config);
	extractor.setBoard(board, sign_to_move);
	extractor.printAllThreats();

//	std::vector<std::pair<uint16_t, float>> list_of_moves;
//	matrix<float> qwer(board.rows(), board.cols());
//	double t0 = getTime();
//	for (int i = 0; i < 1000; i++)
//	{
//	ProvenValue asdf = extractor.solve(qwer, list_of_moves);
//	}
//	std::cout << "time = " << (getTime() - t0) << "ms\n";
//	std::cout << toString(asdf) << '\n';
//	return 0;

	double start = getTime();
	tree.getRootNode().setMove( { 0, 0, invertSign(sign_to_move) });
	search.setBoard(board);

	matrix<float> policy(board.rows(), board.cols());
	for (int i = 0; i <= 1; i++)
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
}

