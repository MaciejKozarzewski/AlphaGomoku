/*
 * test_ExtendedGomocupProtocol.cpp
 *
 *  Created on: Apr 18, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/protocols/ExtendedGomocupProtocol.hpp>

#include <gtest/gtest.h>

namespace
{
	class TestExtendedGomocupProtocol: public ::testing::Test
	{
		protected:
			ag::InputListener listener;
			ag::MessageQueue input_queue;
			ag::MessageQueue output_queue;
			ag::ExtendedGomocupProtocol protocol;

			TestExtendedGomocupProtocol() :
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

	TEST_F(TestExtendedGomocupProtocol, SWAP2BOARD_0_stones)
	{
		setBoardSize(20);

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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
	TEST_F(TestExtendedGomocupProtocol, SWAP2BOARD_3_stones)
	{
		setBoardSize(20);

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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
	TEST_F(TestExtendedGomocupProtocol, SWAP2BOARD_5_stones)
	{
		setBoardSize(20);

		listener.pushLine("SWAP2BOARD");
		listener.pushLine("0,0");
		listener.pushLine("3,2");
		listener.pushLine("4,1");
		listener.pushLine("5,6");
		listener.pushLine("2,5");
		listener.pushLine("DONE");
		protocol.processInput(listener);
		EXPECT_EQ(input_queue.length(), 3);

		Message msg = input_queue.pop();
		EXPECT_EQ(msg.getType(), MessageType::STOP_SEARCH);

		msg = input_queue.pop();
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

} /* namespace ag */

