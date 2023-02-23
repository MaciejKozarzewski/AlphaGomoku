/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

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

#include <iostream>
#include <cstdlib>
#include <numeric>
#include <memory>
#include <functional>
#include <x86intrin.h>

using namespace ag;

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

	GameBuffer buffer;
#ifdef NDEBUG
	for (int i = 20; i <= 24; i++)
#else
	for (int i = 24; i <= 24; i++)
#endif
		buffer.load("/home/maciek/alphagomoku/new_runs_2023/buffer_" + std::to_string(i) + ".bin");
//		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	PatternCalculator extractor_new(game_config);
	PatternCalculator extractor_new2(game_config);

	matrix<Sign> board(game_config.rows, game_config.cols);

	int count = 0;
	for (int i = 0; i < buffer.size(); i++)
	{
		if (i % (buffer.size() / 10) == 0)
			std::cout << i << " / " << buffer.size() << '\n';

		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
		{
			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
			sample.getBoard(board);
			extractor_new.setBoard(board, sample.getMove().sign);

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
				if (board.at(x, y) == Sign::NONE)
				{
					Sign sign = static_cast<Sign>(randInt(1, 3));
					extractor_new.addMove(Move(x, y, sign));
					board.at(x, y) = sign;
				}
				else
				{
					extractor_new.undoMove(Move(x, y, board.at(x, y)));
					board.at(x, y) = Sign::NONE;
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

	GameBuffer buffer;
#ifdef NDEBUG
	for (int i = 20; i <= 24; i++)
#else
	for (int i = 24; i <= 24; i++)
#endif
		buffer.load("/home/maciek/alphagomoku/new_runs_2023/buffer_" + std::to_string(i) + ".bin");
//		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	ThreatSpaceSearch ts_search(game_config, pos);
	std::shared_ptr<SharedHashTable<4>> sht = std::make_shared<SharedHashTable<4>>(game_config.rows, game_config.cols, 1048576);
	ts_search.setSharedTable(sht);

	SearchTask task(game_config);
	matrix<Sign> board(game_config.rows, game_config.cols);
	int total_samples = 0;
	int solved_count = 0;

	for (int i = 0; i < buffer.size(); i++)
//	int i = 9;
	{
		if (i % (buffer.size() / 10) == 0)
			std::cout << i << " / " << buffer.size() << " " << solved_count << "/" << total_samples << '\n';

		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		int j = 0;
		{
//			std::cout << i << " " << j << '\n';
			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
			sample.getBoard(board);
			const Sign sign_to_move = sample.getMove().sign;

			task.set(board, sign_to_move);
			sht->increaseGeneration();
			ts_search.solve(task, TssMode::RECURSIVE, pos);
			solved_count += task.getScore().isProven();
		}
	}

	std::cout << "TSS\n";
	std::cout << "solved " << solved_count << " samples (" << 100.0f * solved_count / total_samples << "%)\n";
	ts_search.print_stats();
}

void test_solver(int pos)
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::STANDARD, 15);

	GameBuffer buffer;
#ifdef NDEBUG
	for (int i = 0; i < 17; i++)
#else
	for (int i = 0; i < 1; i++)
#endif
		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
//		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	matrix<Sign> board(game_config.rows, game_config.cols);
	int total_samples = 0;

	std::fstream output("/home/maciek/Desktop/win_all.txt", std::fstream::out);

	for (int i = 0; i < buffer.size(); i++)
	{
		if (i % (buffer.size() / 10) == 0)
			std::cout << i << " / " << buffer.size() << '\n';

		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
		{
			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
			sample.getBoard(board);
			const Sign sign_to_move = sample.getMove().sign;
			output << get_BOARD_command(board, sign_to_move) << "nodes 0\n";
		}
	}
}

