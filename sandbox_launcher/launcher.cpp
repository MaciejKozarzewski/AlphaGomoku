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
#include <alphagomoku/dataset/Sampler.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/networks/networks.hpp>
#include <alphagomoku/patterns/DefensiveMoveTable.hpp>
#include <alphagomoku/patterns/ThreatTable.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
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
#include <alphagomoku/utils/SpinLock.hpp>
#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/utils/selfcheck.hpp>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/networks/networks.hpp>
#include <alphagomoku/version.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/alpha_beta/MoveGenerator.hpp>
#include <alphagomoku/search/alpha_beta/MinimaxSearch.hpp>
#include <alphagomoku/search/alpha_beta/AlphaBetaSearch.hpp>
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
#include <alphagomoku/networks/NNUE.hpp>

#include <minml/utils/ZipWrapper.hpp>
#include <alphagomoku/search/alpha_beta/ThreatSpaceSearch.hpp>

#include "minml/core/Device.hpp"
#include "minml/utils/json.hpp"
#include "minml/utils/serialization.hpp"
#include "minml/layers/Dense.hpp"
#include "minml/layers/BatchNormalization.hpp"
#include <minml/graph/graph_optimizers.hpp>
#include <minml/utils/testing_util.hpp>

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

std::vector<std::vector<Move>> generate_openings(size_t number, GameConfig game_config, const std::string pathToNetwork)
{
	DeviceConfig device_config;
	device_config.batch_size = 256;
	device_config.device = ml::Device::cuda(0);

	NNEvaluator evaluator(device_config);
	evaluator.loadGraph(pathToNetwork);

	TSSConfig tss_config;
	AlphaBetaSearch solver(game_config);

	std::vector<std::vector<Move>> result;

	OpeningGenerator generator(game_config, 10);
	while (result.size() < number)
	{
		generator.generate(device_config.batch_size, evaluator, solver);
		evaluator.evaluateGraph();

		while (not generator.isEmpty() and result.size() < number)
			result.push_back(generator.pop());
	}

	return result;
}

void run_training()
{
	ml::Device::setNumberOfThreads(1);

	Dataset dataset;
	for (int i = 225; i < 250; i++)
		dataset.load(i, "/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/standard_old/train_buffer/buffer_0.bin");
//	GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/test_old/train_buffer/buffer_0.bin");
//	buffer.load("/home/maciek/alphagomoku/new_runs_2023/test_old/valid_buffer/buffer_0.bin");
	std::cout << dataset.getStats().toString() << '\n';

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.network_arch = "BottleneckPoolingPVraw";
	training_config.augment_training_data = true;
	training_config.blocks = 8;
	training_config.filters = 128;
	training_config.device_config.batch_size = 256;
	training_config.l2_regularization = 5.0e-5f;

	std::unique_ptr<AGNetwork> network = createAGNetwork(training_config.network_arch);
	network->init(game_config, training_config);
	network->moveTo(ml::Device::cuda(0));
	network->changeLearningRate(1.0e-3f);

	SupervisedLearning sl(training_config);

	const std::string path = "/home/maciek/alphagomoku/new_runs_2024/supervised/pooling_btl_v1_8x128s/";
	for (int e = 0; e < 100; e++)
	{
		if (e == 60)
			network->changeLearningRate(1.0e-4f);
		if (e == 80)
			network->changeLearningRate(1.0e-5f);
		sl.clearStats();
		sl.train(*network, dataset, 1000);
		sl.saveTrainingHistory(path);
		network->saveToFile(path + "/network_" + std::to_string(e) + ".bin");

		std::unique_ptr<AGNetwork> tmp = loadAGNetwork(path + "/network_" + std::to_string(e) + ".bin");
		tmp->optimize();
		tmp->saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
	}
}

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
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::STANDARD, 15);

	PatternTable::get(game_config.rules);
	ThreatTable::get(game_config.rules);
	DefensiveMoveTable::get(game_config.rules);

	GameDataBuffer buffer;
#ifdef NDEBUG
	for (int i = 200; i <= 224; i++)
#else
	for (int i = 224; i <= 224; i++)
#endif
		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/valid_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	PatternCalculator extractor_new(game_config);
	PatternCalculator extractor_new2(game_config);

	SearchDataPack pack(game_config.rows, game_config.cols);

	int count = 0;
	for (int i = 0; i < buffer.numberOfGames(); i++)
	{
		if (i % (buffer.numberOfGames() / 10) == 0)
			std::cout << i << " / " << buffer.numberOfGames() << '\n';

		for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
		{
			buffer.getGameData(i).getSample(pack, j);
			extractor_new.setBoard(pack.board, pack.played_move.sign);

//			if (extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::FIVE).size() > 0
//					and (extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::OPEN_4).size() > 0
//							or extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::FORK_4x4).size() > 0))
//				count++;
//			if (extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::FIVE).size() > 0
//					and (extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::OPEN_4).size() > 0
//							or extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::FORK_4x4).size() > 0))
//				count++;

			for (int k = 0; k < 100; k++)
			{
				const int x = randInt(game_config.rows);
				const int y = randInt(game_config.cols);
				if (pack.board.at(x, y) == Sign::NONE)
				{
					Sign sign = static_cast<Sign>(randInt(1, 3));
					extractor_new.addMove(Move(x, y, sign));
					pack.board.at(x, y) = sign;
				}
				else
				{
					extractor_new.undoMove(Move(x, y, pack.board.at(x, y)));
					pack.board.at(x, y) = Sign::NONE;
				}
			}

//			extractor_new2.setBoard(board, sample.getMove().sign);
//			for (int x = 0; x < game_config.rows; x++)
//				for (int y = 0; y < game_config.cols; y++)
//				{
//					for (Direction dir = 0; dir < 4; dir++)
//					{
//						if (extractor_new2.getNormalPatternAt(x, y, dir) != extractor_new.getNormalPatternAt(x, y, dir))
//						{
//							std::cout << "Raw pattern mismatch\n";
//							std::cout << "Single step\n";
//							extractor_new2.printRawFeature(x, y);
//							std::cout << "incremental\n";
//							extractor_new.printRawFeature(x, y);
//							exit(-1);
//						}
//						if (extractor_new2.getPatternTypeAt(Sign::CROSS, x, y, dir) != extractor_new.getPatternTypeAt(Sign::CROSS, x, y, dir)
//								or extractor_new2.getPatternTypeAt(Sign::CIRCLE, x, y, dir)
//										!= extractor_new.getPatternTypeAt(Sign::CIRCLE, x, y, dir))
//						{
//							std::cout << "Pattern type mismatch\n";
//							std::cout << "Single step\n";
//							extractor_new2.printRawFeature(x, y);
//							std::cout << "incremental\n";
//							extractor_new.printRawFeature(x, y);
//							exit(-1);
//						}
//					}
//					if (extractor_new2.getThreatAt(Sign::CROSS, x, y) != extractor_new.getThreatAt(Sign::CROSS, x, y)
//							or extractor_new2.getThreatAt(Sign::CIRCLE, x, y) != extractor_new.getThreatAt(Sign::CIRCLE, x, y))
//					{
//						std::cout << "Threat type mismatch\n";
//						std::cout << "Single step\n";
//						extractor_new2.printThreat(x, y);
//						std::cout << "incremental\n";
//						extractor_new.printThreat(x, y);
//						exit(-1);
//					}
//
//					for (int i = 0; i < 10; i++)
//					{
//						if (extractor_new2.getThreatHistogram(Sign::CROSS).get((ThreatType) i).size()
//								!= extractor_new.getThreatHistogram(Sign::CROSS).get((ThreatType) i).size())
//						{
//							std::cout << "Threat histogram mismatch for cross\n";
//							std::cout << "Single step\n";
//							extractor_new2.getThreatHistogram(Sign::CROSS).print();
//							std::cout << "incremental\n";
//							extractor_new.getThreatHistogram(Sign::CROSS).print();
//							exit(-1);
//						}
//						if (extractor_new2.getThreatHistogram(Sign::CIRCLE).get((ThreatType) i).size()
//								!= extractor_new.getThreatHistogram(Sign::CIRCLE).get((ThreatType) i).size())
//						{
//							std::cout << "Threat histogram mismatch for circle\n";
//							std::cout << "Single step\n";
//							extractor_new2.print();
//							extractor_new2.getThreatHistogram(Sign::CIRCLE).print();
//							std::cout << "incremental\n";
//							extractor_new.print();
//							extractor_new.getThreatHistogram(Sign::CIRCLE).print();
//							exit(-1);
//						}
//					}
//				}
		}
	}

	std::cout << "count  = " << count << '\n';
	std::cout << "New extractor\n";
	extractor_new.print_stats();
}

void test_move_generator()
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::CARO5, 15);
	game_config.draw_after = 200;

	PatternTable::get(game_config.rules);
	ThreatTable::get(game_config.rules);
	DefensiveMoveTable::get(game_config.rules);

//	GameDataBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 200; i <= 200; i++)
//#else
//	for (int i = 200; i <= 200; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';

	PatternCalculator calculator(game_config);
	MoveGenerator generator(game_config, calculator);

	SearchDataPack pack(game_config.rows, game_config.cols);

	ActionStack action_stack(1024);

//	AlphaBetaSearch search(game_config, MoveGeneratorMode::REDUCED);
	TSSConfig tss_cfg;
	tss_cfg.mode = 2;
	tss_cfg.max_positions = 1000000;
	ThreatSpaceSearch search(game_config, tss_cfg);

	SearchTask task(game_config);

	matrix<Sign> board(game_config.rows, game_config.cols);
	Sign sign_to_move;

	double total_nodes = 0;
	double total_samples = 0;
	for (int b = 200; b <= 200; b++)
