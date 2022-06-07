/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/version.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor_v2.hpp>
#include <alphagomoku/vcf_solver/FeatureExtractor_v3.hpp>
#include <alphagomoku/vcf_solver/FeatureTable_v3.hpp>
#include <alphagomoku/solver/PatternCalculator.hpp>
#include <alphagomoku/solver/PatternTable.hpp>
#include <alphagomoku/solver/VCTSolver.hpp>
#include <alphagomoku/vcf_solver/VCTSolver.hpp>
#include <libml/utils/ZipWrapper.hpp>

#include <iostream>
#include <numeric>
#include <functional>

using namespace ag;

void add_edge(SearchTask &task, Move move, float policyPrior = 0.0f, Value actionValue = Value(), ProvenValue pv = ProvenValue::UNKNOWN)
{
	task.addEdge(move);
	task.getEdges().back().setPolicyPrior(policyPrior);
	task.getEdges().back().setValue(actionValue);
	task.getEdges().back().setProvenValue(pv);
}

std::string get_BOARD_command(const matrix<Sign> &board, Sign signToMove)
{
	std::string result = "BOARD\n";
	for (int r = 0; r < board.rows(); r++)
		for (int c = 0; c < board.cols(); c++)
			if (board.at(r, c) != Sign::NONE)
				result += std::to_string(c) + ',' + std::to_string(r) + ',' + (board.at(r, c) == signToMove ? '1' : '2') + '\n';
	return result + "DONE\n";
}

template<class SolverType>
class SolverSearch
{
		SearchTask m_task;
		SolverType m_vcf_solver;
	public:
		SolverSearch(GameConfig gameConfig, int maxPositions) :
				m_task(gameConfig.rules),
				m_vcf_solver(gameConfig, maxPositions)
		{
			m_task.set(matrix<Sign>(gameConfig.rows, gameConfig.cols), Sign::CROSS);
			m_vcf_solver.solve(m_task, 0);
		}
		bool solve(const matrix<Sign> &board, Sign signToMove)
		{
			m_task.set(board, signToMove);
			m_vcf_solver.solve(m_task, 2);
//			std::cout << m_task.toString() << '\n';
			return m_task.isReady();
		}
		void printStats()
		{
			m_vcf_solver.print_stats();
		}
};

class NNSearch
{
		Tree m_tree;
		NNEvaluator m_nn_evaluator;
		Search m_search;
	public:
		NNSearch(GameConfig gameConfig, TreeConfig treeConfig, SearchConfig SearchConfig, DeviceConfig deviceConfig) :
				m_tree(treeConfig),
				m_nn_evaluator(deviceConfig),
				m_search(gameConfig, SearchConfig)
		{
			m_nn_evaluator.useSymmetries(true);
		}
		void loadNetwork(const std::string &path)
		{
			m_nn_evaluator.loadGraph(path);
		}
		void setup(const matrix<Sign> &board, Sign signToMove)
		{
			matrix<Sign> tmp(board);
			tmp.fill(Sign::CIRCLE);
			m_tree.setBoard(tmp, Sign::CIRCLE);

			m_tree.setBoard(board, signToMove);
			m_tree.setEdgeSelector(PuctSelector(1.25f));
			m_tree.setEdgeGenerator(SolverGenerator(m_search.getConfig().expansion_prior_treshold, m_search.getConfig().max_children));
		}
		bool search(int simulations, bool printProgress = false)
		{
			int next_step = 0;
			for (int j = 0; j <= simulations; j++)
			{
				if (m_tree.getSimulationCount() >= next_step and printProgress)
				{
					std::cout << m_tree.getSimulationCount() << " ...\n";
					next_step += (simulations / 10);
				}
				m_search.select(m_tree, simulations);
				m_search.solve();

				m_search.scheduleToNN(m_nn_evaluator);
				m_nn_evaluator.evaluateGraph();

				m_search.generateEdges(m_tree);
				m_search.expand(m_tree);
				m_search.backup(m_tree);
				m_search.tune();

				if (m_tree.isProven())
					break;
			}
			m_search.cleanup(m_tree);
			return m_tree.isProven();
		}
		void printStats() const
		{
			std::cout << m_search.getStats().toString() << '\n';
			std::cout << m_tree.getNodeCacheStats().toString() << '\n';
			std::cout << m_nn_evaluator.getStats().toString() << '\n';
		}
		int getPositions() const
		{
			return m_tree.getSimulationCount();
		}
};

