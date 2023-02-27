/*
 * test_CompressedFloat.cpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/CompressedFloat.hpp>

#include <gtest/gtest.h>

#include <cmath>

namespace ag
{

	TEST(TestCompressedFloat, save_load)
	{
		const float value = 0.456374f;
		CompressedFloat cf(value);

		const float loaded = cf;

		const float diff = std::fabs(value - loaded);
		EXPECT_LT(diff, 0.00001f);
	}

} /* namespace ag */

