/*
 * test_Node.cpp
 *
 *  Created on: Mar 3, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Node.hpp>
#include <gtest/gtest.h>
#include <cmath>

namespace ag
{
	TEST(TestNode, init)
	{
		Node node;
		EXPECT_EQ(node.getPolicyPrior(), 0.0f);
		EXPECT_EQ(node.getValue().win, 0.0f);
		EXPECT_EQ(node.getValue().draw, 0.0f);
		EXPECT_EQ(node.getValue().loss, 0.0f);
		EXPECT_EQ(node.getVisits(), 0);
		EXPECT_EQ(node.getProvenValue(), ProvenValue::UNKNOWN);
		EXPECT_EQ(node.getMove(), 0);
		EXPECT_EQ(node.numberOfChildren(), 0);
		EXPECT_TRUE(node.isLeaf());
		EXPECT_FALSE(node.isProven());
	}
	TEST(TestNode, createChildren)
	{
		Node node;
		Node array[123];

		node.createChildren(array, 123);

		EXPECT_EQ(node.getProvenValue(), ProvenValue::UNKNOWN);
		EXPECT_EQ(node.numberOfChildren(), 123);
		EXPECT_FALSE(node.isLeaf());
		EXPECT_FALSE(node.isProven());
	}
	TEST(TestNode, updateValue)
	{
		Node node;
		float sum = 0.0f;
		for (int i = 0; i < 1000; i++)
		{
			sum += fabsf(sin(i));
			node.updateValue(Value(fabsf(sin(i))));
		}
		EXPECT_EQ(node.getVisits(), 1000);
		EXPECT_NEAR(node.getValue().win, sum / 1000, 1.0e-4f);
	}
	TEST(TestNode, applyVirtualLoss)
	{
		Node node;
		node.updateValue(1.0f);
		for (int i = 0; i < 10; i++)
		{
			node.applyVirtualLoss();
			EXPECT_EQ(node.getVisits(), i + 2);
			EXPECT_NEAR(node.getValue().win, 1.0f / (i + 2), 1.0e-4f);
		}
	}
	TEST(TestNode, cancelVirtualLoss)
	{
		Node node1, node2;
		float sum = 0.0f;
		for (int i = 0; i < 1000; i++)
		{
			sum += fabsf(sin(i));
			node1.updateValue(fabsf(sin(i)));
			node2.applyVirtualLoss();
			node2.updateValue(fabsf(sin(i)));
		}
		for (int i = 0; i < 1000; i++)
			node2.cancelVirtualLoss();
		EXPECT_EQ(node1.getVisits(), 1000);
		EXPECT_EQ(node1.getVisits(), node2.getVisits());
		EXPECT_NEAR(node1.getValue().win, node2.getValue().win, 1.0e-4f);
		EXPECT_NEAR(node1.getValue().draw, node2.getValue().draw, 1.0e-4f);
		EXPECT_NEAR(node1.getValue().loss, node2.getValue().loss, 1.0e-4f);
	}
	TEST(TestNode, setProvenValue)
	{
		Node node;
		Node array[123];

		node.createChildren(array, 123);
		node.setProvenValue(ProvenValue::WIN);

		EXPECT_EQ(node.getProvenValue(), ProvenValue::WIN);
		EXPECT_EQ(node.numberOfChildren(), 123);
		EXPECT_FALSE(node.isLeaf());
		EXPECT_TRUE(node.isProven());
	}
}