void test_feature_extractor()
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

	FeatureExtractor_v2 extractor_old(game_config);
//	FeatureExtractor_v3 extractor_new(game_config);
//	FeatureExtractor_v3 extractor_new2(game_config);
	ag::experimental::PatternCalculator extractor_new(game_config);

	matrix<Sign> board(game_config.rows, game_config.cols);
	extractor_old.setBoard(board, Sign::CROSS);

	for (int i = 0; i < buffer.size(); i++)
	{
		if (i % (buffer.size() / 10) == 0)
			std::cout << i << " / " << buffer.size() << '\n';

		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
		{
			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
			sample.getBoard(board);
			extractor_new.setBoard(board);
			extractor_old.setBoard(board, sample.getMove().sign);
			for (int k = 0; k < 100; k++)
			{
				int x = randInt(game_config.rows);
				int y = randInt(game_config.cols);
				if (board.at(x, y) == Sign::NONE)
				{
					Sign sign = static_cast<Sign>(randInt(1, 3));
					extractor_new.addMove(Move(x, y, sign));
					extractor_old.addMove(Move(x, y, sign));
					board.at(x, y) = sign;
				}
				else
				{
					extractor_new.undoMove(Move(x, y, board.at(x, y)));
					extractor_old.undoMove(Move(x, y, board.at(x, y)));
					board.at(x, y) = Sign::NONE;
				}
			}
//			extractor_new2.setBoard(board, sample.getMove().sign);
//			for (int x = 0; x < game_config.rows; x++)
//				for (int y = 0; y < game_config.cols; y++)
//				{
//					for (int dir = 0; dir < 4; dir++)
//					{
//						if (extractor_new2.getRawFeatureAt(x, y, static_cast<Direction>(dir))
//								!= extractor_new.getRawFeatureAt(x, y, static_cast<Direction>(dir)))
//						{
//							std::cout << "Raw feature mismatch\n";
//							std::cout << "Single step\n";
//							extractor_new2.printRawFeature(x, y);
//							std::cout << "incremental\n";
//							extractor_new.printRawFeature(x, y);
//							exit(-1);
//						}
//						if (extractor_new2.getFeatureTypeAt(Sign::CROSS, x, y, static_cast<Direction>(dir))
//								!= extractor_new.getFeatureTypeAt(Sign::CROSS, x, y, static_cast<Direction>(dir))
//								or extractor_new2.getFeatureTypeAt(Sign::CIRCLE, x, y, static_cast<Direction>(dir))
//										!= extractor_new.getFeatureTypeAt(Sign::CIRCLE, x, y, static_cast<Direction>(dir)))
//						{
//							std::cout << "Feature type mismatch\n";
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
//						extractor_new2.printRawFeature(x, y);
//						std::cout << "incremental\n";
//						extractor_new.printRawFeature(x, y);
//						exit(-1);
//					}
//				}
		}
	}
//	extractor_new2.print();
//	extractor_new2.printAllThreats();

	std::cout << "Old extractor\n";
	extractor_old.print_stats();
	std::cout << "New extractor\n";
	extractor_new.print_stats();
}

void test_proven_positions(int pos)
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

	SolverSearch<FeatureExtractor> solver_v1(game_config, pos);
	SolverSearch<VCFSolver> solver_v2(game_config, pos);
	SolverSearch<VCTSolver> solver_v3(game_config, pos);
	SolverSearch<ag::experimental::VCTSolver> solver_v4(game_config, pos);

	matrix<Sign> board(game_config.rows, game_config.cols);
	int total_samples = 0;
	int v1_solved = 0, v2_solved = 0, v3_solved = 0, v4_solved = 0;

	int v2_solved_v3_not = 0;
	int v3_solved_v2_not = 0;
	int v4_solved_v3_not = 0;
	int v3_solved_v4_not = 0;

	for (int i = 0; i < buffer.size(); i++)
