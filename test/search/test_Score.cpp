/*
 * test_Score.cpp
 *
 *  Created on: Jun 2, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/Score.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestScore, compare)
	{
		EXPECT_GT(Score::win_in(4), Score::win_in(5));  // shorter win is better
		EXPECT_GT(Score::win_in(4), Score::draw_in(5)); // proven win is better than proven draw
		EXPECT_GT(Score::win_in(4), Score::max_eval());	// proven win is better than even the best unproven score
		EXPECT_GT(Score::win_in(4), Score::loss_in(5)); // proven win is better than proven loss

		EXPECT_GT(Score(123), Score(-123));				// higher score is better
		EXPECT_GT(Score::min_eval(), Score::draw_in(5));// even worst unproven score is better than a proven draw
		EXPECT_GT(Score::min_eval(), Score::loss_in(5));// even worst unproven score is better than a proven loss

		EXPECT_GT(Score::draw_in(4), Score::draw_in(3)); // for consistency longer draw is preferred but in reality it doesn't matter
		EXPECT_GT(Score::draw_in(4), Score::loss_in(5)); // proven draw is better than proven loss

		EXPECT_GT(Score::loss_in(4), Score::loss_in(3)); // longer loss is better
	}
	TEST(TestScore, infinities)
	{
		const Score s1 = Score::minus_infinity();
		const Score s2 = Score::plus_infinity();
		const Score s3 = Score();

		EXPECT_TRUE(s1.isInfinite());
		EXPECT_FALSE(s1.isFinite());

		EXPECT_TRUE(s2.isInfinite());
		EXPECT_FALSE(s2.isFinite());

		EXPECT_FALSE(s3.isInfinite());
		EXPECT_TRUE(s3.isFinite());
	}
	TEST(TestScore, plus)
	{
		const Score s1 = Score::minus_infinity();
		const Score s2 = Score::plus_infinity();
		const Score s3 = Score(123);

		EXPECT_EQ((s1 + 1), s1);
		EXPECT_EQ((s2 + 1), s2);
		EXPECT_EQ((s3 + 1), Score(124));
	}
	TEST(TestScore, inversion)
	{
		const Score s1 = Score::win_in(5);
		const Score s2 = Score::loss_in(5);
		const Score s3 = Score::draw_in(5);
		const Score s4 = Score(123);
		const Score s5 = Score(-123);

		const Score sminf = Score::minus_infinity();
		const Score spinf = Score::plus_infinity();

		EXPECT_EQ(-s1, Score::loss_in(5));
		EXPECT_EQ(-s2, Score::win_in(5));
		EXPECT_EQ(-s3, Score::draw_in(5));
		EXPECT_EQ(-s4, Score(-123));
		EXPECT_EQ(-s5, Score(123));
		EXPECT_EQ(-sminf, Score::plus_infinity());
		EXPECT_EQ(-spinf, Score::minus_infinity());
	}
	TEST(TestScore, invert_up)
	{
		const Score s1 = Score::win_in(5);
		const Score s2 = Score::loss_in(5);
		const Score s3 = Score::draw_in(5);
		const Score s4 = Score(123);
		const Score s5 = Score(-123);

		const Score sminf = Score::minus_infinity();
		const Score spinf = Score::plus_infinity();

		EXPECT_EQ(invert_up(s1), Score::loss_in(6));
		EXPECT_EQ(invert_up(s2), Score::win_in(6));
		EXPECT_EQ(invert_up(s3), Score::draw_in(6));
		EXPECT_EQ(invert_up(s4), Score(-123));
		EXPECT_EQ(invert_up(s5), Score(123));
		EXPECT_EQ(invert_up(sminf), Score::plus_infinity());
		EXPECT_EQ(invert_up(spinf), Score::minus_infinity());
	}
	TEST(TestScore, invert_down)
	{
		const Score s1 = Score::win_in(5);
		const Score s2 = Score::loss_in(5);
		const Score s3 = Score::draw_in(5);
		const Score s4 = Score(123);
		const Score s5 = Score(-123);

		const Score sminf = Score::minus_infinity();
		const Score spinf = Score::plus_infinity();

		EXPECT_EQ(invert_down(s1), Score::loss_in(4));
		EXPECT_EQ(invert_down(s2), Score::win_in(4));
		EXPECT_EQ(invert_down(s3), Score::draw_in(4));
		EXPECT_EQ(invert_down(s4), Score(-123));
		EXPECT_EQ(invert_down(s5), Score(123));
		EXPECT_EQ(invert_down(sminf), Score::plus_infinity());
		EXPECT_EQ(invert_down(spinf), Score::minus_infinity());
	}

} /* namespace ag */
