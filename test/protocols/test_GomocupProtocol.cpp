/*
 * test_GomocupProtocol.cpp
 *
 *  Created on: Apr 16, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/GomocupProtocol.hpp>

#include <gtest/gtest.h>

namespace
{
	class TestGomocupProtocol: public ::testing::Test
	{
		protected:
			ag::InputListener listener;
			ag::MessageQueue input_queue;
			ag::MessageQueue output_queue;
			ag::GomocupProtocol protocol;

			TestGomocupProtocol() :
					protocol(input_queue, output_queue)
			{
			}
	};
}

namespace ag
{

	TEST_F(TestGomocupProtocol, INFO_timeout_turn)
	{
		listener.pushLine("INFO timeout_turn 123");
		protocol.processInput(listener);

		const Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "time_for_turn");
		EXPECT_EQ(msg.getOption().value, "123");
	}
	TEST_F(TestGomocupProtocol, INFO_timeout_match)
	{
		listener.pushLine("INFO timeout_match 1234");
		protocol.processInput(listener);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "time_for_match");
		EXPECT_EQ(msg.getOption().value, "1234");
	}
	TEST_F(TestGomocupProtocol, INFO_time_left)
	{
		listener.pushLine("INFO time_left 12");
		protocol.processInput(listener);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "time_left");
		EXPECT_EQ(msg.getOption().value, "12");
	}
	TEST_F(TestGomocupProtocol, INFO_max_memory)
	{
		listener.pushLine("INFO max_memory 12345");
		protocol.processInput(listener);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "max_memory");
		EXPECT_EQ(msg.getOption().value, "12345");
	}
	TEST_F(TestGomocupProtocol, INFO_game_type)
	{
		listener.pushLine("INFO game_type ???");
		protocol.processInput(listener);
		EXPECT_TRUE(input_queue.pop().isEmpty());
	}
	TEST_F(TestGomocupProtocol, INFO_rule)
	{
		listener.pushLine("INFO rule 0");
		listener.pushLine("INFO rule 1");
		listener.pushLine("INFO rule 2");
		listener.pushLine("INFO rule 4");
		listener.pushLine("INFO rule 8");
		for (int i = 0; i < 5; i++)
			protocol.processInput(listener);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "FREESTYLE");

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "STANDARD");

		msg = input_queue.pop();
		EXPECT_TRUE(msg.isEmpty());

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "RENJU");

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "CARO");
	}
	TEST_F(TestGomocupProtocol, INFO_evaluate)
	{
		listener.pushLine("INFO evaluate 0,0");
		protocol.processInput(listener);
		EXPECT_TRUE(input_queue.pop().isEmpty());
	}
	TEST_F(TestGomocupProtocol, INFO_folder)
	{
		listener.pushLine("INFO folder some_path");
		protocol.processInput(listener);
		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "folder");
		EXPECT_EQ(msg.getOption().value, "some_path");
	}
	TEST_F(TestGomocupProtocol, START)
	{
		listener.pushLine("START 15");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 1);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_PROGRAM);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getOption().name, "rows");
		EXPECT_EQ(msg.getOption().value, "15");

		msg = input_queue.pop();
		EXPECT_EQ(msg.getOption().name, "columns");
		EXPECT_EQ(msg.getOption().value, "15");

		msg = output_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::PLAIN_STRING);
		EXPECT_EQ(msg.getString(), "OK");
	}
	TEST_F(TestGomocupProtocol, RECTSTART)
	{
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
//		EXPECT_EQ(msg3.getOption().name, "columns");
//		EXPECT_EQ(msg3.getOption().value, "15");

		EXPECT_EQ(output_queue.pop().getType(), MessageType::ERROR);
	}
	TEST_F(TestGomocupProtocol, RESTART)
	{
		listener.pushLine("RESTART");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 1);
		EXPECT_EQ(output_queue.length(), 1);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_PROGRAM);

		msg = output_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::PLAIN_STRING);
		EXPECT_EQ(msg.getString(), "OK");
	}
	TEST_F(TestGomocupProtocol, SWAP2BOARD_0_stones)
	{
		listener.pushLine("SWAP2BOARD");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "swap2");

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(0, 0, Sign::CROSS), Move(2, 3, Sign::CIRCLE), Move(1, 4, Sign::CROSS) };
		output_queue.push(Message(MessageType::BEST_MOVE, list_of_moves));
		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "0,0 3,2 4,1\n");
	}
	TEST_F(TestGomocupProtocol, SWAP2BOARD_3_stones)
	{
		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(2, 3, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(1, 4, Sign::CROSS));

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "swap2");

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(6, 5, Sign::CIRCLE), Move(5, 2, Sign::CROSS) };
		output_queue.push(Message(MessageType::BEST_MOVE, list_of_moves));
		output_queue.push(Message(MessageType::BEST_MOVE, "swap"));

		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "5,6 2,5\nSWAP\n");
	}
	TEST_F(TestGomocupProtocol, SWAP2BOARD_5_stones)
	{
		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("5,6");
		listener.pushLine("2,5");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_EQ(msg.getListOfMoves().size(), 5u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(2, 3, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(1, 4, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(3), Move(6, 5, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(4), Move(5, 2, Sign::CROSS));

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "swap2");

		std::ostringstream oss;
		OutputSender sender(oss);
		std::vector<Move> list_of_moves = { Move(7, 7, Sign::CIRCLE) };
		output_queue.push(Message(MessageType::BEST_MOVE, list_of_moves));
		output_queue.push(Message(MessageType::BEST_MOVE, "swap"));

		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "7,7\nSWAP\n");
	}
	TEST_F(TestGomocupProtocol, begin)
	{
		listener.pushLine("BEGIN");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, board_0_stones)
	{
		listener.pushLine("BOARD");
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, board_odd_stones)
	{
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(6, 5, Sign::CROSS));

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, board_even_stones)
	{
		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("0,1,1"); // own stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 4u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(0, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(3, 2, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(1, 0, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(3), Move(6, 5, Sign::CIRCLE));

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, board_incorrect)
	{
		listener.pushLine("BOARD");
		listener.pushLine("2,3,1"); // own stone
		listener.pushLine("5,6,1"); // own stone
		listener.pushLine("0,0,2"); // opponent stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 0);
		EXPECT_EQ(output_queue.length(), 1);

		Message msg = output_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::ERROR);
		EXPECT_TRUE(msg.holdsString());
	}
	TEST_F(TestGomocupProtocol, turn)
	{
		listener.pushLine("TURN 2,3");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 1u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");

		std::ostringstream oss;
		OutputSender sender(oss);
		output_queue.push(Message(MessageType::BEST_MOVE, Move(7, 7, Sign::CIRCLE)));
		protocol.processOutput(sender);
		EXPECT_EQ(oss.str(), "7,7\n");

		listener.pushLine("TURN 8,7");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 0);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 3u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(7, 7, Sign::CIRCLE));
		EXPECT_EQ(msg.getListOfMoves().at(2), Move(7, 8, Sign::CROSS));

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, takeback)
	{
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

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 2u);
		EXPECT_EQ(msg.getListOfMoves().at(0), Move(3, 2, Sign::CROSS));
		EXPECT_EQ(msg.getListOfMoves().at(1), Move(0, 0, Sign::CIRCLE));

		msg = output_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::PLAIN_STRING);
		EXPECT_EQ(msg.getString(), "OK");
	}

} /* namespace ag*/