//	int b = 200;
	{
		GameDataBuffer buffer;
		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128c5/train_buffer/buffer_" + std::to_string(b) + ".bin");
		std::cout << buffer.getStats().toString() << '\n';
		total_samples += buffer.numberOfSamples();

		for (int i = 0; i < buffer.numberOfGames(); i++)
//		int i = 9;
		{
			if (i % (buffer.numberOfGames() / 10) == 0)
				std::cout << i << " / " << buffer.numberOfGames() << '\n';

			for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
//			int j = 30;
			{
				buffer.getGameData(i).getSample(pack, j);
				calculator.setBoard(pack.board, pack.played_move.sign);

				ActionList actions(action_stack);
				const Score static_score = generator.generate(actions, MoveGeneratorMode::OPTIMAL);
				total_nodes += actions.size();

//				if (actions.must_defend)
//				{
//					const Score base_score = actions.baseline_score;
//					const int depth = base_score.getDistance();
//					std::cout << "checking " << base_score.toString() << '\n';
//
//					task.set(pack.board, pack.played_move.sign);
//					search.setDepthLimit(depth);
//					search.setNodeLimit(1000000);
//					search.solve(task);
//
//					for (int row = 0; row < task.getBoard().rows(); row++)
//						for (int col = 0; col < task.getBoard().cols(); col++)
//							if (task.getBoard().at(row, col) == Sign::NONE)
//							{
//								const Move m(row, col, task.getSignToMove());
//								if (not task.getActionScores().at(row, col).isLoss())
//								{
//									const bool is_found = actions.contains(m);
//									const Score sc = actions.getScoreOf(m);
//									if (sc.isLoss() or not is_found)
//									{
//										std::cout << "b = " << 200 << ", i=" << i << ", j=" << j << '\n';
//										std::cout << "MISSING MOVE " << m.toString() << " " << m.text() << '\n';
//										std::cout << m.text() << " " << task.getActionScores().at(row, col) << " "
//												<< (actions.contains(m) ? actions.getScoreOf(m).toString() : "N/A") << '\n';
//										std::cout << task.toString() << '\n';
//										calculator.print();
//										calculator.printAllThreats();
//										std::cout << "\nGenerated actions\n";
//										actions.print();
//										std::cout << "True non-losing moves\n";
//										for (int r = 0; r < task.getBoard().rows(); r++)
//											for (int c = 0; c < task.getBoard().cols(); c++)
//												if (task.getBoard().at(r, c) == Sign::NONE and not task.getActionScores().at(r, c).isLoss())
//													std::cout << Move(r, c, task.getSignToMove()).text() << '\n';
//										return;
//									}
//								}
//							}
//				}
//				if (static_score.isWin())
//				if (static_score == Score::win_in(7))
//				{
//					const int depth = static_score.getDistance();
//					std::cout << "checking " << static_score.toString() << '\n';
//
//					task.set(pack.board, pack.played_move.sign);
//					search.solve(task, TssMode::RECURSIVE, 1000000);
//					std::cout << task.toString() << '\n';
//					if(task.getScore() != static_score)
////						exit(-1);
////					for (auto iter = actions.begin(); iter < actions.end(); iter++)
////						if (task.getActionScores().at(iter->move.row, iter->move.col) < iter->score)
//						{
//							std::cout << "b = " << 200 << ", i=" << i << ", j=" << j << '\n';
////							std::cout << "FALSE WINNING MOVE " << iter->move.toString() << " " << iter->move.text() << '\n';
//							std::cout << task.toString() << '\n';
//							calculator.print();
//							calculator.printAllThreats();
//							std::cout << "\nGenerated actions\n";
//							actions.print();
//							return;
//						}
//				}

//			if (static_score == Score::win_in(5))
//			{
//				const int depth = static_score.getDistance();
//				task.set(pack.board, pack.played_move.sign);
//				search.solve(task, depth, 100000000);
//				for (auto iter = actions.begin(); iter < actions.end(); iter++)
//				{
//					const Score exact_score = task.getActionScores().at(iter->move.row, iter->move.col);
//					if (iter->score.isProven() and iter->score != exact_score)
//					{
//						std::cout << task.toString() << '\n';
//						actions.print();
//						return;
//					}
//				}
//
//			}
			}
		}
	}
	std::cout << "avg nodes  = " << total_nodes / total_samples << '\n';
//	calculator.print_stats();
}

void test_proven_positions(int pos)
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::RENJU, 15);
//	GameConfig game_config(GameRules::CARO5, 15);
//	GameConfig game_config(GameRules::CARO6, 20);

	PatternTable::get(game_config.rules);
	ThreatTable::get(game_config.rules);
	DefensiveMoveTable::get(game_config.rules);

	GameDataBuffer buffer("/home/maciek/alphagomoku/proven_positions_big.bin");
//	GameDataBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 200; i <= 200; i++)
//#else
//	for (int i = 200; i <= 200; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	AlphaBetaSearch ab_search(game_config);
	ab_search.loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
	ab_search.setNodeLimit(pos);

	TSSConfig tss_config;
	ThreatSpaceSearch ts_search(game_config, tss_config);
	ts_search.loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));

	SearchDataPack pack(game_config.rows, game_config.cols);
	SearchTask task(game_config);
	matrix<Sign> board(game_config.rows, game_config.cols);
	int total_samples = 0;
	int tss_solved_count = 0, ab_solved_count = 0;
	int total_positions = 0;

	std::map<Score, std::pair<int, int>> score_stats;

#ifndef NDEBUG
	for (int i = 0; i < buffer.numberOfGames(); i += 100)
#else
	for (int i = 0; i < buffer.numberOfGames(); i += 100)
#endif
//	int i = 102600;
	{
		if (i % (buffer.numberOfGames() / 10) == 0)
			std::cout << i << " / " << buffer.numberOfGames() << " " << ab_solved_count << "/" << total_samples << '\n';

		total_samples += buffer.getGameData(i).numberOfSamples();
		for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
//		int j = 0;
		{
//			std::cout << i << " " << j << '\n';
			buffer.getGameData(i).getSample(pack, j);

//			task.set(pack.board, pack.played_move.sign);
//			ts_search.solve(task, TssMode::RECURSIVE, pos);
//			tss_solved_count += task.getScore().isProven();

			task.set(pack.board, pack.played_move.sign);

			const int positions = ab_search.solve(task);
//			vcf_search.setDepthLimit(4);
//			vcf_search.setNodeLimit(pos);
//			const int positions = vcf_search.solve(task);
			total_positions += positions;
			ab_solved_count += task.getScore().isProven();
//			if (task.getScore().isUnproven())
//			{
//				vcf_search.setDepthLimit(100);
//				vcf_search.setNodeLimit(pos - positions);
//				total_positions += vcf_search.solve(task);
//				ab_solved_count += task.getScore().isProven();
//			}
			if (task.getScore().isProven())
			{
				auto iter = score_stats.find(task.getScore());
				if (iter == score_stats.end())
					score_stats.insert( { task.getScore(), { 1, positions } });
				else
				{
					iter->second.first++;
					iter->second.second += positions;
				}
			}
//			return;
		}
	}

	for (auto iter = score_stats.begin(); iter != score_stats.end(); iter++)
		std::cout << iter->first.toString() << " : " << iter->second.first << " (" << (100.0 * iter->second.first / total_samples)
				<< "%), avg positions = " << (double) iter->second.second / iter->second.first << '\n';

//	std::cout << "TSS\n";
	ab_search.print_stats();
//	std::cout << "tss solved " << tss_solved_count << " samples (" << 100.0f * tss_solved_count / total_samples << "%)\n";
	std::cout << "a-b solved " << ab_solved_count << " samples (" << 100.0f * ab_solved_count / total_samples << "%)\n";
//	ts_search.print_stats();
	std::cout << "total_positions = " << total_positions << ", avg = " << total_positions / total_samples << '\n';
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
//	GameConfig game_config(GameRules::STANDARD, 15);
	GameConfig game_config(GameRules::STANDARD, 15);

	SearchConfig search_config;
	search_config.max_batch_size = 8;
	search_config.mcts_config.max_children = 32;
	search_config.mcts_config.policy_expansion_threshold = 1.0e-4f;
	search_config.tss_config.mode = 2;
	search_config.tss_config.max_positions = 1000;
	search_config.tss_config.hash_table_size = 1048576;

	Tree tree(search_config.tree_config);

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
//#ifdef NDEBUG
	device_config.device = ml::Device::cpu();
//#else
//	device_config.device = ml::Device::cpu();
//#endif
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
//	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku550/networks/network_57_opt.bin");
//	nn_evaluator.loadGraph("./old_6x64s.bin");
	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_255_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs/btl_pv_8x128f/checkpoint/network_242_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs/standard_15x15/checkpoint/network_0_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/tl/checkpoint/network_99_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/test_6x64s/checkpoint/network_70_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/tests_2023/base_q_head/checkpoint/network_15_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/cpp_workspace/AlphaGomoku/Release/networks/network_44_opt.bin");

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

//// @formatter:off
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

//// @formatter:off
//	board = Board::fromString(	" O _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//								" X O _ _ X _ _ _ _ _ _ _ _ _ _\n"
//								" X X _ _ _ _ O _ _ _ X _ _ _ _\n"
//								" X O _ _ O X O _ O O _ _ _ _ _\n"
//								" X O _ O X X X X O _ _ _ _ _ _\n"
//								" O X X O X O O O _ X _ _ _ _ _\n"
//								" _ X O X O X X X O O _ O _ _ _\n"
//								" _ _ O X O O X O X _ _ _ _ _ _\n"
//								" _ _ _ O X X O X _ _ _ O _ _ _\n"
//								" _ _ _ _ O X O X O _ X X _ _ _\n"
//								" _ _ _ _ _ X X X O _ _ O _ _ _\n"
//								" _ _ _ _ O _ _ O X _ _ _ _ _ _\n"
//								" _ _ _ _ _ O _ _ _ X X _ _ _ _\n"
//								" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
//								" _ _ _ _ _ _ _ _ _ _ _ O _ _ _\n"); // Ok3 is w win
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ O O _ _ _ _ _ _ _ _\n"
//									" _ _ X _ O _ O O X _ _ _ _ _ _\n"
//									" _ _ _ O _ X _ O _ _ _ O _ _ _\n"
//									" _ _ _ X O _ X X X O X _ _ _ _\n"
//									" _ _ _ O X O O O O X X _ _ _ _\n"
//									" _ _ _ _ _ X O X X _ X _ _ _ _\n"
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
//// @formatter:off
//	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ X _ _ _ ! _ _ _ _ _ _ _ _\n"
//									" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ X O _ X _ _ _ _ _ _ _ _\n"
//									" _ _ _ O X O O O _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"
//									" _ _ _ _ _ X O X _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//								/*    a b c d e f g h i j k l m n o          */); // position is a win for circle
//// @formatter:on
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
// @formatter:off
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
			/*  1 */ " _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n" /*  1 */
			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
			/*  4 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  4 */
			/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
			/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
			/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
			/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
			/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
			/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
			/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
			/*         a b c d e f g h i j k l m n o        */);
// @formatter:on
	sign_to_move = Sign::CROSS;

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

////// @formatter:off
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ X X _ X _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ X _ X O O O X O _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ O _ O X O O O _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ _ _ O X X X _ O _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ X O O _ X _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ _ _ _ O O X _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ _ _ _ _ X X _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ X X _ X _ _ _ _\n"
////////			" _ _ _ X _ X O O O X O _ _ _ _\n"
////////			" _ _ _ _ O _ O X O O O _ _ _ _\n"
////////			" _ _ _ _ _ O X X X _ O _ _ _ _\n"
////////			" _ _ _ _ _ X O O _ X _ _ _ _ _\n"
////////			" _ _ _ _ _ _ O O X _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ X X _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
////////			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ X O _ _ X _ _ _\n"
//			" _ _ _ _ _ _ O X X X O _ _ _ _\n"
//			" _ _ _ _ _ O X O O O X _ _ _ _\n"
//			" _ _ _ X O _ X O O _ X O _ _ _\n"
//			" _ _ _ X O X O _ X X X O _ _ _\n"
//			" _ _ _ O X O X X O _ _ _ _ _ _\n"
//			" _ O X X X O O _ _ _ _ _ _ _ _\n"
//			" _ _ O _ _ _ X _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//	sign_to_move = Sign::CIRCLE;

//// @formatter:off
//	board = Board::fromString(
//			" _ _ _ _ _ _ _ _ X O X X _ _ _\n"
//			" _ _ _ _ _ _ X O X O _ _ _ _ _\n"
//			" _ _ _ O _ O _ _ _ X _ _ _ _ _\n"
//			" _ _ _ _ X O _ _ X _ X _ _ _ _\n"
//			" _ _ _ _ _ O _ _ _ _ _ O _ _ _\n"
//			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//// @formatter:on
//	sign_to_move = Sign::CROSS;

