/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/version.hpp>
#include <iostream>
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/player/ProgramManager.hpp>

#include <numeric>

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

void test_search()
{
	GameConfig game_config(GameRules::STANDARD, 15);

	TreeConfig tree_config;
//	tree_config.bucket_size = 1000000;
//	tree_config.cache_size = 1024;
	Tree tree(tree_config);

	SearchConfig search_config;
	search_config.max_batch_size = 32;
	search_config.exploration_constant = 1.25f;
	search_config.noise_weight = 0.0f;
	search_config.expansion_prior_treshold = 1.0e-4f;
	search_config.max_children = 30;
	search_config.use_endgame_solver = true;
	search_config.vcf_solver_level = 2;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.omp_threads = 1;
	device_config.device = ml::Device::cuda(0);
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/test_10x10_standard/checkpoint/network_65_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/standard_2021/network_5x64wdl_opt.bin");
	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku511/networks/standard_10x128.bin");

	Sign sign_to_move;
	matrix<Sign> board;

	// @formatter:off
	board = Board::fromString(
			/*        a b c d e f g h i j k l m n o         */
			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  0 */
			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  1 */
			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  2 */
			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  3 */
			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  4 */
			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  5 */
			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/*  6 */
			/*  7 */" _ _ _ _ O _ _ O _ _ X _ _ _ _\n"/*  7 */
			/*  8 */" _ _ _ _ _ _ _ _ X X O _ _ _ _\n"/*  8 */
			/*  9 */" _ _ _ _ _ _ _ X O O O X _ _ _\n"/*  9 */
			/* 10 */" _ _ _ _ _ _ _ _ X _ _ _ _ _ _\n"/* 10 */
			/* 11 */" _ _ _ _ _ _ _ _ _ _ O X _ _ _\n"/* 11 */
			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 12 */
			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 13 */
			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"/* 14 */
			/*        a b c d e f g h i j k l m n o         */); // @formatter:on

	board = Board::fromString(" _ _ _ X O O O O X _ _ _ _ _ _\n"
			" X _ O O _ X O _ _ _ _ _ _ _ _\n"
			" X X O X X X X O _ _ _ _ _ _ _\n"
			" X X O X X X O _ X _ _ _ _ _ _\n"
			" O O O X O X O _ _ O _ _ O _ _\n"
			" _ X O O X O X X _ X _ X _ _ _\n"
			" _ _ O O X O X O _ O O _ _ _ _\n"
			" _ _ X O O O X _ _ O _ _ _ _ _\n"
			" _ _ O O X X _ X O O O _ _ _ _\n"
			" _ O X X X X O X X X _ _ _ _ _\n"
			" _ _ X O O X X O X O _ _ _ _ _\n"
			" O X X X X O O O X _ _ _ _ _ _\n"
			" O _ _ O _ _ _ X X _ _ _ _ _ _\n"
			" _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n"
			" _ _ X _ _ _ _ _ _ _ _ _ _ _ _\n");

//	// @formatter:off
//	board = Board::fromString(
//			/*        a b c d e f g h i j k l m n o          */
//			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//			/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//			/*  8 */" _ _ _ _ _ X X X _ _ _ _ _ _ _\n" /*  8 */
//			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//			/*        a b c d e f g h i j k l m n o          */); // @formatter:on
	sign_to_move = Sign::CIRCLE;

	std::cout << get_BOARD_command(board, sign_to_move);
	return;

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

	FeatureExtractor extractor(game_config);
	extractor.setBoard(board, sign_to_move);
	extractor.printAllThreats();

	Search search(game_config, search_config);

	tree.setBoard(board, sign_to_move);
	tree.setEdgeSelector(PuctSelector(1.25f));
	tree.setEdgeGenerator(SolverGenerator(search_config.expansion_prior_treshold, search_config.max_children));

//	matrix<float> policy(board.rows(), board.cols());
	int next_step = 0;
	for (int i = 0; i <= 1000000; i++)
	{
		if (tree.getSimulationCount() >= next_step)
		{
			std::cout << tree.getSimulationCount() << " ...\n";
			next_step += 10000;
		}
//		while (tree.getSimulationCount() < i * 100)
		{
			search.select(tree, 1000000);
			search.tryToSolve();

			search.scheduleToNN(nn_evaluator);
			nn_evaluator.evaluateGraph();

			search.generateEdges(tree);
			search.expand(tree);
			search.backup(tree);

//			std::cout << "\n\n";
//			tree.printSubtree(-1, false);

			if (tree.isProven())
				break;
		}
	}

	tree.printSubtree(0, true, 10);
	search.printSolverStats();
	std::cout << search.getStats().toString() << '\n';
	std::cout << tree.getTreeStats().toString();
	std::cout << "memory = " << (tree.getMemory() >> 20) << "MB\n\n";
	std::cout << tree.getNodeCacheStats().toString() << '\n';
	std::cout << nn_evaluator.getStats().toString() << '\n';

	Node info = tree.getInfo( { });
	info.sortEdges();
	std::cout << info.toString() << '\n';
	for (int i = 0; i < info.numberOfEdges(); i++)
		std::cout << info.getEdge(i).toString() << '\n';
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
	SearchTask task(game_config.rules);
	tree.select(task);
	tree.cancelVirtualLoss(task);

	for (int i = 0; i < task.visitedPathLength(); i++)
	{
		if (i < 10 and task.visitedPathLength() >= 10)
			std::cout << " ";
		if (i < 100 and task.visitedPathLength() >= 100)
			std::cout << " ";
		std::cout << i << " : " << task.getPair(i).edge->toString() << '\n';
	}

	double start = getTime();
	Board::putMove(board, Move(Sign::CIRCLE, 10, 9));
	Board::putMove(board, Move(Sign::CROSS, 8, 7));
	tree.setBoard(board, Sign::CIRCLE);
	double stop = getTime();
	std::cout << (stop - start) << '\n';
	std::cout << tree.getTreeStats().toString() << '\n';
	std::cout << tree.getNodeCacheStats().toString() << '\n';
}

