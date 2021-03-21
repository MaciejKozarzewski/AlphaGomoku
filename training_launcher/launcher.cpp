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
#include <libml/graph/Graph.hpp>
#include <libml/hardware/Device.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>
#include <iostream>
#include <string>

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
	Tree tree(5000000, 100000);
	Cache cache(15, 15, 8192);
	EvaluationQueue queue;
	queue.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64_opt.bin");

	SearchConfig cfg;
	cfg.rows = 15;
	cfg.cols = 15;
	cfg.rules = GameRules::STANDARD;
	cfg.batch_size = 4;
	cfg.exploration_constant = 1.25f;
	cfg.noise_weight = 0.0f;
	cfg.expansion_prior_treshold = 1.0e-4f;
	cfg.augment_position = false;
	cfg.use_endgame_solver = false;

	Search search(cfg, tree, cache, queue);

	Sign sign_to_move = Sign::CIRCLE;
	matrix<Sign> board(15, 15);
	board.at(4, 5) = invertSign(sign_to_move);
	board.at(6, 5) = sign_to_move;
	board.at(3, 2) = invertSign(sign_to_move);
	tree.getRootNode().setMove( { 0, 0, invertSign(sign_to_move) });
	search.setBoard(board);

	matrix<float> policy(15, 15);
	for (int i = 0; i <= 10000; i++)
	{
		if (i % 100 == 0)
		{
			tree.getPlayoutDistribution(tree.getRootNode(), policy);
			normalize(policy);
			std::cout << policyToString(board, policy) << '\n';
		}
		search.iterate(10000);
		queue.evaluateGraph();
		search.handleEvaluation();
	}

//	std::cout << "\n----------------------------------------------------------------------------------\n";
//	tree.printSubtree(tree.getRootNode(), 2, true, 10);
//	std::cout << "\n----------------------------------------------------------------------------------\n";
	std::cout << tree.getPrincipalVariation().toString() << '\n';
	std::cout << policyToString(board, policy);
	std::cout << search.getStats().toString();
	std::cout << queue.getStats().toString();
	std::cout << tree.usedNodes() << "/" << tree.allocatedNodes() << " nodes used\n";
	std::cout << cache.storedElements() << ":" << cache.bufferedElements() << ":" << cache.allocatedElements() << '\n';
	Move move = pickMove(policy);
	move.sign = sign_to_move;
	std::cout << "making move " << move.toString() << '\n';

	board.at(move.row, move.col) = move.sign;

	cache.cleanup(board, true, 10);
	std::cout << cache.storedElements() << ":" << cache.bufferedElements() << ":" << cache.allocatedElements() << '\n';

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