// @formatter:off
//	board = Board::fromString(
//    /*         a b c d e f g h i j k l m n o          */
//	/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//	/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n" /*  1 */
//	/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//	/*  3 */ " _ _ _ _ _ _ _ _ _ _ O _ _ _ _\n" /*  3 */
//	/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//	/*  5 */ " _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /*  5 */
//	/*  6 */ " _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /*  6 */
//	/*  7 */ " _ _ _ O _ _ _ X _ _ _ _ _ _ _\n" /*  7 */
//	/*  8 */ " _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//	/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//	/* 10 */ " _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 10 */
//	/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//	/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//	/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//	/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//    /*         a b c d e f g h i j k l m n o          */);
//	board = Board::fromString(
//		/*         a b c d e f g h i j k l m n o        */
//		/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//		/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//		/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//		/*  3 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  3 */
//		/*  4 */ " _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /*  4 */
//		/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//		/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//		/*  7 */ " _ O _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  7 */
//		/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//		/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//		/* 10 */ " _ _ _ _ _ _ _ _ X _ X _ _ _ _\n" /* 10 */
//		/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ O _ _ _\n" /* 11 */
//		/* 12 */ " _ _ _ _ _ _ _ _ _ X _ _ O _ _\n" /* 12 */
//		/* 13 */ " _ _ _ _ _ _ _ _ O _ _ _ _ O _\n" /* 13 */
//		/* 14 */ " _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n" /* 14 */
//		/*         a b c d e f g h i j k l m n o        */);

//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ _ _ O O O X _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ O X O O _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ _ _ O X X X X O _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ _ _ X _ _ X _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ _ _ _ O X _ _ X _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ X O _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ O O _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o p q r s t          */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ _ _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/* 15 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 15 */
//			/* 16 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 16 */
//			/* 17 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 17 */
//			/* 18 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 18 */
//			/* 19 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 19 */
//			/*         a b c d e f g h i j k l m n o p q r s t          */
//			);
// @formatter:on
//	sign_to_move = Sign::CIRCLE;

// @formatter:off
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ _ X O _ _ _ X _ O _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ X O _ _ X O O _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ O O O X O X X X O _ _ _\n" /*  9 */
//			/* 10 */ " _ _ X X O O X O _ O X X X _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ X O _ X _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ X _ _ _ _ _ O _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ O _ _ _ _ _ _ _ _ X _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
//	board = Board::fromString(
//			/*         a b c d e f g h i j k l m n o        */
//			/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */ " _ _ _ _ X _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */ " _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */ " _ _ _ X _ X _ X _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */ " _ _ _ _ _ _ _ _ _ X _ _ _ _ _\n" /*  8 */
//			/*  9 */ " _ _ _ _ _ _ O _ O X _ _ _ _ _\n" /*  9 */
//			/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /* 11 */
//			/* 12 */ " _ _ _ _ _ _ _ _ O O _ _ _ _ _\n" /* 12 */
//			/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*         a b c d e f g h i j k l m n o        */);
// @formatter:on
//	sign_to_move = Sign::CROSS;

//	ThreatSpaceSearch ts_search(game_config, search_config.tss_config);
	AlphaBetaSearch ts_search(game_config);
	ts_search.loadWeights(nnue::NNUEWeights("networks/freestyle_nnue_64x16x16x1.bin"));
	ts_search.setNodeLimit(1000000);

	SearchTask task(game_config);

//	SearchTask task2(game_config);
//	task2.set(board, sign_to_move);
//	ts_search.solve(task2);
//	std::cout << "\n\n\n";
//	ts_search.print_stats();
//	std::cout << "\n\n\n" << task2.toString() << '\n';
//	return;

	Search search(game_config, search_config);
	search.getSolver().loadWeights(nnue::NNUEWeights("networks/freestyle_nnue_64x16x16x1.bin"));
	tree.setBoard(board, sign_to_move);
//	tree.setEdgeSelector(NoisyPUCTSelector(Board::numberOfMoves(board), 1.25f));

//	search_config.mcts_config.edge_selector_config.init_to = "q_head";
//	search_config.mcts_config.edge_selector_config.noise_type = "none";
//	search_config.mcts_config.edge_selector_config.noise_weight = 1.0f;

//	search_config.mcts_config.edge_selector_config.init_to = "loss";
	tree.setEdgeSelector(PUCTSelector(search_config.mcts_config.edge_selector_config));
//	tree.setEdgeSelector(ThompsonSelector(search_config.mcts_config.edge_selector_config));
//	tree.setEdgeSelector(KLUCBSelector(search_config.mcts_config.edge_selector_config));
	tree.setEdgeGenerator(UnifiedGenerator(search_config.mcts_config.max_children, search_config.mcts_config.policy_expansion_threshold, true));
//	tree.setEdgeGenerator(UnifiedGenerator(true));

	int next_step = 0;
	double time_per_sample = 0.1;
	for (int j = 0; j <= 10000; j++)
	{
		if (tree.getSimulationCount() >= next_step)
		{
			std::cout << tree.getSimulationCount() << " ..." << std::endl;
			next_step += 1000;
		}
		search.select(tree, 10000);
		search.solve(getTime() + 0.5 * search.getBatchSize() * time_per_sample);
		search.scheduleToNN(nn_evaluator);
		time_per_sample = nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);

//		std::cout << "\n\n\n";
//		tree.printSubtree(10, true, 5);
		if (tree.isRootProven())
			break;
	}
	search.cleanup(tree);

//	tree.printSubtree(100);

	std::cout << search.getStats().toString() << '\n';
	std::cout << "memory = " << ((tree.getMemory() + search.getMemory()) / 1048576.0) << "MB\n\n";
	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
	std::cout << tree.getNodeCacheStats().toString() << '\n';
	std::cout << nn_evaluator.getStats().toString() << '\n';
	search.getSolver().print_stats();

	Node info = tree.getInfo( { });
	info.sortEdges();
	std::cout << info.toString() << '\n';
	for (int i = 0; i < info.numberOfEdges(); i++)
		std::cout << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
	std::cout << '\n';

	LCBSelector lcb(search_config.mcts_config.edge_selector_config);
	lcb.select(&info);

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

	std::cout << Board::toString(board, true) << '\n';

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
	const std::string path = "/home/maciek/alphagomoku/new_runs_2024/";
	MasterLearningConfig config(FileLoader(path + "config.json").getJson());
	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

	SelfplayConfig cfg(config.evaluation_config.selfplay_options);

//	cfg.simulations = 1000;
//	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_255_opt.bin", "AG_255");
//
//	cfg.simulations = 1750;
//	cfg.final_selector.exploration_constant = 1.25f;
//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 1.25f;
//	manager.setFirstPlayer(cfg, "./old_6x64s.bin", "old_6x64s_x1.75");

//	cfg.simulations = 400;
//	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs/btl_pv_8x128f/checkpoint/network_220_opt.bin", "AG_220");
//	cfg.final_selector.exploration_constant = 1.25f;
//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 1.25f;
//	manager.setFirstPlayer(cfg, "./old_6x64f.bin", "old_6x64f");

	cfg.simulations = 100;
	cfg.search_config.mcts_config.edge_selector_config.init_to = "parent";
	cfg.search_config.mcts_config.edge_selector_config.policy = "puct";
	manager.setFirstPlayer(cfg, "/home/maciek/Desktop/AlphaGomoku571/networks/standard_conv_8x128.bin", "puct_init_parent");

//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 1.25;
//	cfg.search_config.mcts_config.edge_selector_config.init_to = "q_head";
//	cfg.search_config.tss_config.mode = 2;

//	cfg.search_config.tss_config.mode = 1;
//	manager.setSecondPlayer(cfg, "/home/maciek/Desktop/AlphaGomoku571/networks/standard_conv_8x128.bin", "ab");

//	cfg.search_config.tss_config.mode = 2;
//	cfg.search_config.tss_config.hash_table_size *= 2;
	cfg.search_config.mcts_config.edge_selector_config.init_to = "loss";
	cfg.search_config.mcts_config.edge_selector_config.policy = "puct";
	manager.setSecondPlayer(cfg, "/home/maciek/Desktop/AlphaGomoku571/networks/standard_conv_8x128.bin", "puct_150_init_loss");

//	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/new_runs/sl_btl_brd_pv_8x128s/checkpoint/network_261_opt.bin", "broadcast_261");
//	cfg.final_selector.exploration_constant = 1.25f;
//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 1.25f;
//	cfg.simulations = 1000;
//	cfg.search_config.tss_config.mode = 2;
//	manager.setFirstPlayer(cfg, "./old_6x64s.bin", "tss2_reduced_at_root_x2");
//	cfg.simulations = 750;
//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 0.5f;
//	cfg.search_config.tss_config.mode = 1;
//	manager.setSecondPlayer(cfg, "./old_6x64s.bin", "tss1");

	const double start = getTime();
	manager.generate(4000);
	const double stop = getTime();
	std::cout << "generated in " << (stop - start) << '\n';

	const std::string to_save = manager.getPGN();
	std::ofstream file("/home/maciek/alphagomoku/new_runs_2024/puct_100.pgn", std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();

//	for (int i = 90; i < 100; i += 1)
//	{
//		cfg.simulations = 400;
//		manager.setFirstPlayer(cfg,
//				"/home/maciek/alphagomoku/new_runs/supervised/broadcast_skip_bias_relu_8x128s/network_" + std::to_string(i) + "_opt.bin",
//				"broadcast_skip_bias_relu_" + std::to_string(i));
////		cfg.simulations = 800;
////		manager.setFirstPlayer(cfg,
////				"/home/maciek/alphagomoku/new_runs/supervised/broadcast_skip2_relu_8x128s/network_" + std::to_string(i) + "_opt.bin",
////				"broadcast_skip_relu_" + std::to_string(i) + "_x0.8");
////		cfg.simulations = 1000;
////		manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_96_opt.bin", "bottleneck");
//		cfg.final_selector.exploration_constant = 1.25f;
//		cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 1.25f;
//		manager.setSecondPlayer(cfg, "./old_6x64s.bin", "old_6x64s");
//
//		const double start = getTime();
//		manager.generate(1000);
//		const double stop = getTime();
//		std::cout << "generated in " << (stop - start) << '\n';
//
//		const std::string to_save = manager.getPGN();
//		std::ofstream file("/home/maciek/alphagomoku/new_runs/supervised/direct_brd.pgn", std::ios::out | std::ios::app);
//		file.write(to_save.data(), to_save.size());
//		file.close();
//	}
}

