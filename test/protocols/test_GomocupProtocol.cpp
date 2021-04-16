/*
 * test_GomocupProtocol.cpp
 *
 *  Created on: 16 kwi 2021
 *      Author: maciek
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestGomocupProtocol, info)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("INFO timeout_turn 123");
		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "time_for_turn");
		EXPECT_EQ(msg.getOption().value, "123");

		listener.pushLine("INFO timeout_match 1234");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "time_for_match");
		EXPECT_EQ(msg.getOption().value, "1234");

		listener.pushLine("INFO time_left 12");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "time_left");
		EXPECT_EQ(msg.getOption().value, "12");

		listener.pushLine("INFO max_memory 12345");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "max_memory");
		EXPECT_EQ(msg.getOption().value, "12345");

		listener.pushLine("INFO game_type ???"); // not supported - passed value will be ignored
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::EMPTY_MESSAGE);
		EXPECT_TRUE(msg.holdsNoData());

		listener.pushLine("INFO rule 0");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "FREESTYLE");

		listener.pushLine("INFO rule 1");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "STANDARD");

		listener.pushLine("INFO rule 2"); // not supported - passed value will be ignored
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::EMPTY_MESSAGE);
		EXPECT_TRUE(msg.holdsNoData());

		listener.pushLine("INFO rule 4");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "RENJU");

		listener.pushLine("INFO rule 8");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "CARO");

		listener.pushLine("INFO evaluate ???"); // not supported - passed value will be ignored
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::EMPTY_MESSAGE);
		EXPECT_TRUE(msg.holdsNoData());

		listener.pushLine("INFO folder some_path");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "folder");
		EXPECT_EQ(msg.getOption().value, "some_path");
	}
	TEST(TestGomocupProtocol, start)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("START 15");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::START_PROGRAM);
		EXPECT_EQ(msg.getGameConfig().rows, 15);
		EXPECT_EQ(msg.getGameConfig().cols, 15);
	}
	TEST(TestGomocupProtocol, rectstart)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("RECTSTART 15,20");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::START_PROGRAM);
		EXPECT_EQ(msg.getGameConfig().rows, 20);
		EXPECT_EQ(msg.getGameConfig().cols, 15);
	}
	TEST(TestGomocupProtocol, restart)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("RESTART");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::START_PROGRAM);
		EXPECT_TRUE(msg.holdsNoData());
	}
	TEST(TestGomocupProtocol, swap2board_0_stones)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::OPENING_SWAP2);
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(0, 0, Sign::CROSS), Move(2, 3, Sign::CIRCLE), Move(1, 4, Sign::CROSS) };
		msg = Message(MessageType::OPENING_SWAP2, list_of_moves);
		bool flag = protocol.processOutput(msg, sender);
		EXPECT_TRUE(flag);
		EXPECT_EQ(oss.str(), "0,0 3,2 4,1\n");
	}
	TEST(TestGomocupProtocol, swap2board_3_stones)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::OPENING_SWAP2);
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(2, 3, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(1, 4, Sign::CROSS));

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(6, 5, Sign::CIRCLE), Move(5, 2, Sign::CROSS) };
		msg = Message(MessageType::OPENING_SWAP2, list_of_moves);
		bool flag = protocol.processOutput(msg, sender);
		EXPECT_TRUE(flag);
		EXPECT_EQ(oss.str(), "5,6 2,5\n");

		msg = Message(MessageType::OPENING_SWAP2, "SWAP");
		flag = protocol.processOutput(msg, sender);
		EXPECT_TRUE(flag);
		EXPECT_EQ(oss.str(), "5,6 2,5\nSWAP\n");
	}
	TEST(TestGomocupProtocol, swap2board_5_stones)
	{
		InputListener listener;
		GomocupProtocol protocol;

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("5,6");
		listener.pushLine("2,5");
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::OPENING_SWAP2);
		EXPECT_EQ(msg.getListOfMoves().size(), 5u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(2, 3, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(1, 4, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(3), Move(6, 5, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(4), Move(5, 2, Sign::CROSS));

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(7, 7, Sign::CIRCLE) };
		msg = Message(MessageType::OPENING_SWAP2, list_of_moves);
		bool flag = protocol.processOutput(msg, sender);
		EXPECT_TRUE(flag);
		EXPECT_EQ(oss.str(), "7,7\n");

		msg = Message(MessageType::OPENING_SWAP2, "SWAP");
		flag = protocol.processOutput(msg, sender);
		EXPECT_TRUE(flag);
		EXPECT_EQ(oss.str(), "7,7\nSWAP\n");
	}
	TEST(TestGomocupProtocol, begin)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("BEGIN");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);
	}
	TEST(TestGomocupProtocol, board_0_stones)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("BOARD");
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);
	}
	TEST(TestGomocupProtocol, board_odd_stones)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(6, 5, Sign::CROSS));
	}
	TEST(TestGomocupProtocol, board_even_stones)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("0,1,1"); // own stone
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 4u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(3, 2, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(1, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(3), Move(6, 5, Sign::CIRCLE));
	}
	TEST(TestGomocupProtocol, board_incorrect)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("BOARD");
		listener.pushLine("2,3,1"); // own stone
		listener.pushLine("5,6,1"); // own stone
		listener.pushLine("0,0,2"); // opponent stone
		listener.pushLine("DONE");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::ERROR);
		EXPECT_TRUE(msg.holdsString());
	}
	TEST(TestGomocupProtocol, turn)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("TURN 2,3");

		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 1u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));

		std::ostringstream oss;
		OutputSender sender(oss);
		msg = Message(MessageType::MAKE_MOVE, Move(7, 7, Sign::CIRCLE));
		bool flag = protocol.processOutput(msg, sender);
		EXPECT_TRUE(flag);
		EXPECT_EQ(oss.str(), "7,7\n");

		listener.pushLine("TURN 8,7");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(7, 7, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(7, 8, Sign::CROSS));
	}
	TEST(TestGomocupProtocol, takeback)
	{
		InputListener listener;
		GomocupProtocol protocol;
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("DONE");
		Message msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(6, 5, Sign::CROSS));

		listener.pushLine("TAKEBACK 5,6");
		msg = protocol.processInput(listener);
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 2u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));
	}

} /* namespace ag*/

