/*
 * test_Tree.cpp
 *
 *  Created on: Mar 3, 2021
 *      Author: Maciej Kozarzewski
 */

//#include <alphagomoku/mcts/Node.hpp>
//#include <alphagomoku/mcts/Value.hpp>
//#include <alphagomoku/mcts/Tree.hpp>
//#include <alphagomoku/mcts/SearchTrajectory.hpp>
#include <gtest/gtest.h>

namespace ag
{
//	TEST(TestTree, init)
//	{
//		TreeConfig cfg;
//		Tree_old tree(cfg);
//		EXPECT_EQ(tree.allocatedNodes(), 0);
//		EXPECT_EQ(tree.usedNodes(), 0);
//	}
//	TEST(TestTree, simulation)
//	{
//		SearchTrajectory_old trajectory;
//		TreeConfig cfg;
//		Tree_old tree(cfg);
//		tree.getRootNode().setMove(Move(0, 2, Sign::CIRCLE));
//
//		//simulation 1
//		std::vector<std::pair<uint16_t, float>> moves;
//		moves.push_back( { Move::move_to_short(0, 1, Sign::CROSS), 0.3f });
//		moves.push_back( { Move::move_to_short(0, 4, Sign::CROSS), 0.6f });
//		moves.push_back( { Move::move_to_short(0, 6, Sign::CROSS), 0.1f });
//		tree.select(trajectory);
//		tree.expand(trajectory.getLeafNode(), moves);
//		tree.backup(trajectory, Value(0.9f, 0.0f, 0.1f), ProvenValue::UNKNOWN);
//
//		// simulation 2
//		moves.clear();
//		moves.push_back( { Move::move_to_short(2, 3, Sign::CIRCLE), 0.01f });
//		moves.push_back( { Move::move_to_short(3, 3, Sign::CIRCLE), 0.03f });
//		tree.select(trajectory);
//		tree.expand(trajectory.getLeafNode(), moves);
//		tree.backup(trajectory, Value(0.94f, 0.0f, 0.06), ProvenValue::UNKNOWN);
//
//		//simulation 3
//		tree.select(trajectory);
//		tree.backup(trajectory, Value(0.0f, 0.0f, 1.0f), ProvenValue::UNKNOWN);
//
//		//depth 0
//		EXPECT_EQ(tree.getRootNode().getVisits(), 3);
//		EXPECT_FLOAT_EQ(tree.getRootNode().getValue().win, (0.9f + 0.06f + 0.0f) / 3);
//
//		//depth 1
//		EXPECT_EQ(tree.getRootNode().getChild(0).getVisits(), 0);
//		EXPECT_FLOAT_EQ(tree.getRootNode().getChild(0).getValue().win, 0);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getVisits(), 2);
//		EXPECT_FLOAT_EQ(tree.getRootNode().getChild(1).getValue().win, (0.94f + 1.0f) / 2);
//		EXPECT_EQ(tree.getRootNode().getChild(2).getVisits(), 0);
//		EXPECT_FLOAT_EQ(tree.getRootNode().getChild(2).getValue().win, 0);
//
//		//depth 2
//		EXPECT_EQ(tree.getRootNode().getChild(1).getChild(0).getVisits(), 0);
//		EXPECT_FLOAT_EQ(tree.getRootNode().getChild(1).getChild(0).getValue().win, 0);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getChild(1).getVisits(), 1);
//		EXPECT_FLOAT_EQ(tree.getRootNode().getChild(1).getChild(1).getValue().win, 0);
//	}
//	TEST(TestTree , exactSearch)
//	{
//		GTEST_SKIP();// test was created for "init to loss" select strategy
//		SearchTrajectory_old trajectory;
//		TreeConfig cfg;
//		Tree_old tree(cfg);
//		tree.getRootNode().setMove(Move(0, 2, Sign::CIRCLE));
//
//		//simulation 1
//		tree.select(trajectory);
//		std::vector<std::pair<uint16_t, float>> moves;
//		moves.push_back( { Move::move_to_short(0, 1, Sign::CROSS), 0.3f });
//		moves.push_back( { Move::move_to_short(0, 4, Sign::CROSS), 0.6f });
//		moves.push_back( { Move::move_to_short(0, 6, Sign::CROSS), 0.1f });
//
//		tree.expand(trajectory.getLeafNode(), moves);
//		tree.backup(trajectory, Value(0.9f), ProvenValue::UNKNOWN);
//
//		// simulation 2
//		tree.select(trajectory);
//		moves.clear();
//		moves.push_back( { Move::move_to_short(2, 3, Sign::CIRCLE), 0.01f });
//		moves.push_back( { Move::move_to_short(3, 3, Sign::CIRCLE), 0.03f });
//
//		tree.expand(trajectory.getLeafNode(), moves);
//		tree.backup(trajectory, Value(0.94f), ProvenValue::UNKNOWN);
//
//		//simulation 3
//		tree.select(trajectory);
//		tree.backup(trajectory, Value(0.0f), ProvenValue::LOSS);
//		tree.select(trajectory);
//		tree.backup(trajectory, Value(0.5f), ProvenValue::DRAW);
//
//		tree.select(trajectory);
//		tree.backup(trajectory, Value(0.5f), ProvenValue::DRAW);
//		tree.select(trajectory);
//		tree.backup(trajectory, Value(1.0f), ProvenValue::WIN);
//
//		tree.printSubtree(tree.getRootNode());
//		//depth 0
//		EXPECT_EQ(tree.getRootNode().getVisits(), 6);
//		EXPECT_EQ(tree.getRootNode().getProvenValue(), ProvenValue::LOSS);
//
//		//depth 1
//		EXPECT_EQ(tree.getRootNode().getChild(0).getVisits(), 1);
//		EXPECT_EQ(tree.getRootNode().getChild(0).getProvenValue(), ProvenValue::DRAW);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getVisits(), 3);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getProvenValue(), ProvenValue::DRAW);
//		EXPECT_EQ(tree.getRootNode().getChild(2).getVisits(), 1);
//		EXPECT_EQ(tree.getRootNode().getChild(2).getProvenValue(), ProvenValue::WIN);
//
//		//depth 2
//		EXPECT_EQ(tree.getRootNode().getChild(1).getChild(0).getVisits(), 1);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getChild(0).getProvenValue(), ProvenValue::DRAW);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getChild(1).getVisits(), 1);
//		EXPECT_EQ(tree.getRootNode().getChild(1).getChild(1).getProvenValue(), ProvenValue::LOSS);
//	}
}

