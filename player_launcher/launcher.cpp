/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/version.hpp>
#include <alphagomoku/player/EngineManager.hpp>

#include <iostream>
#include <alphagomoku/utils/statistics.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/game/Board.hpp>

using namespace ag;

int main(int argc, char *argv[])
{
	GameConfig game_config(GameRules::FREESTYLE, 10);

	Tree tree(game_config, TreeConfig());

//	Board board(game_config);
//	board.putMove(Move(1, 5, Sign::CROSS));
//	board.putMove(Move(5, 3, Sign::CIRCLE));
//	board.putMove(Move(2, 0, Sign::CROSS));
//	tree.setBoard(board);

	SearchTask task;
	tree.select(task, PuctSelector(1.25f));

	task.setValue(Value(0.0, 0.0, 1.0));
//	task.setProvenValue(ProvenValue::LOSS);
//	task.addMove( { Value(), 0.1f, Move(0, 0, Sign::CIRCLE), ProvenValue::UNKNOWN });
//	task.addMove( { Value(), 0.1f, Move(5, 4, Sign::CIRCLE), ProvenValue::UNKNOWN });
//	task.addMove( { Value(), 0.1f, Move(5, 6, Sign::CIRCLE), ProvenValue::UNKNOWN });
//	task.addMove( { Value(), 0.7f, Move(5, 5, Sign::CIRCLE), ProvenValue::UNKNOWN });
	task.setReady();
	std::cout << task.toString();
	tree.expand(task);
	tree.backup(task);

	tree.printSubtree();
	std::cout << '\n';
	tree.select(task, PuctSelector(1.25f));

	task.setValue(Value(1.0, 0.0, 0.0));
	task.setProvenValue(ProvenValue::WIN);
//	task.addMove( { Value(), 0.2f, Move(0, 0, Sign::CROSS), ProvenValue::LOSS });
//	task.addMove( { Value(), 0.8f, Move(5, 5, Sign::CROSS), ProvenValue::LOSS });
	task.setReady();
	std::cout << task.toString();
//	tree.expand(task);
	tree.backup(task);

	tree.printSubtree();
	std::cout << '\n';

	std::cout << "END\n";

//	ag::EngineManager engine_manager;
//	try
//	{
//		bool can_continue = engine_manager.processArguments(argc, argv);
//		if (can_continue)
//		{
//			engine_manager.run();
//			ag::Logger::write(ag::ProgramInfo::name() + " successfully exits");
//		}
//		return 0;
//	} catch (std::exception &e)
//	{
//		std::cerr << e.what() << std::endl;
//		ag::Logger::write(e.what());
//	}
//	return -1;
}

