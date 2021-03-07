/*
 * test_file_util.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/file_util.hpp>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestFileUtil, save_load)
	{
		float data[100];
		for (int i = 0; i < 100; i++)
			data[i] = i;
		SerializedObject so;
		Json json = { { "key0", 0.0 }, { "key1", "string" } };
		so.save(data, sizeof(data));

		FileSaver fs("test_file.bin");
		fs.save(json, so);
		fs.close();

		FileLoader fl("test_file.bin");
		float loaded_data[100];
		fl.getBinaryData().load(loaded_data, 0, sizeof(loaded_data));
		EXPECT_EQ(fl.getJson().dump(), json.dump());
		for (int i = 0; i < 100; i++)
			EXPECT_EQ(data[i], loaded_data[i]);
	}
}

