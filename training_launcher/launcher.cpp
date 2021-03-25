/*
 * launcher.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

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
#include <alphagomoku/utils/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/evaluation/EvaluationManager.hpp>
#include <alphagomoku/utils/ThreadPool.hpp>
#include <libml/graph/Graph.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>

using namespace ag;

void test_train()
{
	Json config = FileLoader("/home/maciek/alphagomoku/standard_2021/config.json").getJson();

	SupervisedLearning sl(config);
	AGNetwork model(config);
	//	AGNetwork model(std::string("/home/maciek/alphagomoku/standard_2021/network.bin"));
	GameBuffer buffer;
	for (int i = 80; i < 100; i++)
		buffer.load("/home/maciek/alphagomoku/standard_2021/train_buffer/buffer_" + std::to_string(i) + ".zip");

	sl.saveTrainingHistory();
	for (int i = 0; i < 50; i++)
	{
		sl.train(model, buffer, 1000);
		sl.saveTrainingHistory();
	}
	model.changeLearningRate(1.0e-4);
	for (int i = 0; i < 30; i++)
	{
		sl.train(model, buffer, 1000);
		sl.saveTrainingHistory();
	}
	model.changeLearningRate(1.0e-5);
	for (int i = 0; i < 20; i++)
	{
		sl.train(model, buffer, 1000);
		sl.saveTrainingHistory();
	}
	std::cout << "training finished\n";

	model.getGraph().moveTo(ml::Device::cpu());
	{
		SerializedObject so;
		Json json = model.getGraph().save(so);
		FileSaver fs("/home/maciek/alphagomoku/standard_2021/network_5x64.bin");
		fs.save(json, so, 2);
	}
	std::cout << "model saved\n";

	model.optimize();
	std::cout << "model optimized\n";
	{
		SerializedObject so;
		Json json = model.getGraph().save(so);
		FileSaver fs("/home/maciek/alphagomoku/standard_2021/network_5x64_opt.bin");
		fs.save(json, so, 2);
	}
	std::cout << "optimized model saved\n";

	matrix<Sign> board(15, 15);
	matrix<float> policy(15, 15);
	Sign sign_to_move;
	GameOutcome outcome;

	int batch_size = buffer.getFromBuffer(0).length();
	for (int i = 0; i < batch_size; i++)
	{
		buffer.getFromBuffer(0).getSample(board, policy, sign_to_move, outcome, i);
		buffer.getFromBuffer(0).printSample(i);

		model.packData(0, board, policy, outcome, sign_to_move);
		model.forward(1);

		buffer.getFromBuffer(0).getSample(board, policy, sign_to_move, outcome, i);
		policy.clear();
		float value = model.unpackOutput(0, policy);
		std::cout << "-----------------------------------------------------------\n";
		std::cout << "value = " << value << '\n';
		std::cout << policyToString(board, policy);
		std::cout << "-----------------------------------------------------------\n\n";
	}
}

int main()
{
	FileLoader fl("/home/maciek/alphagomoku/standard_2021/simple_config.json");
//	EvaluationManager manager(fl.getJson());
//	Json config = fl.getJson()["evaluation_options"];
//	manager.setCrossPlayer(config, "/home/maciek/alphagomoku/standard_2021/network_5x64_opt.bin");
//	config["simulations"] = 100;
//	manager.setCirclePlayer(config, "/home/maciek/alphagomoku/standard_2021/network_5x64_opt.bin");

//	manager.generate(1000);
//	std::string to_save = manager.getGameBuffer().generatePGN("AG_800", "AG_100");
//	std::ofstream file("/home/maciek/alphagomoku/standard_2021/games.pgn");
//	file.write(to_save.data(), to_save.size());
//	file.close();

	GeneratorManager manager(fl.getJson());
	manager.generate("/home/maciek/alphagomoku/standard_2021/network_5x64_opt.bin", 2000);
//	manager.generate("/home/maciek/alphagomoku/standard_2021/network_10x128/network_opt.bin", 200);

	return 0;

	GameConfig game_config;
	game_config.rules = GameRules::STANDARD;
	game_config.rows = 15;
	game_config.cols = 15;

	TreeConfig tree_config;
	tree_config.max_number_of_nodes = 100000000;
	tree_config.bucket_size = 1000000;
	Tree tree(tree_config);

	CacheConfig cache_config;
	cache_config.min_cache_size = 1024576;
	cache_config.update_from_search = false;
	Cache cache(game_config, cache_config);

	EvaluationQueue queue;
	queue.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64_opt.bin", 32, ml::Device::cuda(0));

	SearchConfig search_config;
	search_config.batch_size = 4;
	search_config.exploration_constant = 1.25f;
	search_config.noise_weight = 0.0f;
	search_config.expansion_prior_treshold = 1.0e-6f;
	search_config.augment_position = false;
	search_config.use_endgame_solver = false;

	Search search(game_config, search_config, tree, cache, queue);

	Sign sign_to_move = Sign::CIRCLE;
	matrix<Sign> board(15, 15);

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
//	board = boardFromString(" X X O X X X O X O X X X O X O\n"
//							" X X O X O X O O X X X O X X X\n"
//							" X O O X O X X O X O X X O O O\n"
//							" O X X O X X O O X O O X X X O\n"
//							" X X O X O O O X X O X O O O O\n"
//							" X X O X O X O O O X O X X X X\n"
//							" O X O X X X X O O O O X X O X\n"
//							" O O X O O X X O X O O X X O O\n"
//							" X X X O X O O O O X X X O O X\n"
//							" O O O X O X X X X O O O O X X\n"
//							" O X O O O O X O O X X O O X O\n"
//							" X X O X X X X O O O X X X O O\n"
//							" X X O O X _ O X _ _ X O _ _ X\n"
//							" O O X O X X O O X _ X X O O _\n"
//							" X X X O X _ O X _ O O X O _ O\n");
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

	board.at(4, 5) = Sign::CROSS;
	board.at(6, 5) = Sign::CIRCLE;
	board.at(1, 2) = Sign::CROSS;
	board.at(6, 6) = Sign::CIRCLE;
	board.at(3, 3) = Sign::CROSS;
	board.at(5, 6) = Sign::CIRCLE;
	board.at(3, 8) = Sign::CROSS;
//	board.at(7, 6) = Sign::CIRCLE;
	tree.getRootNode().setMove( { 0, 0, invertSign(sign_to_move) });
	search.setBoard(board);

	matrix<float> policy(15, 15);
	for (int i = 0; i <= 10; i++)
	{
		while (search.getSimulationCount() < i * 100000)
		{
			search.simulate(i * 100000);
			queue.evaluateGraph();
			search.handleEvaluation();
			if (tree.isProven())
				break;
		}
		tree.getPlayoutDistribution(tree.getRootNode(), policy);
		normalize(policy);

		std::cout << tree.getPrincipalVariation().toString() << '\n';
		std::cout << policyToString(board, policy);
		std::cout << queue.getStats().toString();
		std::cout << search.getStats().toString();
		std::cout << tree.getStats().toString();
		std::cout << cache.getStats().toString() << '\n';
		if (tree.isProven())
			break;
	}
	std::cout << "\n----------------------------------------------------------------------------------\n";
	tree.printSubtree(tree.getRootNode(), 1, true);
	std::cout << "\n----------------------------------------------------------------------------------\n";

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

