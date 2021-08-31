/*
 * test_GomocupProtocol.cpp
 *
 *  Created on: Apr 16, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>

#include <gtest/gtest.h>

namespace ag
{

	TEST(TestGomocupProtocol, info)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("INFO timeout_turn 123");
		listener.pushLine("INFO timeout_match 1234");
		listener.pushLine("INFO time_left 12");
		listener.pushLine("INFO max_memory 12345");
		listener.pushLine("INFO game_type ???"); // not supported - passed value will be ignored
		listener.pushLine("INFO rule 0");
		listener.pushLine("INFO rule 1");
		listener.pushLine("INFO rule 2"); // not supported - passed value will be ignored
		listener.pushLine("INFO rule 4");
		listener.pushLine("INFO rule 8");
		listener.pushLine("INFO evaluate ???"); // not supported - passed value will be ignored
		listener.pushLine("INFO folder some_path");
		for (int i = 0; i < 12; i++)
			protocol.processInput(listener);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "time_for_turn");
		EXPECT_EQ(input_queue.pop().getOption().value, "123");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "time_for_match");
		EXPECT_EQ(input_queue.pop().getOption().value, "1234");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "time_left");
		EXPECT_EQ(input_queue.pop().getOption().value, "12");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "max_memory");
		EXPECT_EQ(input_queue.pop().getOption().value, "12345");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::EMPTY_MESSAGE);
		EXPECT_TRUE(input_queue.pop().holdsNoData());

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "rules");
		EXPECT_EQ(input_queue.pop().getOption().value, "FREESTYLE");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "rules");
		EXPECT_EQ(input_queue.pop().getOption().value, "STANDARD");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::EMPTY_MESSAGE);
		EXPECT_TRUE(input_queue.pop().holdsNoData());

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "rules");
		EXPECT_EQ(input_queue.pop().getOption().value, "RENJU");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "rules");
		EXPECT_EQ(input_queue.pop().getOption().value, "CARO");

		EXPECT_EQ(input_queue.peek().getType(), MessageType::EMPTY_MESSAGE);
		EXPECT_TRUE(input_queue.pop().holdsNoData());

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_OPTION);
		EXPECT_EQ(input_queue.peek().getOption().name, "folder");
		EXPECT_EQ(input_queue.pop().getOption().value, "some_path");
	}
	TEST(TestGomocupProtocol, start)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("START 15");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 1);

		EXPECT_EQ(input_queue.pop().getType(), MessageType::START_PROGRAM);

		EXPECT_EQ(input_queue.peek().getOption().name, "rows");
		EXPECT_EQ(input_queue.pop().getOption().value, "15");

		EXPECT_EQ(input_queue.peek().getOption().name, "cols");
		EXPECT_EQ(input_queue.pop().getOption().value, "15");

		EXPECT_EQ(output_queue.peek().getType(), MessageType::PLAIN_STRING);
		EXPECT_EQ(output_queue.pop().getString(), "OK");
	}
	TEST(TestGomocupProtocol, rectstart)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("RECTSTART 15,20");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 0);
		EXPECT_EQ(output_queue.length(), 1);

//		Message msg1 = input_queue.pop();
//		EXPECT_EQ(msg1.getType(), MessageType::START_PROGRAM);
//		Message msg2 = input_queue.pop();
//		EXPECT_EQ(msg2.getOption().name, "rows");
//		EXPECT_EQ(msg2.getOption().value, "20");
//		Message msg3 = input_queue.pop();
//		EXPECT_EQ(msg3.getOption().name, "cols");
//		EXPECT_EQ(msg3.getOption().value, "15");

		EXPECT_EQ(output_queue.pop().getType(), MessageType::ERROR);
	}
	TEST(TestGomocupProtocol, restart)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("RESTART");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 1);
		EXPECT_EQ(output_queue.length(), 1);

		EXPECT_EQ(input_queue.pop().getType(), MessageType::START_PROGRAM);

		EXPECT_EQ(output_queue.peek().getType(), MessageType::PLAIN_STRING);
		EXPECT_EQ(output_queue.pop().getString(), "OK");
	}
	TEST(TestGomocupProtocol, swap2board_0_stones)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_EQ(input_queue.pop().getListOfMoves().size(), 0u);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "swap2");

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(0, 0, Sign::CROSS), Move(2, 3, Sign::CIRCLE), Move(1, 4, Sign::CROSS) };
		output_queue.push(Message(MessageType::BEST_MOVE, list_of_moves));
		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "0,0 3,2 4,1\n");
	}
	TEST(TestGomocupProtocol, swap2board_3_stones)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 3u);
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(1), Move(2, 3, Sign::CIRCLE));
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(2), Move(1, 4, Sign::CROSS));

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "swap2");

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(6, 5, Sign::CIRCLE), Move(5, 2, Sign::CROSS) };
		output_queue.push(Message(MessageType::BEST_MOVE, list_of_moves));
		output_queue.push(Message(MessageType::BEST_MOVE, "swap"));

		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "5,6 2,5\nSWAP\n");
	}
	TEST(TestGomocupProtocol, swap2board_5_stones)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("5,6");
		listener.pushLine("2,5");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 5u);
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(1), Move(2, 3, Sign::CIRCLE));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(2), Move(1, 4, Sign::CROSS));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(3), Move(6, 5, Sign::CIRCLE));
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(4), Move(5, 2, Sign::CROSS));

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "swap2");

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(7, 7, Sign::CIRCLE) };
		output_queue.push(Message(MessageType::BEST_MOVE, list_of_moves));
		output_queue.push(Message(MessageType::BEST_MOVE, "swap"));

		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "7,7\nSWAP\n");
	}
	TEST(TestGomocupProtocol, begin)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);

		listener.pushLine("BEGIN");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.pop().getListOfMoves().size(), 0u);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "bestmove");
	}
	TEST(TestGomocupProtocol, board_0_stones)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);
		listener.pushLine("BOARD");
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.pop().getListOfMoves().size(), 0u);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "bestmove");
	}
	TEST(TestGomocupProtocol, board_odd_stones)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 3u);
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(2), Move(6, 5, Sign::CROSS));

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "bestmove");
	}
	TEST(TestGomocupProtocol, board_even_stones)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("0,1,1"); // own stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 4u);
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(1), Move(3, 2, Sign::CIRCLE));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(2), Move(1, 0, Sign::CROSS));
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(3), Move(6, 5, Sign::CIRCLE));

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "bestmove");
	}
	TEST(TestGomocupProtocol, board_incorrect)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);
		listener.pushLine("BOARD");
		listener.pushLine("2,3,1"); // own stone
		listener.pushLine("5,6,1"); // own stone
		listener.pushLine("0,0,2"); // opponent stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 0);
		EXPECT_EQ(output_queue.length(), 1);

		EXPECT_EQ(output_queue.peek().getType(), MessageType::ERROR);
		EXPECT_TRUE(output_queue.pop().holdsString());
	}
	TEST(TestGomocupProtocol, turn)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);
		listener.pushLine("TURN 2,3");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 1u);
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(0), Move(3, 2, Sign::CROSS));

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "bestmove");

		std::ostringstream oss;
		OutputSender sender(oss);
		output_queue.push(Message(MessageType::BEST_MOVE, Move(7, 7, Sign::CIRCLE)));
		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "7,7\n");

		listener.pushLine("TURN 8,7");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 3u);
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(1), Move(7, 7, Sign::CIRCLE));
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(2), Move(7, 8, Sign::CROSS));

		EXPECT_EQ(input_queue.peek().getType(), MessageType::START_SEARCH);
		EXPECT_EQ(input_queue.pop().getString(), "bestmove");
	}
	TEST(TestGomocupProtocol, takeback)
	{
		InputListener listener;
		MessageQueue input_queue;
		MessageQueue output_queue;
		GomocupProtocol protocol(input_queue, output_queue);
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		input_queue.clear();

		listener.pushLine("TAKEBACK 5,6");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 1);
		EXPECT_EQ(output_queue.length(), 1);

		EXPECT_EQ(input_queue.peek().getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(input_queue.peek().holdsListOfMoves());
		EXPECT_EQ(input_queue.peek().getListOfMoves().size(), 2u);
		EXPECT_EQ(input_queue.peek().getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(input_queue.pop().getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));

		EXPECT_EQ(output_queue.peek().getType(), MessageType::PLAIN_STRING);
		EXPECT_EQ(output_queue.pop().getString(), "OK");
	}

} /* namespace ag*/