void parameter_tuning()
{
//	std::vector<double> exploration_c = { 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5 };
//	std::vector<double> exploration_exp = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 };

//	std::vector<double> exploration_tree = { 0.25, 0.5, 0.75, 1.0, 1.25 };
//	std::vector<double> exploration_final = { 0.25, 0.5, 0.75, 1.0, 1.25 };

	std::vector<double> exploration_tree = { 1.25 };
	std::vector<double> exploration_final = { 1.25 };

	for (size_t i1 = 0; i1 < exploration_tree.size(); i1++)
		for (size_t i2 = 0; i2 < exploration_final.size(); i2++)
		{
			const std::string path = "/home/maciek/alphagomoku/new_runs/";
			MasterLearningConfig config(FileLoader(path + "tuning_config.json").getJson());
			EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

			SelfplayConfig cfg(config.evaluation_config.selfplay_options);
			cfg.simulations = 1000;
			cfg.search_config.tss_config.max_positions = 200;

			manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/new_runs/test8_test/checkpoint/network_144_opt.bin", "base");

			cfg.search_config.mcts_config.edge_selector_config.exploration_constant = exploration_tree[i1];
//			cfg.final_selector.policy = "lcb";
//			cfg.final_selector.init_to = "loss";
//			cfg.search_config.mcts_config.edge_selector_config.init_to = "parent";
			cfg.search_config.tree_config.information_leak_threshold = -0.01;
			cfg.final_selector.exploration_constant = exploration_final[i2];
			manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs/test8_test/checkpoint/network_144_opt.bin",
					"lcb_" + std::to_string(exploration_tree[i1]) + "_" + std::to_string(exploration_final[i2]));

			std::cout << "testing " << exploration_tree[i1] << " " << exploration_final[i2] << '\n';
			const double start = getTime();
			manager.generate(400);
			const double stop = getTime();
			std::cout << "generated in " << (stop - start) << '\n';

			const std::string to_save = manager.getPGN();
			std::ofstream file("/home/maciek/alphagomoku/new_runs/testy_minimax2.pgn", std::ios::out | std::ios::app);
			file.write(to_save.data(), to_save.size());
			file.close();
		}
}

class Embedding
{
		/*
		 * row 0 - is cross (black) to move. If set to 0 it means that circle (white) is moving.
		 * Input encoding
		 *   0:6 - cross (black) threats (OPEN_3 to FIVE)
		 *   7:13 - circle (white) threats (OPEN_3 to FIVE)
		 *     14 - is cross (black) stone
		 *     15 - is circle (white) stone
		 */
	public:
		void set(const PatternCalculator &calc, float *tensor)
		{
			tensor[0] = static_cast<int>(calc.getSignToMove() == Sign::CROSS);

			int idx = 1;
			for (int row = 0; row < calc.getConfig().rows; row++)
				for (int col = 0; col < calc.getConfig().cols; col++, idx += 16)
				{
					const ThreatEncoding te = calc.getThreatAt(row, col);
					const ThreatType cross = std::max(ThreatType::OPEN_3, std::min(ThreatType::FIVE, te.forCross()));
					const ThreatType circle = std::max(ThreatType::OPEN_3, std::min(ThreatType::FIVE, te.forCircle()));
					tensor[idx + (int) cross - 2] = 1.0f;
					tensor[idx + 7 + (int) circle - 2] = 1.0f;
					switch (calc.signAt(row, col))
					{
						default:
							break;
						case Sign::CROSS:
							tensor[idx + 14] = 1.0f;
							break;
						case Sign::CIRCLE:
							tensor[idx + 15] = 1.0f;
							break;
					}
				}
		}
};

std::string convert(float f)
{
	if (f == 0.0f)
		return "0.0f";
	else
		return std::to_string(f) + 'f';
}

void train_nnue()
{
	ml::Device::setNumberOfThreads(1);
	GameDataBuffer buffer;
	for (int i = 200; i < 225; i++)
		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128c5/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	GameConfig game_config(GameRules::CARO5, 15);

	SearchDataPack pack(game_config.rows, game_config.cols);
	const int batch_size = 1024;
	nnue::TrainingNNUE model(game_config, { 64, 16, 16, 1 }, batch_size);

	for (int e = 0; e < 100; e++)
	{
		if (e == 50)
			model.setLearningRate(1.0e-4f);
		if (e == 75)
			model.setLearningRate(1.0e-5f);

		const double start = getTime();
		float loss = 0.0f;
		int count = 0;
		for (int step = 0; step < 1000; step++)
		{
			for (int b = 0; b < batch_size; b++)
			{
				const int i = randInt(buffer.numberOfGames());
				const int j = randInt(buffer.getGameData(i).numberOfSamples());

				buffer.getGameData(i).getSample(pack, j);
				augment(pack.board, randInt(8));
				model.packInputData(b, pack.board, pack.played_move.sign);
//				model.packTargetData(b, convertOutcome(pack.game_outcome, pack.played_move.sign).getExpectation());
				model.packTargetData(b, pack.minimax_value.getExpectation());
			}

			model.forward(batch_size);
			loss += model.backward(batch_size);
			model.learn();
			count++;
		}
		std::cout << "epoch " << e << ", loss " << loss / count << " in " << (getTime() - start) << "s\n";
		model.save("nnue_c5_64x16x16x1.bin");
	}
}

void prepare_proven_dataset()
{
	SearchDataPack pack(15, 15);
	GameDataBuffer result;

	for (int b = 100; b <= 150; b++)
	{
		GameDataBuffer buffer;
		buffer.load("/home/maciek/alphagomoku/new_runs/test8_test/train_buffer/buffer_" + std::to_string(b) + ".bin");
//		buffer.load("/home/maciek/alphagomoku/new_runs/test8_test/valid_buffer/buffer_" + std::to_string(b) + ".bin");

		for (int i = 0; i < buffer.numberOfGames(); i++)
		{
			if (i % (buffer.numberOfGames() / 10) == 0)
				std::cout << i << " / " << buffer.numberOfGames() << '\n';

			for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
			{
				buffer.getGameData(i).getSample(pack, j);
				if (pack.minimax_score.isProven())
				{
					GameDataStorage tmp(15, 15);
					for (int k = 0; k < buffer.getGameData(i).numberOfMoves(); k++)
						tmp.addMove(buffer.getGameData(i).getMove(k));
					tmp.addSample(pack);
					result.addGameData(tmp);
					break;
				}
			}
		}
	}

	std::cout << result.getStats().toString() << '\n';
	result.save("/home/maciek/alphagomoku/proven_positions_big.bin");
}

void test_proven_search(int mcts_nodes, int tss_nodes, bool fast)
{
//	GameConfig game_config(GameRules::FREESTYLE, 15);
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::RENJU, 15);
//	GameConfig game_config(GameRules::CARO5, 15);
//	GameConfig game_config(GameRules::CARO6, 20);

	GameDataBuffer buffer("/home/maciek/alphagomoku/proven_positions.bin");
	std::cout << buffer.getStats().toString() << '\n';

	SearchConfig search_config;
	search_config.max_batch_size = 32;
	search_config.mcts_config.edge_selector_config.exploration_constant = 1.25f;
	search_config.mcts_config.max_children = 32;
	search_config.tss_config.mode = 2;
	search_config.tss_config.max_positions = tss_nodes;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
	device_config.device = ml::Device::cpu();
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku560/networks/standard_conv_8x128.bin");
//	nn_evaluator.loadGraph("./old_6x64s.bin");

	Search search(game_config, search_config);
	Tree tree(search_config.tree_config);

	SearchDataPack pack(game_config.rows, game_config.cols);
	int total_samples = 0;
	int solved_count = 0;
	int total_visits = 0;

	const int count = fast ? 1000 : buffer.numberOfGames();

	for (int i = 0; i < buffer.numberOfGames(); i += (buffer.numberOfGames() / count))
//	int i = 0;
	{
		if (i % (count / 10) == 0)
			std::cout << "processed " << i << " / " << count << ", solved " << solved_count << "/" << total_samples << '\n';

		total_samples += buffer.getGameData(i).numberOfSamples();
		for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
		{
			buffer.getGameData(i).getSample(pack, j);

			tree.setBoard(pack.board, pack.played_move.sign);
			search.setBoard(pack.board, pack.played_move.sign);
			tree.setEdgeSelector(PUCTSelector(search_config.mcts_config.edge_selector_config));
			tree.setEdgeGenerator(UnifiedGenerator(search_config.mcts_config.max_children, search_config.mcts_config.policy_expansion_threshold));

			for (int k = 0; k <= mcts_nodes; k++)
			{
				search.select(tree, mcts_nodes);
				search.solve();
				search.scheduleToNN(nn_evaluator);
				nn_evaluator.evaluateGraph();

				search.generateEdges(tree);
				search.expand(tree);
				search.backup(tree);
				if (tree.isRootProven())
					break;
			}
			search.cleanup(tree);

			const Node info = tree.getInfo( { });
			solved_count += info.getScore().isProven();
			total_visits += info.getVisits();
//			if (info.getScore().isProven())
//			{
//				pack.print();
//				tree.printSubtree(5);
//				std::cout << info.toString() << '\n';
//				for (int k = 0; k < info.numberOfEdges(); k++)
//					std::cout << info.getEdge(k).toString() << '\n';
//				std::cout << i << '\n';
//				exit(0);
//			}
		}

	}

//	search.getSolver().print_stats();
	std::cout << search.getStats().toString() << '\n';
	std::cout << tree.getNodeCacheStats().toString() << '\n';
	std::cout << '\n' << "solved " << solved_count << "/" << total_samples << " (" << 100.0f * solved_count / total_samples << "%)\n";
	std::cout << "total visits = " << total_visits << '\n';
}

