/*
 * Swap5Controller.cpp
 *
 *  Created on: Jul 1, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/Swap5Controller.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace ag
{
	Swap5Controller::Swap5Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void Swap5Controller::setup(const std::string &args)
	{
		if (args == "play")
			must_play = true;
		else
			must_play = false;
	}
	void Swap5Controller::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::CHECK_BOARD)
		{
			const int stones_on_board = Board::numberOfMoves(search_engine.getBoard());
			switch (stones_on_board)
			{
				case 0:
				{
					const int row = randInt(engine_settings.getGameConfig().rows);
					const int col = randInt(engine_settings.getGameConfig().cols);
					Logger::write("Placing first stone");
					outputQueue.push(Message(MessageType::BEST_MOVE, Move(row, col, Sign::CROSS)));
					state = ControllerState::IDLE;
					break;
				}
				case 1:
				case 2:
				case 3:
				case 4:
					Logger::write("Evaluating first " + std::to_string(stones_on_board) + " stones");
					state = ControllerState::BALANCE;
					start_best_move_search();
					break;
				case 5:
					Logger::write("Looking for the best 6th move");
					state = ControllerState::PLAY;
					start_best_move_search();
			}
		}

		if (state == ControllerState::BALANCE)
		{
			if (is_search_completed(0.1 * time_manager.getTimeForOpening(engine_settings)))
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				if (must_play)
					outputQueue.push(Message(MessageType::BEST_MOVE, get_balanced_move(summary)));
				else
				{
					if (summary.node.getValue().getExpectation() < 0.5f)
						outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
					else
						outputQueue.push(Message(MessageType::BEST_MOVE, get_balanced_move(summary)));
				}
				state = ControllerState::IDLE;
			}
		}

		if (state == ControllerState::PLAY)
		{
			if (is_search_completed(0.2 * time_manager.getTimeForOpening(engine_settings)))
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				outputQueue.push(Message(MessageType::BEST_MOVE, get_best_move(summary)));
				state = ControllerState::IDLE;
			}
		}
	}

} /* namespace ag */

