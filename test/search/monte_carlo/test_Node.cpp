/*
 * test_Node.cpp
 *
 *  Created on: Mar 9, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Node.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestNode, default_construct)
	{
		const Node node;

		EXPECT_FALSE(node.isOwning());
		EXPECT_FALSE(node.isRoot());
		EXPECT_FALSE(node.mustDefend());
		EXPECT_FALSE(node.isFullyExpanded());
		EXPECT_TRUE(node.isLeaf());

		EXPECT_EQ(node.getWinRate(), 0.0f);
		EXPECT_EQ(node.getDrawRate(), 0.0f);
		EXPECT_EQ(node.getLossRate(), 1.0f);
		EXPECT_EQ(node.getVisits(), 0);
		EXPECT_EQ(node.getScore(), Score());

		EXPECT_FALSE(node.isProven());
		EXPECT_EQ(node.numberOfEdges(), 0);
		EXPECT_EQ(node.getDepth(), 0);
		EXPECT_EQ(node.getVirtualLoss(), 0);
		EXPECT_EQ(node.getSignToMove(), Sign::NONE);
	}

} /* namespace ag */

