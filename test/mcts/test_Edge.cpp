/*
 * test_Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Edge.hpp>
#include <gtest/gtest.h>
#include <cmath>

namespace ag
{
	TEST(TestEdge, init)
	{
		Edge edge;
		EXPECT_EQ(edge.getPolicyPrior(), 0.0f);
		EXPECT_EQ(edge.getValue().win, 0.0f);
		EXPECT_EQ(edge.getValue().draw, 0.0f);
		EXPECT_EQ(edge.getVisits(), 0);
		EXPECT_EQ(edge.getProvenValue(), ProvenValue::UNKNOWN);
		EXPECT_EQ(edge.getMove(), 0);
		EXPECT_TRUE(edge.isLeaf());
		EXPECT_FALSE(edge.isProven());
	}
	TEST(TestEdge, updateValue)
	{
		Edge edge;
		float sum = 0.0f;
		for (int i = 0; i < 1000; i++)
		{
			sum += fabsf(sin(i));
			edge.updateValue(Value(fabsf(sin(i))));
		}
		EXPECT_EQ(edge.getVisits(), 1000);
		EXPECT_NEAR(edge.getValue().win, sum / 1000, 1.0e-4f);
	}
	TEST(TestEdge, applyVirtualLoss)
	{
		Edge edge;
		edge.updateValue(1.0f);
		for (int i = 0; i < 10; i++)
		{
			edge.applyVirtualLoss();
			EXPECT_EQ(edge.getVisits(), i + 2);
			EXPECT_NEAR(edge.getValue().win, 1.0f / (i + 2), 1.0e-4f);
		}
	}
	TEST(TestEdge, cancelVirtualLoss)
	{
		Edge edge1, edge2;
		float sum = 0.0f;
		for (int i = 0; i < 1000; i++)
		{
			sum += fabsf(sin(i));
			edge1.updateValue(fabsf(sin(i)));
			edge2.applyVirtualLoss();
			edge2.updateValue(fabsf(sin(i)));
		}
		for (int i = 0; i < 1000; i++)
			edge2.cancelVirtualLoss();
		EXPECT_EQ(edge1.getVisits(), 1000);
		EXPECT_EQ(edge1.getVisits(), edge2.getVisits());
		EXPECT_NEAR(edge1.getValue().win, edge2.getValue().win, 1.0e-4f);
		EXPECT_NEAR(edge1.getValue().draw, edge2.getValue().draw, 1.0e-4f);
		EXPECT_NEAR(edge1.getValue().loss, edge2.getValue().loss, 1.0e-4f);
	}
	TEST(TestEdge, setProvenValue)
	{
		Edge edge;

		edge.setProvenValue(ProvenValue::WIN);
		EXPECT_EQ(edge.getProvenValue(), ProvenValue::WIN);
		EXPECT_TRUE(edge.isProven());
	}
}