//	for (int i = 26; i <= 26; i++)
	{
		if (i % (buffer.size() / 10) == 0)
			std::cout << i << " / " << buffer.size() << '\n';

		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		for (int j = 0; j <= 0; j++)
		{
			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
			sample.getBoard(board);
//			board.at(7, 3) = Sign::CROSS;
//			board.at(7, 2) = Sign::CIRCLE;
			const Sign sign_to_move = sample.getMove().sign;

//			bool is_solved_v1 = solver_v1.solve(board, sign_to_move);
//			v1_solved += is_solved_v1;

			bool is_solved_v2 = solver_v2.solve(board, sign_to_move);
			v2_solved += is_solved_v2;
//			std::cout << "\n\n\n------------------------------------------------------------\n\n\n";

			bool is_solved_v3 = solver_v3.solve(board, sign_to_move);
			v3_solved += is_solved_v3;

			bool is_solved_v4 = solver_v4.solve(board, sign_to_move);
			v4_solved += is_solved_v4;

			if (is_solved_v2 and not is_solved_v3)
			{
//				std::cout << i << " " << j << '\n';
//				std::cout << Board::toString(board, true);
				v2_solved_v3_not++;
//				break;
			}
			if (is_solved_v3 and not is_solved_v2)
			{
//				std::cout << i << " " << j << '\n';
//				std::cout << Board::toString(board, true);
				v3_solved_v2_not++;
//				break;
			}
			if (is_solved_v4 and not is_solved_v3)
			{
//				std::cout << i << " " << j << '\n';
//				std::cout << Board::toString(board, true);
				v4_solved_v3_not++;
//				break;
			}
			if (is_solved_v3 and not is_solved_v4)
			{
//				std::cout << i << " " << j << '\n';
//				std::cout << Board::toString(board, true);
				v3_solved_v4_not++;
//				return;
			}
		}
	}
	std::cout << "FeatureExtractor\n";
	std::cout << "solved " << v1_solved << " samples (" << 100.0f * v1_solved / total_samples << "%)\n";
	solver_v1.printStats();

	std::cout << "\nVCFSolver\n";
	std::cout << "solved " << v2_solved << " samples (" << 100.0f * v2_solved / total_samples << "%)\n";
	solver_v2.printStats();

	std::cout << "VCFSolver_v3\n";
	std::cout << "solved " << v3_solved << " samples (" << 100.0f * v3_solved / total_samples << "%)\n";
	solver_v3.printStats();

	std::cout << "VCFSolver_v4\n";
	std::cout << "solved " << v4_solved << " samples (" << 100.0f * v4_solved / total_samples << "%)\n";
	solver_v4.printStats();

	std::cout << "v2 solved, while v3 didn't " << v2_solved_v3_not << '\n';
	std::cout << "v3 solved, while v2 didn't " << v3_solved_v2_not << '\n';
	std::cout << "v4 solved, while v3 didn't " << v4_solved_v3_not << '\n';
	std::cout << "v3 solved, while v4 didn't " << v3_solved_v4_not << '\n';
}

void test_search()
{
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::FREESTYLE, 20);

	TreeConfig tree_config;
	Tree tree(tree_config);

	SearchConfig search_config;
	search_config.max_batch_size = 32;
	search_config.exploration_constant = 1.25f;
	search_config.noise_weight = 0.0f;
	search_config.expansion_prior_treshold = 1.0e-4f;
	search_config.max_children = 30;
	search_config.vcf_solver_level = 2;
	search_config.vcf_solver_max_positions = 100;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
	device_config.device = ml::Device::cuda(0);
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(true);
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/test5_15x15_standard/checkpoint/network_32_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64wdl_opt.bin");
	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku531/networks/standard_10x128.bin");
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
//			/*        a b c d e f g h i j k l m n o          */);                                                                                                             // @formatter:on
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
//			/*        a b c d e f g h i j k l m n o          */); // @formatter:on
//
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
// @formatter:off
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
//			/*  6 */" _ _ _ _ _ O X X X _ _ _ _ _ _\n"/*  6 */
//			/*  7 */" _ _ _ _ _ O O X O _ _ _ _ _ _\n"/*  7 */
//			/*  8 */" _ _ _ _ _ O X X X O _ _ _ _ _\n"/*  8 */
//			/*  9 */" _ _ _ _ _ O X O _ O _ _ _ _ _\n"/*  9 */
//			/* 10 */" _ _ _ _ O X X X _ _ _ _ _ _ _\n"/* 10 */
//			/* 11 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"/* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
//			/*        a b c d e f g h i j k l m n o         */); // @formatter:on
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

