/*
 * test_SearchTask.cpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/mcts/Edge.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/mcts/Value.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace
{
	using namespace ag;
	class TestSearchTask: public ::testing::Test
	{
		protected:
			SearchTask task;
			Node node1;
			Edge edges1[1];
			Node node2;

			TestSearchTask()
			{
//				task.reset(Board(GameConfig(GameRules::FREESTYLE, 15)));
//				node1.assignEdges(edges1, 1);
//				edges1[0].assignNode(&node2);
//				edges1[0].setMove(Move(0, 0, Sign::CROSS));
//				task.append(&node1, &edges1[0]);
//
//				for (int i = 0; i < 30; i++)
//					node1.updateValue(Value(0.4f, 0.3f, 0.2f));
//				for (int i = 0; i < 14; i++)
//					edges1[0].updateValue(Value(0.2f, 0.3f, 0.4f));
			}
	};
}

namespace ag
{
//	TEST_F(TestSearchTask, no_information_leak)
//	{
//		for (int i = 0; i < 20; i++)
//			node2.updateValue(Value(0.2f, 0.3f, 0.4f));
//
//		task.correctInformationLeak();
//		edges1[0].updateValue(task.getValue());
//		EXPECT_FLOAT_EQ(edges1[0].getWinRate(), node2.getWinRate());
//		EXPECT_FLOAT_EQ(edges1[0].getDrawRate(), node2.getDrawRate());
//	}
//	TEST_F(TestSearchTask, information_leak)
//	{
//		for (int i = 0; i < 20; i++)
//			node2.updateValue(Value(0.7f, 0.3f, 0.0f));
//
//		task.correctInformationLeak();
//		edges1[0].updateValue(task.getValue());
//		EXPECT_FLOAT_EQ(edges1[0].getWinRate(), node2.getWinRate());
//		EXPECT_FLOAT_EQ(edges1[0].getDrawRate(), node2.getDrawRate());
//	}
} /* namespace ag */