void convert_old_network()
{
//	FileLoader fl("/home/maciek/Desktop/AlphaGomoku550/networks/freestyle_6x64.bin");
//	FileLoader fl("/home/maciek/alphagomoku/standard_15x15/checkpoint/network_90_opt.bin");
	FileLoader fl("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/libml/checkpoint/network_99_opt.bin");

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.blocks = 6;
	training_config.filters = 64;

	ResnetOld model;
	model.init(game_config, training_config);
	model.optimize();
	model.getGraph().print();

	Json old_layers = fl.getJson()["layers"];
	int old_layer_id = 1;
	int new_layer_id = 1;
	for (; old_layer_id < 2 + 2 * training_config.blocks; old_layer_id++, new_layer_id++)
	{ // convolutional blocks
		ml::Layer &layer = model.getGraph().getNode(new_layer_id).getLayer();
		assert(old_layers[old_layer_id]["name"].getString() == "Conv2D");
		layer.getWeights().getParam().unserialize(old_layers[old_layer_id]["weights"]["param"], fl.getBinaryData());
		layer.getBias().getParam().unserialize(old_layers[old_layer_id]["bias"]["param"], fl.getBinaryData());
	}

// policy head
	ml::Layer &layer_p1 = model.getGraph().getNode(new_layer_id).getLayer(); // first Conv2D layer
	assert(layer_p1.name() == "Conv2D");
	assert(old_layers[old_layer_id]["name"].getString() == "Conv2D");
	layer_p1.getWeights().getParam().unserialize(old_layers[old_layer_id]["weights"]["param"], fl.getBinaryData());
	layer_p1.getBias().getParam().unserialize(old_layers[old_layer_id]["bias"]["param"], fl.getBinaryData());
	old_layer_id++;
	new_layer_id++;

	ml::Layer &layer_p2 = model.getGraph().getNode(new_layer_id).getLayer(); // second Conv2D layer
	assert(layer_p2.name() == "Conv2D");
	assert(old_layers[old_layer_id]["name"].getString() == "Conv2D");
	assert(old_layers[old_layer_id + 1]["name"].getString() == "Flatten");
	layer_p2.getWeights().getParam().unserialize(old_layers[old_layer_id]["weights"]["param"], fl.getBinaryData());
	old_layer_id += 1 + 1; // this layer + Flatten
	new_layer_id++;

	ml::Layer &layer_p3 = model.getGraph().getNode(new_layer_id).getLayer(); // final Dense layer
	assert(layer_p3.name() == "Dense");
	assert(old_layers[old_layer_id]["name"].getString() == "Affine");
	assert(old_layers[old_layer_id + 1]["name"].getString() == "Softmax");
//	ml::Tensor &affine_weights = layer_p3.getWeights().getParam();
//	affine_weights.zeroall(model.getGraph().context());
//	for (int i = 0; i < game_config.rows * game_config.cols; i++)
//		affine_weights.set(1.0f, { i, i }); // creating artificial diagonal unit matrix so only bias will be used
	layer_p3.getBias().getParam().unserialize(old_layers[old_layer_id]["bias"]["param"], fl.getBinaryData());
	old_layer_id += 1 + 1;
	new_layer_id += 1 + 1; // this layer + Softmax

// value head
	ml::Layer &layer_v1 = model.getGraph().getNode(new_layer_id).getLayer(); // first Conv2D layer
	assert(layer_v1.name() == "Conv2D");
	assert(old_layers[old_layer_id]["name"].getString() == "Conv2D");
	layer_v1.getWeights().getParam().unserialize(old_layers[old_layer_id]["weights"]["param"], fl.getBinaryData());
	layer_v1.getBias().getParam().unserialize(old_layers[old_layer_id]["bias"]["param"], fl.getBinaryData());
	old_layer_id += 1 + 1; // this layer + Flatten
	new_layer_id++;

	ml::Layer &layer_v2 = model.getGraph().getNode(new_layer_id).getLayer(); // first Dense layer
	assert(layer_v2.name() == "Dense");
	assert(old_layers[old_layer_id]["name"].getString() == "Dense");
	layer_v2.getWeights().getParam().unserialize(old_layers[old_layer_id]["weights"]["param"], fl.getBinaryData());
	layer_v2.getBias().getParam().unserialize(old_layers[old_layer_id]["bias"]["param"], fl.getBinaryData());
	old_layer_id++;
	new_layer_id++;

	ml::Layer &layer_v3 = model.getGraph().getNode(new_layer_id).getLayer(); // second Dense layer
	assert(layer_v3.name() == "Dense");
	assert(old_layers[old_layer_id]["name"].getString() == "Dense");
	layer_v3.getWeights().getParam().unserialize(old_layers[old_layer_id]["weights"]["param"], fl.getBinaryData());
	layer_v3.getBias().getParam().unserialize(old_layers[old_layer_id]["bias"]["param"], fl.getBinaryData());
//	for (int i = 0; i < layer_v3.getWeights().shape().lastDim(); i++)
//	{ // swap win and loss outputs as they were inverted in previous version
//		const float tmp0 = layer_v3.getWeights().getParam().get( { 0, i });
//		const float tmp2 = layer_v3.getWeights().getParam().get( { 2, i });
//		layer_v3.getWeights().getParam().set(tmp2, { 0, i });
//		layer_v3.getWeights().getParam().set(tmp0, { 2, i });
//	}
//	// same for biases
//	const float tmp0 = layer_v3.getBias().getParam().get( { 0 });
//	const float tmp2 = layer_v3.getBias().getParam().get( { 2 });
//	layer_v3.getBias().getParam().set(tmp2, { 0 });
//	layer_v3.getBias().getParam().set(tmp0, { 2 });

	old_layer_id++;
	new_layer_id++;

	model.saveToFile("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/libml/libml_99_opt.bin");
}

void convert_old_buffer(const std::string &src, const std::string &dst)
{
	struct internal_data
	{
			uint32_t sign :2;
			uint32_t pv :2;
			uint32_t prior :18;
			uint32_t win :14;
			uint32_t draw :14;
			uint32_t loss :14;
	};
	auto convert_pv = [](int pv)
	{
		switch (pv)
		{
			default:
			case 0:
			return ProvenValue::UNKNOWN;
			case 1:
			return ProvenValue::LOSS;
			case 2:
			return ProvenValue::DRAW;
			case 3:
			return ProvenValue::WIN;
		}
	};

	FileLoader fl(src, true);

	const SerializedObject &so = fl.getBinaryData();

	const int rows = fl.getJson()[0]["rows"].getInt();
	const int cols = fl.getJson()[0]["cols"].getInt();

	GameConfig cfg(rulesFromString(fl.getJson()[0]["rules"].getString()), rows, cols);

	GameDataBuffer result(cfg);

	size_t size = fl.getJson().size();
	for (size_t g = 0; g < size; g++)
	{
		const Json &game = fl.getJson()[g];

		GameDataStorage gds(game["rows"].getInt(), game["cols"].getInt());

		size_t offset = game["binary_offset"].getLong();
		size_t nb_of_moves = game["nb_of_moves"].getLong();

		for (size_t i = 0; i < nb_of_moves; i++, offset += sizeof(uint16_t))
		{
			const Move m(so.load<uint16_t>(offset));
			gds.addMove(m);
		}
		gds.setOutcome(outcomeFromString(game["outcome"]));

		size_t nb_of_states = game["nb_of_states"].getLong();
		for (size_t i = 0; i < nb_of_states; i++)
		{
			offset += sizeof(int); // rows
			offset += sizeof(int); // cols

			SearchDataPack pack(rows, cols);

			matrix<internal_data> actions(rows, cols);
			so.load(actions.data(), offset, actions.sizeInBytes());
			offset += actions.sizeInBytes();

			float sum_visits = 0.0f;
			for (int j = 0; j < actions.size(); j++)
			{
				pack.board[j] = static_cast<Sign>(actions[j].sign);
				pack.action_scores[j] = Score(convert_pv(actions[j].pv));
				pack.visit_count[j] = actions[j].prior;
				sum_visits += pack.visit_count[j];
				pack.action_values[j] = Value(actions[j].win / 16383.0f, actions[j].draw / 16383.0f);
			}
			for (int j = 0; j < actions.size(); j++)
				pack.visit_count[j] = static_cast<int>(800.0f / sum_visits * pack.visit_count[j] + 0.5f);

			const float win_rate = so.load<float>(offset);
			offset += sizeof(float);
			const float draw_rate = so.load<float>(offset);
			offset += sizeof(float);
			offset += sizeof(float); // loss rate
			pack.minimax_value = Value(win_rate, draw_rate);

			const int tmp = so.load<int>(offset);
			offset += sizeof(int);

			const ProvenValue proven_value = convert_pv(tmp);
			pack.minimax_score = Score(proven_value);

			const GameOutcome game_outcome = static_cast<GameOutcome>(so.load<int>(offset));
			offset += sizeof(int);
			pack.game_outcome = game_outcome;

			const Move played_move(so.load<uint16_t>(offset));
			offset += sizeof(uint16_t);
			pack.played_move = played_move;

			gds.addSample(pack);
		}

		result.addGameData(gds);
	}

	std::cout << result.getStats().toString() << '\n';
	result.save(dst);
}

void transfer_learning()
{
	ml::Device::setNumberOfThreads(1);
	GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/transfer/saved_state/buffer.bin");
//	buffer.load("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/transfer/valid_buffer/buffer_0.bin");
	std::cout << buffer.getStats().toString() << '\n';

	const int batch_size = 256;

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.network_arch = "ResnetOld";
	training_config.augment_training_data = true;
	training_config.blocks = 6;
	training_config.filters = 64;
	training_config.device_config.batch_size = batch_size;
	training_config.l2_regularization = 1.0e-4f;

	std::unique_ptr<AGNetwork> teacher = loadAGNetwork("./old_10x128s.bin");
//	std::unique_ptr<AGNetwork> teacher = loadAGNetwork(
//			"/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/corrected/checkpoint/network_99_opt.bin");
	teacher->setBatchSize(batch_size);
	teacher->moveTo(ml::Device::cuda(0));

	std::unique_ptr<AGNetwork> network = createAGNetwork(training_config.network_arch);
	network->init(game_config, training_config);
	network->moveTo(ml::Device::cuda(0));
	network->changeLearningRate(1.0e-3f);

	SamplerVisits sampler;
//	sampler.init(buffer, batch_size);

	std::vector<TrainingDataPack> training_batch;
	for (size_t i = 0; i < batch_size; i++)
		training_batch.push_back(TrainingDataPack(15, 15));

	const std::string path = "/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/corrected_full/checkpoint/";
	for (int e = 0; e < 160; e++)
	{
		std::vector<float> loss(2, 0.0f);
		if (e == 80)
			network->changeLearningRate(1.0e-4f);
		if (e == 120)
			network->changeLearningRate(1.0e-5f);
		for (int i = 0; i < 1000; i++)
		{
			if (i % 100 == 0)
				std::cout << i << '\n';
			for (int b = 0; b < batch_size; b++)
			{
				sampler.get(training_batch.at(b));
				if (training_config.augment_training_data)
					ag::augment(training_batch.at(b).board, randInt(8));
				teacher->packInputData(b, training_batch.at(b).board, training_batch.at(b).sign_to_move);
				network->packInputData(b, training_batch.at(b).board, training_batch.at(b).sign_to_move);
			}
			teacher->forward(batch_size);

			for (int b = 0; b < batch_size; b++)
			{
				teacher->unpackOutput(b, training_batch.at(b).policy_target, training_batch.at(b).action_values_target,
						training_batch.at(b).value_target);
				network->packTargetData(b, training_batch.at(b).policy_target, training_batch.at(b).action_values_target,
						training_batch.at(b).value_target);
			}
			network->forward(batch_size);
			network->backward(batch_size);
			std::vector<float> tmp = network->getLoss(batch_size);
			loss[0] += tmp[0];
			loss[1] += tmp[1];
		}
		std::cout << "epoch " << e << " : " << loss[0] / 1000 << " " << loss[1] / 1000 << '\n';
		network->saveToFile(path + "network_" + std::to_string(e) + ".bin");

		std::unique_ptr<AGNetwork> tmp = loadAGNetwork(path + "network_" + std::to_string(e) + ".bin");
		tmp->optimize();
		tmp->saveToFile(path + "network_" + std::to_string(e) + "_opt.bin");
	}
}

#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/Softmax.hpp>

void compare_models()
{
	const ml::Shape input_shape( { 32, 15, 15, 4 });
	const int blocks = 6;
	const int filters = 64;

	ml::Graph graph;
	auto x = graph.addInput(input_shape);

	x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
	x = graph.add(ml::BatchNormalization("relu").useGamma(true), x);

	for (int i = 0; i < blocks; i++)
	{
		auto y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		y = graph.add(ml::BatchNormalization("relu").useGamma(true), y);

		y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("linear").useGamma(true), y);

		x = graph.add(ml::Add("relu"), { x, y });
	}

// policy head
	auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
	p = graph.add(ml::BatchNormalization("relu").useGamma(true), p);

	p = graph.add(ml::Conv2D(1, 1, "linear").useBias(false), p);
	p = graph.add(ml::Dense(15 * 15, "linear").useWeights(false), p);
	p = graph.add(ml::Softmax( { 1 }), p);
	graph.addOutput(p);

