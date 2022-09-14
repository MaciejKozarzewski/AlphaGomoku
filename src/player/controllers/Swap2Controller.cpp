/*
 * Swap2Controller.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/Swap2Controller.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace ag
{
	Swap2Controller::Swap2Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void Swap2Controller::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::CHECK_BOARD)
		{
			switch (Board::numberOfMoves(search_engine.getBoard()))
			{
				case 0:
					Logger::write("Placing first 3 stones");
					state = ControllerState::PUT_FIRST_3_STONES;
					break;
				case 3:
					Logger::write("Evaluating first 3 stones");
					state = ControllerState::EVALUATE_FIRST_3_STONES;
					start_best_move_search();
					break;
				case 5:
					Logger::write("Evaluating first 5 stones");
					state = ControllerState::EVALUATE_5_STONES;
					start_best_move_search();
					break;
			}
		}

		if (state == ControllerState::PUT_FIRST_3_STONES)
		{
			const int r = randInt(engine_settings.getSwap2Openings().size());
			Message msg(MessageType::BEST_MOVE, engine_settings.getSwap2Openings()[r]);
			outputQueue.push(msg);
			state = ControllerState::IDLE;
			return;
		}

		if (state == ControllerState::EVALUATE_FIRST_3_STONES)
		{
			if (is_search_completed(0.5 * time_manager.getTimeForOpening(engine_settings)))
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				Logger::write(summary.node.toString());

				const float expectation_value = summary.node.getValue().getExpectation();
				Logger::write("Evaluated 3 stone opening : " + std::to_string(100 * expectation_value) + "% probability of winning");
				if (expectation_value < 1.0f / 3.0f)
				{
					outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
					state = ControllerState::IDLE;
				}
				else
				{
					if (expectation_value > 2.0f / 3.0f)
					{
						outputQueue.push(Message(MessageType::BEST_MOVE, get_best_move(summary)));
						state = ControllerState::IDLE;
					}
					else
					{
						Logger::write("Balancing 3 stone position");
						if (summary.node.getVisits() < 1000)
						{ // variant with little time
							first_balancing_move = get_balanced_move(summary);
							log_balancing_move("First", first_balancing_move);

							// update board and set up the search with new board
							matrix<Sign> board = search_engine.getBoard();
							Board::putMove(board, first_balancing_move);
							search_engine.setPosition(board, invertSign(first_balancing_move.sign));

							start_balancing_search(1); // start search to find second balancing move
						}
						else
						{
							first_balancing_move.sign = Sign::ILLEGAL;
							start_balancing_search(2);
						}

						state = ControllerState::BALANCE_THE_OPENING;
					}
				}
			}
			else
				return;
		}

		if (state == ControllerState::BALANCE_THE_OPENING)
		{
			if (is_search_completed(0.5 * time_manager.getTimeForOpening(engine_settings)))
			{
				stop_search();
				const SearchSummary summary_at_root = search_engine.getSummary( { }, false);
				Logger::write(summary_at_root.node.toString());

				if (first_balancing_move.sign == Sign::ILLEGAL)
				{ // choosing both moves at once
					first_balancing_move = get_balanced_move(summary_at_root);
					log_balancing_move("First", first_balancing_move);

					const SearchSummary summary2 = search_engine.getSummary( { first_balancing_move }, false);
					second_balancing_move = get_balanced_move(summary2);
				}
				else
					second_balancing_move = get_balanced_move(summary_at_root);
				log_balancing_move("Second", second_balancing_move);

				std::vector<Move> response = { first_balancing_move, second_balancing_move };
				outputQueue.push(Message(MessageType::BEST_MOVE, response));
				state = ControllerState::IDLE;
			}
			else
				return;
		}

		if (state == ControllerState::EVALUATE_5_STONES)
		{
			if (is_search_completed(time_manager.getTimeForOpening(engine_settings)))
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				if (summary.node.getValue().getExpectation() < 0.5f)
					outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
				else
					outputQueue.push(Message(MessageType::BEST_MOVE, get_best_move(summary)));
				state = ControllerState::IDLE;
			}
		}
	}

} /* namespace ag */