void test_forbidden_moves()
{
	GameConfig game_config(GameRules::RENJU, 15);

	GameBuffer buffer;
//#ifdef NDEBUG
	for (int i = 0; i <= 21; i++)
//#else
//		for (int i = 0; i < 1; i++)
//#endif
		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	PatternCalculator calc(game_config);

	matrix<Sign> board(game_config.rows, game_config.cols);
	int total_samples = 0;

	const double start = getTime();
	for (int i = 0; i < buffer.size(); i++)
//	int i = 143634;
	{
		if (i % (buffer.size() / 100) == 0)
			std::cout << i << " / " << buffer.size() << '\n';

		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		int j = 0;
		{
			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
			sample.getBoard(board);
			const Sign sign_to_move = sample.getMove().sign;

			calc.setBoard(board, sign_to_move);
			for (int r = 0; r < game_config.rows; r++)
				for (int c = 0; c < game_config.cols; c++)
				{
					const bool b0 = isForbidden(board, Move(r, c, Sign::CROSS));
					const bool b1 = calc.isForbidden(Sign::CROSS, r, c);
					if (b0 != b1)
					{
						std::cout << "mismatch\n";
						return;
					}
				}

//			bool flag = false;
//			IsOverline is_overline(game_config.rules, Sign::CIRCLE);
//			for (int r = 0; r < game_config.rows; r++)
//			{
//				if (flag == false)
//					for (int c = 0; c < game_config.cols; c++)
//						for (int d = 0; d < 4; d++)
//							if (is_overline(Pattern(11, calc.getNormalPatternAt(r, c, d))))
//							{
//								flag = true;
//								break;
//							}
//			}
		}
	}

	std::cout << "time " << (getTime() - start) << ", samples = " << total_samples << '\n';
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
	search_config.solver_level = 2;
	search_config.solver_max_positions = 100;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
//#ifdef NDEBUG
	device_config.device = ml::Device::cuda(0);
//#else
//	device_config.device = ml::Device::cpu();
//#endif
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/test5_15x15_standard/checkpoint/network_32_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64wdl_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku532/networks/standard_10x128.bin");
	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/puct_network_28_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku521/networks/freestyle_12x12.bin");
//	nn_evaluator.loadGraph("C:\\Users\\Maciek\\Desktop\\network_75_opt.bin");

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

// @formatter:off
	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
									" _ _ _ X _ X _ O _ _ _ _ _ _ _\n"
									" _ _ _ O O X O _ _ _ _ O _ _ _\n"
									" _ _ _ _ X X X O _ _ X _ _ _ _\n"
									" _ _ _ O X O O O O X _ _ _ _ _\n"
									" _ _ _ _ _ X O X X _ _ _ _ _ _\n"
									" _ _ _ _ _ X O X _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ O _ _ _ X _ _ _ _\n"
									" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
								/*    a b c d e f g h i j k l m n o        */); // Oh2 or Of2 is a win
// @formatter:on
	sign_to_move = Sign::CIRCLE;

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
//	sign_to_move = Sign::CROSS;

// @formatter:off
	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ O O _ _ _ _ _ _ _ _\n"
									" _ _ X _ O _ O O X _ _ _ _ _ _\n"
									" _ _ _ O _ X _ O _ _ _ O _ _ _\n"
									" _ _ _ X O _ X X X O X _ _ _ _\n"
									" _ _ _ O X O O O O X X _ _ _ _\n"
									" _ _ _ _ _ X O X X _ _ _ _ _ _\n"
									" _ _ _ _ _ X O X _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ O _ _ _ X _ _ _ _\n"
									" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
								/*    a b c d e f g h i j k l m n o          */); // position is a loss
// @formatter:on
	sign_to_move = Sign::CROSS;

// @formatter:off
	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 0
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 1
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 2
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 3
									" _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" // 4
									" _ _ _ _ O _ _ _ X _ _ _ _ _ _\n" // 5
									" _ _ _ O _ O X O _ _ _ _ _ _ _\n" // 6
									" _ _ _ _ _ X O _ X _ _ _ _ _ _\n" // 7
									" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n" // 8
									" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n" // 9
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //10
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //11
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //12
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //13
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //14
								/*    a b c d e f g h i j k l m n o          */);
// @formatter:on
	sign_to_move = Sign::CROSS;

// @formatter:off
	board = Board::fromString(	/*    a b c d e f g h i j k l m n o        */
									" _ _ _ _ _ _ _ _ O X _ _ _ _ _\n" // 0
									" _ _ _ _ _ _ _ _ _ _ _ _ X O _\n" // 1
									" _ _ _ _ _ _ _ _ _ _ _ _ X _ _\n" // 2
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" // 3
									" _ _ _ _ _ _ _ _ _ O _ X _ _ O\n" // 4
									" _ _ _ _ _ _ _ _ _ _ X O _ _ _\n" // 5
									" _ _ _ _ _ _ X _ _ O O X X X _\n" // 6
									" _ _ _ _ _ _ _ _ _ X O _ O _ _\n" // 7
									" _ _ _ _ _ _ _ _ _ _ X O _ _ _\n" // 8
									" _ _ _ _ _ _ _ _ _ _ O _ X _ _\n" // 9
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //10
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //11
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //12
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //13
									" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" //14
								/*    a b c d e f g h i j k l m n o          */);
// @formatter:on
	sign_to_move = Sign::CIRCLE;

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