int main(int argc, char *argv[])
{
//	test_search();
//	std::cout << "END" << std::endl;
//	return 0;

//	GameConfig game_config(GameRules::FREESTYLE, 10);
//	TreeConfig tree_config;
//
//	Tree tree(tree_config);
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	Board::putMove(board, Move(1, 5, Sign::CROSS));
//	Board::putMove(board, Move(5, 3, Sign::CIRCLE));
//	Board::putMove(board, Move(2, 0, Sign::CROSS));
//	Board::putMove(board, Move(3, 0, Sign::CROSS));
//	Board::putMove(board, Move(4, 0, Sign::CROSS));
//	Board::putMove(board, Move(5, 0, Sign::CROSS));
//	tree.setBoard(board, Sign::CROSS);
//	tree.setEdgeSelector(PuctSelector(1.25f));
//	tree.setEdgeGenerator(BaseGenerator());

//	SearchTask task(game_config.rules);
//	task.reset(board, Sign::CROSS);
//	task.setValue( { 0.33f, 0.57f });
//	add_edge(task, Move(Sign::CROSS, 4, 4), 0.5f);
//	add_edge(task, Move(Sign::CROSS, 4, 7), 0.3f);
//	add_edge(task, Move(Sign::CROSS, 2, 5), 0.2f);
//	task.setReady();
//	tree.expand(task);
//	tree.backup(task);

//	tree.select(task);
//	add_edge(task, Move(Sign::CIRCLE, 3, 3), 0.99f);
//	add_edge(task, Move(Sign::CIRCLE, 3, 4), 0.01f);
//	task.setValue( { 0.6f, 0.3f });
//	task.setReady();
//	tree.expand(task);
//	tree.backup(task);

//	std::cout << Board::toString(board) << '\n';
//	tree.printSubtree();

//	Board::putMove(board, Move(Sign::CROSS, 4, 4));
//	board.fill(Sign::CIRCLE);
//	tree.setBoard(board, Sign::CIRCLE);
//	std::cout << "new tree\n";
//	tree.printSubtree();

//	SearchTask task;
//	tree.select(task, selector);
//
//	task.setValue(Value(0.2, 0.1, 0.7));
//	task.getPolicy().at(0, 0) = 0.1f;
//	task.getPolicy().at(5, 4) = 0.1f;
//	task.getPolicy().at(5, 6) = 0.1f;
//	task.getPolicy().at(5, 5) = 0.7f;
//	task.addProvenEdge(Move(1, 1, Sign::CROSS), ProvenValue::DRAW);
//	task.addProvenEdge(Move(4, 1, Sign::CROSS), ProvenValue::LOSS);
//	task.addProvenEdge(Move(8, 1, Sign::CROSS), ProvenValue::LOSS);
//	task.setReady();
//	std::cout << task.toString();
//
//	generator.generate(task);
//	tree.expand(task);
//	tree.backup(task);
//	tree.printSubtree();
//	std::cout << '\n';

//	tree.select(task, PuctSelector(1.25f));
//	task.setValue(Value(0.9, 0.0, 0.1));
//	task.addEdge(Move(1, 1, Sign::CROSS), ProvenValue::LOSS);
//	task.addEdge(Move(4, 1, Sign::CROSS), ProvenValue::LOSS);
//	task.addEdge(Move(8, 1, Sign::CROSS), ProvenValue::LOSS);
//	task.setReady();
//	std::cout << task.toString();
//	tree.expand(task);
//	tree.backup(task);
//
//	tree.printSubtree();
//	std::cout << '\n';

//	std::cout << "END" << std::endl;

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