// value head
	auto v = graph.add(ml::Conv2D(2, 1, "linear").useBias(false), x);
	v = graph.add(ml::BatchNormalization("relu").useGamma(true), v);

	v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), v);
	v = graph.add(ml::BatchNormalization("relu").useGamma(true), v);
	v = graph.add(ml::Dense(3, "linear"), v);
	v = graph.add(ml::Softmax( { 1 }), v);
	graph.addOutput(v);

	graph.init();
	graph.setOptimizer(ml::Optimizer());
	graph.setRegularizer(ml::Regularizer(1.0e-4f));
	graph.moveTo(ml::Device::cpu());
//	graph.moveTo(ml::Device::cuda(0));

	for (int i = 0; i < 1; i++)
	{
		std::cout << "step " << i << " -----------------------------------------\n";
		ml::testing::initForTest(graph.getInput(), 0.1 * i);
		graph.forward(input_shape[0]);
		graph.context().synchronize();
		std::cout << '\n';
		graph.backward(input_shape[0]);
		graph.context().synchronize();
		graph.learn();
		graph.context().synchronize();
	}
	std::cout << "inference-----------------------------------------\n";
	graph.makeNonTrainable();
	ml::testing::initForTest(graph.getInput(), 0);
	graph.forward(input_shape[0]);
}

float square(float x)
{
	return x * x;
}
float cross_entropy(const float *target, const float *output, int size)
{
	float result = 0.0;
	for (int i = 0; i < size; i++)
	{
		result += -target[i] * safe_log(output[i]) - (1.0f - target[i]) * safe_log(1.0f - output[i]);
		result -= -target[i] * safe_log(target[i]) - (1.0f - target[i]) * safe_log(1.0f - target[i]);
	}
	return result;
}
float cross_entropy(Value target, Value output)
{
	float t[3] = { std::max(0.0f, target.win_rate), std::max(0.0f, target.draw_rate), std::max(0.0f, target.loss_rate()) };
	float o[3] = { std::max(0.0f, output.win_rate), std::max(0.0f, output.draw_rate), std::max(0.0f, output.loss_rate()) };
	return cross_entropy(t, o, 3);
}

namespace evaluation
{
	class Player
	{
		protected:
			std::string name;
			Sign sign = Sign::NONE;
		public:
			virtual ~Player() = default;
			virtual void setSign(Sign s) noexcept
			{
				sign = s;
			}
			Sign getSign() const noexcept
			{
				return sign;
			}
			std::string getName() const
			{
				return name;
			}
			virtual void setBoard(const matrix<Sign> &board, Sign signToMove) = 0;
			virtual Move getMove() const noexcept = 0;
			virtual std::unique_ptr<Player> clone() const = 0;
	};
	class MinimaxPlayer: public Player
	{
			SearchTask task;
			MinimaxSearch search;
			int max_depth;
			MoveGeneratorMode mode;
		public:
			MinimaxPlayer(const std::string &name, GameConfig cfg, int depth, MoveGeneratorMode mode) :
					task(cfg),
					search(cfg),
					max_depth(depth),
					mode(mode)
			{
				this->name = name;
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove)
			{
				task.set(board, signToMove);
				search.solve(task, max_depth, mode);
			}
			Move getMove() const noexcept
			{
				matrix<Score> scores = task.getActionScores();
				Score best_score = Score::min_value();
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) > best_score and task.getBoard().at(row, col) == Sign::NONE)
							best_score = scores.at(row, col);

				std::vector<Move> moves;
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) == best_score and task.getBoard().at(row, col) == Sign::NONE)
							moves.emplace_back(task.getSignToMove(), row, col);
				return moves.at(randInt(moves.size()));
			}
			std::unique_ptr<Player> clone() const
			{
				return std::make_unique<MinimaxPlayer>(name, task.getConfig(), max_depth, mode);
			}
	};
	class PolicyHeadPlayer: public Player
	{
			SearchTask task;
			std::string path_to_network;
			NNEvaluator evaluator;
		public:
			PolicyHeadPlayer(const std::string &name, GameConfig cfg, const std::string &path) :
					task(cfg),
					path_to_network(path),
					evaluator(DeviceConfig())
			{
				this->name = name;
				evaluator.loadGraph(path);
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove)
			{
				task.set(board, signToMove);
				evaluator.addToQueue(task);
				evaluator.evaluateGraph();
			}
			Move getMove() const noexcept
			{
				matrix<float> policy = task.getPolicy();
				Move best_move;
				float best_policy = std::numeric_limits<float>::lowest();
				for (int row = 0; row < policy.rows(); row++)
					for (int col = 0; col < policy.cols(); col++)
						if (policy.at(row, col) > best_policy and task.getBoard().at(row, col) == Sign::NONE)
						{
							best_policy = policy.at(row, col);
							best_move = Move(row, col, task.getSignToMove());
						}
				return best_move;
			}
			std::unique_ptr<Player> clone() const
			{
				return std::make_unique<PolicyHeadPlayer>(name, task.getConfig(), path_to_network);
			}
	};
	class AlphaBetaPlayer: public Player
	{
			SearchTask task;
			AlphaBetaSearch search;
			int max_depth, max_nodes;
			MoveGeneratorMode mode;
			Score best_score = Score::minus_infinity();
			matrix<Sign> last_board;
		public:
			AlphaBetaPlayer(const std::string &name, GameConfig cfg, int depth, int nodes, MoveGeneratorMode mode) :
					task(cfg),
					search(cfg),
					max_depth(depth),
					max_nodes(nodes),
					mode(mode)
			{
				this->name = name;
				search.loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
				search.setDepthLimit(max_depth);
				search.setNodeLimit(max_nodes);
			}
			void setSign(Sign s) noexcept
			{
				Player::setSign(s);
				best_score = Score::minus_infinity();
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove)
			{
				task.set(board, signToMove);
				search.solve(task);
				const Score s = task.getScore();
				if (best_score.isWin() and not s.isWin())
				{
					std::cout << "Using " << (int) mode << " mode\n";
					std::cout << "score changed from " << best_score << " to " << s << '\n';

					std::cout << signToMove << " to move\n";
					std::cout << Board::toString(last_board, true);
					PatternCalculator calc(task.getConfig());
					calc.setBoard(last_board, signToMove);
					calc.printAllThreats();

					std::cout << "current board\n" << Board::toString(board, true);

					exit(-2);
				}
				best_score = std::max(best_score, s);
				last_board = board;
			}
			Move getMove() const noexcept
			{
				matrix<Score> scores = task.getActionScores();
				Score best_score = Score::min_value();
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) > best_score and task.getBoard().at(row, col) == Sign::NONE)
							best_score = scores.at(row, col);

				std::vector<Move> moves;
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) == best_score and task.getBoard().at(row, col) == Sign::NONE)
							moves.emplace_back(task.getSignToMove(), row, col);
				return moves.at(randInt(moves.size()));
			}
			std::unique_ptr<Player> clone() const
			{
				return std::make_unique<AlphaBetaPlayer>(name, task.getConfig(), max_depth, max_nodes, mode);
			}
	};

	class EvaluationGame
	{
		private:
			struct GameResult
			{
					std::string cross_player;
					std::string circle_player;
					GameOutcome outcome;
			};
			GameConfig game_config;
			int games_to_play;
			std::vector<std::vector<Move>> openings;
			std::vector<GameResult> game_results;
			std::mutex main_mutex;

			std::unique_ptr<Player> first_player;
			std::unique_ptr<Player> second_player;
			std::vector<std::future<void>> threads;
		public:
			EvaluationGame(GameConfig gameConfig, int gamesToPlay, int numberOfThreads) :
					game_config(gameConfig),
					games_to_play(gamesToPlay),
					threads(numberOfThreads)
			{
				openings = generate_openings(gamesToPlay / 2, game_config, "/home/maciek/Desktop/AlphaGomoku560/networks/standard_conv_8x128.bin");
			}
			void start()
			{
				for (size_t i = 0; i < threads.size(); i++)
				{
					threads[i] = std::async(std::launch::async, [this]()
					{	this->run();});
				}
			}
			void waitForFinish()
			{
				for (size_t i = 0; i < threads.size(); i++)
				{
					for (int j = 0;; j++)
					{
						if (j % 60 == 0)
						{
							std::lock_guard<std::mutex> lg(main_mutex);
							std::cout << openings.size() * 2 << " games left\n";
							summarize_results();
						}
						if (threads[i].valid())
						{
							std::future_status input_status = threads[i].wait_for(std::chrono::milliseconds(1000));
							if (input_status == std::future_status::ready) // there is some input to process
							{
								threads[i].get();
								break;
							}
						}
						else
							break;
					}
				}
				summarize_results();
			}
			void setPlayers(const Player &firstPlayer, const Player &secondPlayer)
			{
				first_player = firstPlayer.clone();
				second_player = secondPlayer.clone();
			}
			void run()
			{
				for (;;)
				{
					std::vector<Move> opening;
					{ /* artificial scope for lock */
						std::lock_guard<std::mutex> lg(main_mutex);
						if (openings.empty())
							return;
						opening = openings.back();
						openings.pop_back();
					}

					std::unique_ptr<Player> player0 = first_player->clone();
					std::unique_ptr<Player> player1 = second_player->clone();

					const GameOutcome outcome1 = play_single_game(*player0, *player1, opening);
					const GameResult gr1 { player0->getName(), player1->getName(), outcome1 };

					const GameOutcome outcome2 = play_single_game(*player1, *player0, opening);
					const GameResult gr2 { player1->getName(), player0->getName(), outcome2 };

					std::lock_guard<std::mutex> lg(main_mutex);
					game_results.push_back(gr1);
					game_results.push_back(gr2);
				}
			}
		private:
			GameOutcome play_single_game(Player &crossPlayer, Player &circlePlayer, const std::vector<Move> &opening)
			{
				crossPlayer.setSign(Sign::CROSS);
				circlePlayer.setSign(Sign::CIRCLE);

				Game game(game_config);
				game.loadOpening(opening);
				while (not game.isOver())
				{
					std::cout << "Move " << game.numberOfMoves() << ", " << game.getSignToMove() << " to move\n";
					std::cout << Board::toString(game.getBoard(), true);
					Move move;
					if (game.getSignToMove() == Sign::CROSS)
					{
						crossPlayer.setBoard(game.getBoard(), game.getSignToMove());
						move = crossPlayer.getMove();
					}
					else
					{
						circlePlayer.setBoard(game.getBoard(), game.getSignToMove());
						move = circlePlayer.getMove();
					}
					game.makeMove(move);
				}
				return game.getOutcome();
			}
			void summarize_results() const
			{
				int wins = 0, draws = 0, losses = 0;
				for (size_t i = 0; i < game_results.size(); i++)
				{
					switch (game_results[i].outcome)
					{
						case GameOutcome::UNKNOWN:
							break;
						case GameOutcome::DRAW:
							draws++;
							break;
						case GameOutcome::CROSS_WIN:
							if (game_results[i].cross_player == first_player->getName())
								wins++;
							else
								losses++;
							break;
						case GameOutcome::CIRCLE_WIN:
							if (game_results[i].circle_player == first_player->getName())
								wins++;
							else
								losses++;
							break;
					}
				}
				std::cout << "'" << first_player->getName() << "' vs '" << second_player->getName() << "':\n";
				std::cout << wins << " : " << draws << " : " << losses << '\n';

				const double ratio = (wins + 0.5 * draws) / (wins + draws + losses);
				std::cout << "win ratio = " << ratio << "\n";
				const double elo_diff = 177.0 * (safe_log(ratio) - safe_log(1.0 - ratio));
				std::cout << "estimated elo difference = " << std::fabs(elo_diff) << "\n\n";
			}
	};
}