// @formatter:off
	board = Board::fromString(	/*         a b c d e f g h i j k l m n o        */
								/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
								/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
								/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
								/*  3 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  3 */
								/*  4 */ " _ _ _ _ _ _ _ _ O X X _ X _ _\n" /*  4 */
								/*  5 */ " _ _ _ _ _ _ _ _ _ X O _ _ _ _\n" /*  5 */
								/*  6 */ " _ _ _ _ _ _ _ _ _ X O _ X _ _\n" /*  6 */
								/*  7 */ " _ _ _ _ _ _ _ _ _ _ O _ X _ _\n" /*  7 */
								/*  8 */ " _ _ _ _ _ _ _ _ _ _ X O O _ _\n" /*  8 */
								/*  9 */ " _ _ _ _ _ _ _ _ _ O _ X _ _ _\n" /*  9 */
								/* 10 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /* 10 */
								/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
								/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
								/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
								/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
								/*         a b c d e f g h i j k l m n o          */);    // @formatter:on
	sign_to_move = Sign::CIRCLE;

	ag::experimental::VCTSolver solver(game_config, search_config.vcf_solver_max_positions);
	SearchTask task(game_config.rules);
	task.set(board, sign_to_move);
	solver.solve(task, 2);
	solver.print_stats();
//	return;

	Search search(game_config, search_config);
	tree.setBoard(board, sign_to_move);
	tree.setEdgeSelector(PuctSelector(1.25f));
	tree.setEdgeGenerator(SolverGenerator(search_config.expansion_prior_treshold, search_config.max_children));

	int next_step = 0;
	for (int j = 0; j <= 100000; j++)
	{
		if (tree.getSimulationCount() >= next_step)
		{
			std::cout << tree.getSimulationCount() << " ..." << std::endl;
			next_step += 10000;
		}
		search.select(tree, 100000);
		search.solve();

		search.scheduleToNN(nn_evaluator);
		nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
		search.tune();

		if (tree.isProven())
			break;
	}
	search.cleanup(tree);

	tree.printSubtree(1, true, 10);
	std::cout << search.getStats().toString() << '\n';
	std::cout << "memory = " << (tree.getMemory() >> 20) << "MB\n\n";
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

	for (int i = 0; i < task.visitedPathLength(); i++)
	{
		if (i < 10 and task.visitedPathLength() >= 10)
			std::cout << " ";
		if (i < 100 and task.visitedPathLength() >= 100)
			std::cout << " ";
		std::cout << i << " : " << task.getPair(i).edge->getMove().toString() << " : " << task.getPair(i).edge->toString() << '\n';
	}

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
	manager.generate(path + "freestyle_10x128.bin", 10000, 0);

	const GameBuffer &buffer = manager.getGameBuffer();
	buffer.save(path + "buffer_1000f.bin");

//	GameBuffer buffer(path + "buffer.bin");

	Json stats(JsonType::Array);
	matrix<Sign> board(config.game_config.rows, config.game_config.cols);

	for (int i = 0; i < buffer.size(); i++)
	{
		buffer.getFromBuffer(i).getSample(0).getBoard(board);
		int nb_moves = Board::numberOfMoves(board);
		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
		{
			Value value = buffer.getFromBuffer(i).getSample(j).getMinimaxValue();
			ProvenValue pv = buffer.getFromBuffer(i).getSample(j).getProvenValue();

			Json entry;
			entry["move"] = nb_moves + j;
			entry["winrate"] = value.win;
			entry["drawrate"] = value.draw;
			entry["proven value"] = toString(pv);
			stats[i][j] = entry;
		}
	}

	FileSaver fs(path + "stats_1000f.json");
	fs.save(stats, SerializedObject());
	std::cout << "END" << std::endl;
}