// @formatter:off
	board = Board::fromString(
			/*        a b c d e f g h i j k l m n o         */
			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
			/*  6 */" _ _ _ _ _ O _ X X _ _ _ _ _ _\n"/*  6 */
			/*  7 */" _ _ _ _ _ _ O X O _ _ _ _ _ _\n"/*  7 */
			/*  8 */" _ _ _ _ _ O X X X O _ _ _ _ _\n"/*  8 */
			/*  9 */" _ _ _ _ _ O X O _ O _ _ _ _ _\n"/*  9 */
			/* 10 */" _ _ _ _ O X X X _ _ _ _ _ _ _\n"/* 10 */
			/* 11 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"/* 11 */
			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
			/*        a b c d e f g h i j k l m n o         */);
// @formatter:on
	sign_to_move = Sign::CROSS;

// @formatter:off
	board = Board::fromString(
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
			" _ _ _ X O _ _ O X _ _ _ _ _ _\n"
			" _ _ _ O _ O X O X _ _ _ _ _ _\n"
			" _ _ _ _ X X O _ X _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ X O _ _ _ _ _ _\n"
			" _ _ _ _ X O O X _ _ X _ _ _ _\n"
			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
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

	GameConfig cfg(GameRules::STANDARD, 15, 15);
	ThreatSpaceSearch ts_search(cfg, 1000000);
	std::shared_ptr<SharedHashTable<4>> sht = std::make_shared<SharedHashTable<4>>(cfg.rows, cfg.cols, 1048576);
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
	for (int j = 0; j <= 10000; j++)
	{
		if (tree.getSimulationCount() >= next_step)
		{
			std::cout << tree.getSimulationCount() << " ..." << std::endl;
			next_step += 100000;
		}
		search.select(tree, 10000);
		search.solve();
		search.scheduleToNN(nn_evaluator);
		nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
//		search.tune();

		if (tree.isRootProven())
			break;
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
	std::string path = "/home/maciek/Desktop/test_tm/";
	MasterLearningConfig config(FileLoader(path + "config.json").getJson());

	GeneratorManager manager(config.game_config, config.generation_config);
//	manager.generate(path + "freestyle_10x128.bin", 10000, 0);

	const GameBuffer &buffer = manager.getGameBuffer();
	buffer.save(path + "buffer_1000f.bin");

//	GameBuffer buffer(path + "buffer.bin");

	Json stats(JsonType::Array);
	matrix<Sign> board(config.game_config.rows, config.game_config.cols);

	for (int i = 0; i < buffer.size(); i++)
	{
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		int nb_moves = Board::numberOfMoves(board);
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
		{
			Value value = buffer.getFromBuffer(i).getSample(j).getMinimaxValue();
//			ProvenValue pv = buffer.getFromBuffer(i).getSample(j).getProvenValue();

			Json entry;
			entry["move"] = nb_moves + j;
			entry["winrate"] = value.win;
			entry["drawrate"] = value.draw;
//			entry["proven value"] = toString(pv);
			stats[i][j] = entry;
		}
	}

	FileSaver fs(path + "stats_1000f.json");
	fs.save(stats, SerializedObject());
	std::cout << "END" << std::endl;
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
	MasterLearningConfig config(FileLoader("/home/maciek/alphagomoku/new_runs_2023/config.json").getJson());
	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

	SelfplayConfig cfg(config.evaluation_config.selfplay_options);
	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/new_runs_2023/puct_network_28_opt.bin", "bugged");

	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs_2023/network_11_opt.bin", "correct");

	const double start = getTime();
	manager.generate(1000);
	const double stop = getTime();
	std::cout << "generated in " << (stop - start) << '\n';
	std::string to_save;
	for (int i = 0; i < manager.numberOfThreads(); i++)
		to_save += manager.getGameBuffer(i).generatePGN();
	std::ofstream file("/home/maciek/alphagomoku/new_runs_2023/correct.pgn", std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();
}

int main(int argc, char *argv[])
{
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
//	test_proven_positions(100);
//	test_proven_positions(1000);
//	ab_search_test();
//	test_search();
	test_evaluate();
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
	matrix<Sign> board = Board::fromString( " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ O _ O _ _ _ _ _ _ _\n"
			" _ _ _ _ O _ X _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ X _ X _ _ _ _ _ _ _\n"
			" _ _ _ _ X O X O X _ _ _ _ _ _\n"
			" _ _ _ X _ O O X _ X _ _ _ _ _\n"
			" _ _ O _ O X _ X X O O _ _ _ _\n"
			" _ _ _ _ _ O _ X _ O _ _ _ _ _\n"
			" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ O _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");// @formatter:on
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

	ag::ProgramManager player_manager;
	try
	{
		bool can_continue = player_manager.processArguments(argc, argv);
		if (can_continue)
		{
			player_manager.run();
			ag::Logger::write(ag::ProgramInfo::name() + " successfully exits");
		}
		return 0;
	} catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		ag::Logger::write(e.what());
	}
	return -1;
}