int main(int argc, char *argv[])
{
	std::cout << "BEGIN" << std::endl;
	std::cout << ml::Device::hardwareInfo() << '\n';
//	return 0;

	if (false)
	{
		GameConfig cfg(GameRules::STANDARD, 15, 15);
		evaluation::EvaluationGame eg(cfg, 100, 1);
//		evaluation::MinimaxPlayer player0("minimax_v0_reduced", cfg, 3, MoveGeneratorMode::REDUCED);
		evaluation::PolicyHeadPlayer player0("policy_head", cfg, "/home/maciek/Desktop/AlphaGomoku560/networks/standard_conv_8x128.bin");
//		evaluation::MinimaxPlayer player1("minimax_v0_optimal", cfg, 3, MoveGeneratorMode::OPTIMAL);
//		evaluation::AlphaBetaPlayer player0("alphabeta_v1_reduced", cfg, 7, 1000000, MoveGeneratorMode::REDUCED);
		evaluation::AlphaBetaPlayer player1("alphabeta_v1_optimal", cfg, 20, 1000000, MoveGeneratorMode::OPTIMAL);

		eg.setPlayers(player0, player1);
		eg.start();
		eg.waitForFinish();
		return 0;
	}
	if (false)
	{
		GameConfig cfg(GameRules::CARO5, 15, 15);
		evaluation::AlphaBetaPlayer player("", cfg, 20, 2000000, MoveGeneratorMode::OPTIMAL);
// @formatter:off
		matrix<Sign> board = Board::fromString(
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
				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
				/*         a b c d e f g h i j k l m n o        */

//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ O _ _ X O X _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ X X O O O O X O _ _ _\n" /*  3 */
//				/*  4 */ " _ _ X O O O O X X O X _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ X O O O X X X _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ X X X O X O O _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ X _ _ O O X X X _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ O O O O X O O _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ O _ X _ X O _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ X _ O _ X _ _ X _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ X O _ _ X _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ X O X _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */

//				/*         a b c d e f g h i j k l m n o        */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ O _ X _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ O X _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ X _ _ _ X _ X _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o        */

//				/*         a b c d e f g h i j k l m n o        */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ O _ _ _ _ O _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ X _ _ X _ O _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ X X O _ O O X _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o        */

//				/*        a b c d e f g h i j k l m n o          */
//				/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */" _ _ _ _ _ X X X O _ _ _ _ _ _\n" /*  4 */
//				/*  5 */" _ _ _ _ _ X O X X _ _ _ _ _ _\n" /*  5 */
//				/*  6 */" _ _ _ _ _ X O O O O X _ _ _ _\n" /*  6 */
//				/*  7 */" _ _ _ _ _ O O _ _ _ _ _ _ _ _\n" /*  7 */
//				/*  8 */" _ _ _ _ X O _ _ O _ _ _ _ _ _\n" /*  8 */
//				/*  9 */" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */" _ _ _ _ _ _ _ X O _ _ O _ _ _\n" /* 10 */
//				/* 11 */" _ _ _ _ _ _ _ O X _ _ _ _ _ _\n" /* 11 */
//				/* 12 */" _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*        a b c d e f g h i j k l m n o          */
				);
// @formatter:on

		std::vector<std::pair<int, int>> moves = { { 4, 2 }, { 3, 10 }, { 3, 2 }, { 5, 6 }, { 4, 4 }, { 5, 2 }, { 5, 5 }, { 6, 5 }, { 6, 6 },
				{ 3, 3 }, { 7, 4 }, { 8, 4 }, { 4, 6 }, { 7, 3 }, { 6, 8 }, { 9, 5 }, { 10, 6 }, { 7, 9 }, { 10, 7 }, { 7, 8 }, { 10, 5 }, { 10, 9 },
				{ 8, 9 }, { 7, 7 }, { 7, 10 }, { 9, 8 }, { 8, 7 }, { 10, 4 }, { 6, 10 }, { 8, 6 }, { 11, 3 }, { 6, 7 } };

		Sign sign_to_move = Sign::CROSS;

		for (auto iter = moves.begin(); iter < moves.end(); iter++)
		{
			Board::putMove(board, Move(iter->second, iter->first, sign_to_move));
			sign_to_move = invertSign(sign_to_move);
		}
		std::cout << Board::toString(board, true) << '\n';

//		std::cout << "START 15\n";
//		std::cout << "INFO rule 1\n";
//		std::cout << "INFO timeout_turn 5000\n";
//		std::cout << "INFO timeout_match 300000\n";
//		std::cout << "INFO max_memory 350000000\n";
//		std::cout << get_BOARD_command(board, sign_to_move);
//		return 0;

		player.setBoard(board, sign_to_move);
		return 0;
	}

//	{
//		FileLoader fl("/home/maciek/alphagomoku/new_runs/test2_test/config.json");
//		MasterLearningConfig cfg(fl.getJson());
//
//		for (int i = 0; i < 300; i++)
//		{
//			const int size = cfg.training_config.buffer_size.getValue(i);
//			std::cout << "iteration " << (i + 1) << ", size = " << size << ", from " << std::max(0, i + 1 - size) << " to " << i << '\n';
//		}
//		return 0;
//	}

//	if (false)
//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/retrain_10x128s/valid_buffer/buffer_1.bin");
//		std::cout << buffer.getStats().toString() << '\n';
//
//		std::unique_ptr<AGNetwork> old_network = loadAGNetwork("./old_6x64s.bin");
//		std::unique_ptr<AGNetwork> pvq_network = loadAGNetwork(
//				"/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/libml/minml_pvq_1024/network_99_opt.bin");
//		std::unique_ptr<AGNetwork> pv_network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_62_opt.bin");
////		std::unique_ptr<AGNetwork> pv_network = loadAGNetwork("./old_6x64s.bin");
//
//		old_network->setBatchSize(256);
//		pvq_network->setBatchSize(256);
//		pv_network->setBatchSize(256);
//		old_network->moveTo(ml::Device::cuda(0));
//		pvq_network->moveTo(ml::Device::cuda(0));
//		pv_network->moveTo(ml::Device::cuda(0));
//
//		SearchDataPack pack0(15, 15);
//		SearchDataPack pack1(15, 15);
//		SearchDataPack pack2(15, 15);
//		SearchDataPack pack3(15, 15);
//
//		int count = 0;
//		float old_value_error = 0.0f;
//		float pv_value_error = 0.0f;
//		float pvq_value_error = 0.0f;
//
//		float old_policy_error = 0.0f;
//		float pv_policy_error = 0.0f;
//		float pvq_policy_error = 0.0f;
//
//		for (int i = 0; i < buffer.size(); i++)
//		{
//			const int batch_size = buffer.getGameData(i).numberOfSamples();
//			for (int j = 0; j < batch_size; j++)
//			{
//				buffer.getGameData(i).getSample(pack0, j);
//
//				old_network->packInputData(j, pack0.board, pack0.played_move.sign);
//				pvq_network->packInputData(j, pack0.board, pack0.played_move.sign);
//				pv_network->packInputData(j, pack0.board, pack0.played_move.sign);
//			}
//			old_network->forward(batch_size);
//			pvq_network->forward(batch_size);
//			pv_network->forward(batch_size);
//
//			for (int j = 0; j < batch_size; j++)
//			{
//				buffer.getGameData(i).getSample(pack0, j);
//				buffer.getGameData(i).getSample(pack1, j);
//				buffer.getGameData(i).getSample(pack2, j);
//				buffer.getGameData(i).getSample(pack3, j);
////				std::cout << "\n\n" << j << "--------------------------------------------------------------\n\n";
////				std::cout << toString(pack1.played_move.sign) << " to move (" << pack1.played_move.text() << ")\n";
////				std::cout << "Minimax value = " << pack1.minimax_value.toString() << '\n';
//				old_network->unpackOutput(j, pack1.policy_prior, pack1.action_values, pack1.minimax_value);
////				std::cout << "Old Network\n";
////				std::cout << "Value = " << pack1.minimax_value.toString() << '\n';
////				std::cout << Board::toString(pack1.board, pack1.policy_prior, true) << '\n';
//
//				pvq_network->unpackOutput(j, pack2.policy_prior, pack2.action_values, pack2.minimax_value);
////				std::cout << "PVQ Network\n";
////				std::cout << "Value = " << pack2.minimax_value.toString() << '\n';
////				std::cout << Board::toString(pack2.board, pack2.policy_prior, true) << '\n';
//
//				pv_network->unpackOutput(j, pack3.policy_prior, pack3.action_values, pack3.minimax_value);
////				std::cout << "PV Network\n";
////				std::cout << "Value = " << pack3.minimax_value.toString() << '\n';
////				std::cout << Board::toString(pack3.board, pack3.policy_prior, true) << '\n';
//
//				old_value_error += cross_entropy(pack0.minimax_value, pack1.minimax_value);
//				pvq_value_error += cross_entropy(pack0.minimax_value, pack2.minimax_value);
//				pv_value_error += cross_entropy(pack0.minimax_value, pack3.minimax_value);
//
//				old_policy_error += cross_entropy(pack0.policy_prior.data(), pack1.policy_prior.data(), pack0.policy_prior.size());
//				pvq_policy_error += cross_entropy(pack0.policy_prior.data(), pack2.policy_prior.data(), pack0.policy_prior.size());
//				pv_policy_error += cross_entropy(pack0.policy_prior.data(), pack3.policy_prior.data(), pack0.policy_prior.size());
//				count++;
//			}
//		}
//		std::cout << "old error = " << old_value_error / count << ", " << old_policy_error / count << '\n';
//		std::cout << "pvq error = " << pvq_value_error / count << ", " << pvq_policy_error / count << '\n';
//		std::cout << "pv error  = " << pv_value_error / count << ", " << pv_policy_error / count << '\n';
//
////
////		SearchDataPack pack(15, 15);
////		for (int i = 0; i < 10; i++)
////		{
////			buffer.getGameData(10000).getSample(pack, i);
////			pack.print();
////		}
////
////		GameConfig cfg(GameRules::CARO5, 15);
////		TSSConfig tss_cfg;
////		tss_cfg.mode = 2;
////
////// @formatter:off
////		matrix<Sign> board = Board::fromString(
////		/*         a b c d e f g h i j k l m n o          */
////		/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
////		/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
////		/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
////		/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
////		/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
////		/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
////		/*  6 */ " _ _ _ X X _ X _ _ X X _ _ _ _\n" /*  6 */
////		/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
////		/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
////		/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
////		/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
////		/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
////		/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
////		/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
////		/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
/////*         a b c d e f g h i j k l m n o          */);
////// @formatter:on
////		const Sign sign_to_move = Sign::CROSS;
////
////		PatternCalculator calc(cfg);
////		calc.setBoard(board, sign_to_move);
////		calc.print();
////		calc.printAllThreats();
////
////		SearchDataPack pack(cfg.rows, cfg.cols);
////		for (int i = 0; i < 10; i++)
////		{
////			buffer.getGameData(i).getSample(pack, 0);
//////			pack.print();
////
////			ThreatSpaceSearch tss(cfg, tss_cfg);
////			SearchTask task(cfg);
////			task.set(pack.board, pack.played_move.sign);
////			tss.solve(task, TssMode::RECURSIVE, 1000);
////		}
//		return 0;
//	}

//	for (int i = 0; i < 15; i++)
//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/standard_15x15/valid_buffer/buffer_10.bin");
//
//		SearchDataPack pack(15, 15);
//		buffer.getGameData(0).getSample(pack, 18);
//		pack.print();
//
//		std::vector<double> hist(23);
//		for (int g = 0; g < buffer.size(); g++)
//			hist[static_cast<int>(buffer.getGameData(g).numberOfMoves()) / 10]++;
//
//		double sum = std::accumulate(hist.begin(), hist.end(), 0.0);
//
//		std::cout << "buffer " << i << " :";
//		for (size_t j = 0; j < hist.size(); j++)
//			std::cout << " " << hist[j] / sum;
//		std::cout << '\n';
//	}

//	run_training();
//	std::cout << "END" << std::endl;
//	return 0;

//	transfer_learning();

//	convert_old_network();
	test_search();
//	prepare_proven_dataset();
//	test_proven_positions(1000);
//	train_simple_evaluator();
//	test_evaluate();
//	test_pattern_calculator();
//	test_move_generator();
//	test_proven_search(1000, 1000, true);
//	parameter_tuning();
//	train_nnue();
	return 0;

//	nnue::TrainingNNUE nnue(GameConfig(GameRules::RENJU, 15), 1, "nnue_c5_64x16x16x1.bin");
//	nnue::NNUEWeights weights = nnue.dump();
//	SerializedObject so;
//	Json json = weights.save(so);
//	FileSaver fs("networks/caro5_nnue_64x16x16x1.bin");
//	fs.save(json, so, 2, false);
//	return 0;
//
//	{
//		matrix<Sign> board = Board::fromString(""
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
//				" _ _ _ O _ O _ O _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"
//				" _ _ _ _ _ O X X X X _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//		const Sign sign_to_move = Sign::CIRCLE;
//		nnue.packInputData(0, board, sign_to_move);
//		nnue.forward(1);
//		const float eval = nnue.unpackOutput(0);
//		std::cout << "eval = " << eval << '\n';
////		return 0;
//
//		nnue::InferenceNNUE inf_nnue(GameConfig(GameRules::STANDARD, 15), weights);
//		PatternCalculator calc(GameConfig(GameRules::STANDARD, 15));
//		calc.setBoard(board, sign_to_move);
//		inf_nnue.refresh(calc);
//
////		calc.addMove(Move(Sign::CIRCLE, 8, 10));
////		inf_nnue.update(calc);
//		std::cout << "eval = " << inf_nnue.forward() << '\n';
//
////		Board::putMove(board, Move(Sign::CIRCLE, 8, 10));
////		calc.setBoard(board, invertSign(sign_to_move));
////		inf_nnue.refresh(calc);
//
//		const double start = getTime();
//		int repeats = 0;
//		for (; repeats < 100000000; repeats += 10)
//		{
//			for (int j = 0; j < 10; j++)
////				calc.setBoard(board, sign_to_move);
////				inf_nnue.refresh(calc);
//				inf_nnue.forward();
////				inf_nnue.update(calc);
//			if ((getTime() - start) > 10.0)
//				break;
//		}
//		const double stop = getTime();
//		const double time = stop - start;
//		std::cout << "time = " << time << "s, repeats = " << repeats << '\n';
//		std::cout << "n/s = " << repeats / time << " (" << time / repeats * 1.0e6 << "ms)\n";
//		std::cout << "eval = " << inf_nnue.forward() << '\n';
////		inf_nnue.forward();
//	}

//	test_proven_search(1000, 200, true);
//	for (int i = 0; i < 120; i++)
//	{
//		const std::string path = "freestyle_20x20/train_buffer/buffer_" + std::to_string(i) + ".bin";
//		convert_old_buffer("/home/maciek/alphagomoku/" + path, "/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/" + path);
//	}
//	train_simple_evaluator();
//	return 0;

//	TrainingManager tm("/home/maciek/alphagomoku/new_runs/test9_test/");
//	for (int i = 0; i < 1; i++)
//		tm.runIterationRL();

//	TrainingManager tm("/home/mkozarzewski/alphagomoku/new_runs/sl_res_pv_8x64s/", "/home/mkozarzewski/alphagomoku/new_runs/btl_pv_8x128s/");
//	for (int i = 0; i < 200; i++)
//		tm.runIterationSL();
//	return 0;

//	TrainingManager tm("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/history1/", "/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/standard_15x15/");

//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/test_test/train_buffer/buffer_0.bin");
//		SearchDataPack pack(15, 15);
//		SamplerVisits sampler;
//		sampler.init(buffer, 1);
//		TrainingDataPack target(15, 15);
//		sampler.get(target);
////		for (int i = 0; i < 10;i++)//buffer.getGameData(1000).numberOfSamples(); i++)
////		{
////			buffer.getGameData(1000).getSample(pack, i);
////			pack.print();
////		}
//		return 0;
//	}

//	test_expand();

//	return 0;

//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/valid_buffer/buffer_200.bin");
//		std::cout << buffer.getStats().toString() << '\n';
//
//		std::unique_ptr<AGNetwork> network_fp32, network_fp16;
//		network_fp32 = loadAGNetwork("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_200_opt.bin");
//		network_fp16 = loadAGNetwork("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_200_opt.bin");
//
//		network_fp32->setBatchSize(1);
//		network_fp32->moveTo(ml::Device::cuda(0));
//
//		network_fp16->setBatchSize(1);
//		network_fp16->moveTo(ml::Device::cpu());
//		network_fp16->convertToHalfFloats();
//
//		SearchDataPack pack(15, 15);
//
//		matrix<Value> action_values;
//		matrix<float> policy_fp32(15, 15), policy_fp16(15, 15);
//		Value value_fp32, value_fp16;
//
//		float ce_policy = 0.0f, ce_value = 0.0f;
//		int count = 0;
//		for (int i = 0; i < buffer.numberOfGames() / 10; i++)
//		{
//			std::cout << i << "/" << buffer.numberOfGames() / 10 << '\n';
//			for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
//			{
//				buffer.getGameData(i).getSample(pack, j);
//				network_fp32->packInputData(0, pack.board, pack.played_move.sign);
//				network_fp16->packInputData(0, pack.board, pack.played_move.sign);
//
//				network_fp32->forward(1);
//				network_fp16->forward(1);
//
//				network_fp32->unpackOutput(0, policy_fp32, action_values, value_fp32);
//				network_fp16->unpackOutput(0, policy_fp16, action_values, value_fp16);
//
////				std::cout << value_fp32.toString() << "\n" << Board::toString(pack.board, policy_fp32, true) << '\n';
////				std::cout << value_fp16.toString() << "\n" << Board::toString(pack.board, policy_fp16, true) << '\n';
//
//				ce_policy += cross_entropy(policy_fp32.data(), policy_fp16.data(), policy_fp32.size());
//				ce_value += cross_entropy(value_fp32, value_fp16);
//				count++;
//			}
//			std::cout << "diff policy = " << ce_policy / count << '\n';
//			std::cout << "diff value  = " << ce_value / count << '\n';
//		}
//
//		return 0;
//	}

	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/train_buffer/buffer_34.bin");
//		std::cout << buffer.getStats().toString() << '\n';

		GameConfig game_config(GameRules::STANDARD, 15, 15);
		TrainingConfig cfg;
		cfg.blocks = 8;
		cfg.filters = 128;
//		std::unique_ptr<AGNetwork> network;
		std::unique_ptr<AGNetwork> network = std::make_unique<BottleneckPoolingPVraw>();
//		network = loadAGNetwork("./old_6x64s.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_90_opt.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/test_sampler_v2/checkpoint/network_94_opt.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_255_opt.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/supervised/broadcast_skip2_relu_8x128s/network_99_opt.bin");

		network->init(game_config, cfg);
		network->optimize();
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_60_opt.bin");
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_60_opt.bin");
//		network.optimize();
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/checkpoint/network_33_opt.bin");
//		network->optimize();
		network->setBatchSize(12);
		network->moveTo(ml::Device::cpu());
//		network->moveTo(ml::Device::cuda(0));
		network->convertToHalfFloats();

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
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
				" _ _ _ O _ O _ O _ _ _ _ _ _ _\n"
				" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"
				" _ _ _ _ _ O X X X X _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");

