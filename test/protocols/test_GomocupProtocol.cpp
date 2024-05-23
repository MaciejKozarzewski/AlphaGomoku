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
			void setBoardSize(int size)
			{
				listener.pushLine("START " + std::to_string(size));
				protocol.processInput(listener);
				ag::OutputSender sender;
				protocol.processOutput(sender);
				input_queue.clear();
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
		EXPECT_TRUE(input_queue.isEmpty());
	}
	TEST_F(TestGomocupProtocol, INFO_rule)
	{
		listener.pushLine("INFO rule 0");
		listener.pushLine("INFO rule 1");
		listener.pushLine("INFO rule 2");
		listener.pushLine("INFO rule 4");
		for (int i = 0; i < 4; i++)
			protocol.processInput(listener);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "FREESTYLE");

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "STANDARD");

		msg = output_queue.pop(); // error for unsupported rule = 2 (continuous game)
		EXPECT_EQ(msg.getType(), MessageType::ERROR);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_OPTION);
		EXPECT_EQ(msg.getOption().name, "rules");
		EXPECT_EQ(msg.getOption().value, "RENJU");
	}
	TEST_F(TestGomocupProtocol, INFO_evaluate)
	{
		listener.pushLine("INFO evaluate 0,0");
		protocol.processInput(listener);

		Message msg = output_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::INFO_MESSAGE);
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
		EXPECT_EQ(input_queue.length(), 4);
		EXPECT_EQ(output_queue.length(), 1);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_PROGRAM);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getOption().name, "rows");
		EXPECT_EQ(msg.getOption().value, "15");

		msg = input_queue.pop();
		EXPECT_EQ(msg.getOption().name, "columns");
		EXPECT_EQ(msg.getOption().value, "15");

		msg = input_queue.pop();
		EXPECT_EQ(msg.getOption().name, "draw_after");
		EXPECT_EQ(msg.getOption().value, "225");

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

	TEST_F(TestGomocupProtocol, begin)
	{
		setBoardSize(20);

		listener.pushLine("BEGIN");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, board_0_stones)
	{
		setBoardSize(20);

		listener.pushLine("BOARD");
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::SET_POSITION);
		EXPECT_TRUE(msg.holdsListOfMoves());
		EXPECT_EQ(msg.getListOfMoves().size(), 0u);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::START_SEARCH);
		EXPECT_EQ(msg.getString(), "bestmove");
	}
	TEST_F(TestGomocupProtocol, board_odd_stones)
	{
		setBoardSize(20);

		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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
		setBoardSize(20);

		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("0,1,1"); // own stone
		listener.pushLine("DONE");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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
		setBoardSize(20);

		listener.pushLine("BOARD");
		listener.pushLine("2,3,1"); // own stone
		listener.pushLine("5,6,1"); // own stone
		listener.pushLine("0,0,2"); // opponent stone
		listener.pushLine("DONE");

		EXPECT_THROW(protocol.processInput(listener), ProtocolRuntimeException);
	}
	TEST_F(TestGomocupProtocol, turn)
	{
		setBoardSize(20);

		listener.pushLine("TURN 2,3");

		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 0);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 0);

		msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

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
		setBoardSize(20);

		listener.pushLine("BOARD");
		listener.pushLine("2,3,2"); // opponent stone
		listener.pushLine("0,0,1"); // own stone
		listener.pushLine("5,6,2"); // opponent stone
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);
		EXPECT_EQ(output_queue.length(), 0);

		input_queue.clear();

		listener.pushLine("TAKEBACK 5,6");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 2);
		EXPECT_EQ(output_queue.length(), 1);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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

