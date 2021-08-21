/*
 * test_Line.cpp
 *
 *  Created on: Jul 27, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/rules/Line.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <stddef.h>
#include <array>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>

namespace ag
{
	TEST(TestLine, fromString)
	{
//		Line line("?_XO_||_XX_", GameRules::FREESTYLE);
//		EXPECT_EQ(line.toString(), "|_XO_||_XX_");
	}

	/*TEST(TestLine, freestyle_five)
	{
		GameRules rules = GameRules::FREESTYLE;
		Line line;
		line = Line("???????????", rules);
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("||XXXXX__", rules); // five at the board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__XXXXX__", rules); // five in the board center
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXX___", rules); // five with even more space around
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXXO", rules); // five blocked at both ends by opponent
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXX|", rules); // five blocked at both ends, one by opponent, another by board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXXX__", rules); // overline
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__OXXXXXXO_", rules); // overline blocked at both ends by the opponent
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__XXXXXXX__", rules); // even longer overline
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("____XXXX___", rules); // open four
		EXPECT_EQ(line.get_cross_five(), 0);
	}
	TEST(TestLine, standard_five)
	{
		GameRules rules = GameRules::STANDARD;
		Line line;
		line = Line("???????????", rules);
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("||XXXXX__", rules); // five at the board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__XXXXX__", rules); // five in the board center
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXX___", rules); // five with even more space around
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXXO", rules); // five blocked at both ends by opponent
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXX|", rules); // five blocked at both ends, one by opponent, another by board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXXX__", rules); // overline
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("__OXXXXXXO_", rules); // overline blocked at both ends by the opponent
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("__XXXXXXX__", rules); // even longer overline
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("____XXXX___", rules); // open four
		EXPECT_EQ(line.get_cross_five(), 0);
	}
	TEST(TestLine, renju_five)
	{
		GameRules rules = GameRules::RENJU;
		Line line;
		line = Line("???????????", rules);
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("||XXXXX__", rules); // five at the board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__XXXXX__", rules); // five in the board center
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXX___", rules); // five with even more space around
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXXO", rules); // five blocked at both ends by opponent
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXX|", rules); // five blocked at both ends, one by opponent, another by board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXXX__", rules); // overline
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("__OXXXXXXO_", rules); // overline blocked at both ends by the opponent
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("__XXXXXXX__", rules); // even longer overline
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("____XXXX___", rules); // open four
		EXPECT_EQ(line.get_cross_five(), 0);
	}
	TEST(TestLine, caro_five)
	{
		GameRules rules = GameRules::CARO;
		Line line;
		line = Line("???????????", rules);
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("||XXXXX__", rules); // five at the board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__XXXXX__", rules); // five in the board center
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXX___", rules); // five with even more space around
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("OXXXXXO", rules); // five blocked at both ends by opponent
		EXPECT_EQ(line.get_cross_five(), 0);

		line = Line("OXXXXX|", rules); // five blocked at both ends, one by opponent, another by board edge
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("___XXXXXX__", rules); // overline
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__OXXXXXXO_", rules); // overline blocked at both ends by the opponent
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("__XXXXXXX__", rules); // even longer overline
		EXPECT_EQ(line.get_cross_five(), 1);

		line = Line("____XXXX___", rules); // open four
		EXPECT_EQ(line.get_cross_five(), 0);
	}*/

}
/* namespace ag */