//		board = Board::fromString(""
//		/*        a b c d e f g h i j k l m n o          */
//		/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */" _ _ _ _ X _ O O O _ X _ _ _ _\n" /*  7 */
//				/*  8 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//		/*        a b c d e f g h i j k l m n o          */);

		const Sign sign_to_move = Sign::CIRCLE;
//		PatternCalculator calc(game_config);
//		calc.setBoard(board, sign_to_move);
//		calc.printAllThreats();
//		calc.printForbiddenMoves();
//		network.packInputData(0, board, sign_to_move);

		SearchDataPack pack(15, 15);
		const int game_index = 0;
		const int sample_index = 0;

		matrix<float> policy(15, 15);
		matrix<Value> action_values(15, 15);
		Value value;

//		network.packInputData(0, pack.board, pack.played_move.sign);
		for (int i = 0; i < network->getBatchSize(); i++)
			network->packInputData(i, board, sign_to_move);
		network->forward(1);
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//		network->forward(2);
//		return 0;

		const double start = getTime();
		int repeats = 0;
		for (; repeats < 10000; repeats++)
		{
			network->forward(network->getBatchSize());
			if ((getTime() - start) > 20.0)
				break;
		}
		const double stop = getTime();
		const double time = stop - start;
		std::cout << "time = " << time << "s, repeats = " << repeats << '\n';
		std::cout << "n/s = " << network->getBatchSize() * repeats / time << '\n';
//		return 0;
		network->unpackOutput(0, policy, action_values, value);
		std::cout << "\n\n--------------------------------------------------------------\n\n";
		std::cout << "Value = " << value.toString() << '\n'; //<< " (" << value_target.toString() << ")\n";
		std::cout << Board::toString(board, policy, true) << '\n';
//		std::cout << Board::toString(board, action_values, true) << '\n';

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

//	AGNetwork network(GameConfig(GameRules::STANDARD, 15, 15));
////	network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/checkpoint/network_20_opt.bin");
//	network.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3_10x128.bin");
//	network.optimize();
////	network.convertToHalfFloats();
//
////	network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/network_int8.bin");
//////	return 0;
//////	network.optimize();
//	network.setBatchSize(10);
//	network.moveTo(ml::Device::cuda(0));
////
//	matrix<Sign> board(15, 15);
//	board = Board::fromString(""
//			" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ X X O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ O O _ X _ X _ _ _ _ _\n"
//			" _ _ O X O O O O X _ _ _ _ _ _\n"
//			" _ _ _ X _ O _ O X X _ _ _ _ _\n"
//			" _ _ _ _ X X _ _ _ X _ _ _ _ _\n"
//			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
////	board.at(7, 7) = Sign::CIRCLE;
//	const Sign sign_to_move = Sign::CIRCLE;

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
}