void network_only_player(const std::string &mode, int argc, char *argv[])
{
	assert(mode == "policy" || mode == "value");
	ArgumentParser argument_parser;
	argument_parser.parseArguments(argc, argv);

	InputListener input_listener(std::cin);
	OutputSender output_sender(std::cout);

	MessageQueue input_queue;
	MessageQueue output_queue;
	GomocupProtocol protocol(input_queue, output_queue);

	DeviceConfig device_options;
	device_options.device = ml::Device::cuda(randInt(2));
	if (mode == "policy")
		device_options.batch_size = 8;
	else
		device_options.batch_size = 128;
	NNEvaluator nn_evaluator(device_options);

	GameRules rules = GameRules::FREESTYLE;
	matrix<Sign> board;
	Sign sign_to_move = Sign::CROSS;
	while (true)
	{
		protocol.processInput(input_listener);

		while (input_queue.isEmpty() == false)
		{
			const Message input_message = input_queue.pop();
			switch (input_message.getType())
			{
				case MessageType::START_PROGRAM:
				{
					board.clear();
					protocol.reset();
					break;
				}
				case MessageType::SET_OPTION:
				{
					if (input_message.getOption().name == "rows")
					{
						if (input_message.getOption().value == "15")
						{
							board = matrix<Sign>(15, 15);
							rules = GameRules::STANDARD;
							nn_evaluator.loadGraph(argument_parser.getLaunchPath() + "/networks/standard_10x128.bin");
						}
						if (input_message.getOption().value == "20")
						{
							board = matrix<Sign>(20, 20);
							rules = GameRules::FREESTYLE;
							nn_evaluator.loadGraph(argument_parser.getLaunchPath() + "/networks/freestyle_10x128.bin");
						}
					}
					break;
				}
				case MessageType::SET_POSITION:
				{
					board.clear();
					for (size_t i = 0; i < input_message.getListOfMoves().size(); i++)
						Board::putMove(board, input_message.getListOfMoves()[i]);
					sign_to_move = input_message.getListOfMoves().empty() ? Sign::CROSS : invertSign(input_message.getListOfMoves().back().sign);
					break;
				}
				case MessageType::START_SEARCH:
				{
					Move best_move;
					if (mode == "policy")
					{
						std::vector<SearchTask> tasks(8, SearchTask(rules));
						for (int i = 0; i < 8; i++)
						{
							tasks.at(i).set(board, sign_to_move);
							nn_evaluator.addToQueue(tasks.at(i), i);
						}
						nn_evaluator.evaluateGraph();
						matrix<float> policy(board.rows(), board.cols());
						for (int i = 0; i < 8; i++)
							addMatrices(policy, tasks.at(i).getPolicy());
						normalize(policy);
						best_move = pickMove(policy);
					}
					if (mode == "value")
					{
						std::vector<SearchTask> tasks(8 * board.size(), SearchTask(rules));
						int counter = 0;
						for (int r = 0; r < board.rows(); r++)
							for (int c = 0; c < board.cols(); c++)
								if (board.at(r, c) == Sign::NONE)
								{
									board.at(r, c) = sign_to_move;
									for (int i = 0; i < 8; i++, counter++)
									{
										tasks.at(counter).set(board, invertSign(sign_to_move));
										nn_evaluator.addToQueue(tasks.at(counter), i);
									}
									board.at(r, c) = Sign::NONE;
								}
						nn_evaluator.evaluateGraph();
						matrix<Value> values(board.rows(), board.cols());
						counter = 0;
						for (int r = 0; r < board.rows(); r++)
							for (int c = 0; c < board.cols(); c++)
								if (board.at(r, c) == Sign::NONE)
								{
									Value q;
									for (int i = 0; i < 8; i++, counter++)
										q += tasks.at(counter).getValue().getInverted();
									values.at(r, c) = q * 0.125f;
								}

						auto idx = std::distance(values.begin(), std::max_element(values.begin(), values.end(), [](const Value &lhs, const Value &rhs)
						{	return lhs.getExpectation() < rhs.getExpectation();}));
						best_move = Move(idx / values.rows(), idx % values.rows());
					}
					best_move.sign = sign_to_move;
					output_queue.push(Message(MessageType::BEST_MOVE, best_move));
					break;
				}
				case MessageType::EXIT_PROGRAM:
					return;
				default:
					break;
			}
		}
		protocol.processOutput(output_sender);
	}
}

int main(int argc, char *argv[])
{
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

	GameConfig game_config(GameRules::STANDARD, 15);
	// @formatter:off
	matrix<Sign> board = Board::fromString(	" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
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
											" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");  	// @formatter:on
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
//	ag::experimental::VCTSolver solver(game_config, 20);
//	SearchTask task(game_config.rules);
//	task.set(board, Sign::CROSS);
//	solver.solve(task, 2);
//	solver.print_stats();

//	std::cout << "----------------------------------------------------------------\n";
//	task.set(board, Sign::CROSS);
//	ag::VCTSolver solver_old(game_config, 100000);
//	solver_old.solve(task, 2);
//	std::cout << task.toString() << '\n';
//	solver_old.print_stats();

//	network_only_player("value", argc, argv);
//	ag::experimental::PatternCalculator calc(GameConfig(GameRules::STANDARD, 15, 15));
//	ag::experimental::PatternTable table_f(GameRules::FREESTYLE);
//	ag::experimental::PatternTable table_s(GameRules::STANDARD);
//	ag::experimental::PatternTable table_r(GameRules::RENJU);
//	ag::experimental::PatternTable table_c(GameRules::CARO);
//	test_proven_positions(100);
//	test_search();
//	test_feature_extractor();
//	time_manager();
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

