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
					start_search(false);
					break;
				case 5:
					Logger::write("Evaluating first 5 stones");
					state = ControllerState::EVALUATE_5_STONES;
					start_search(false);
					break;
			}
		}

		if (state == ControllerState::PUT_FIRST_3_STONES)
		{
			const int r = randInt(engine_settings.getSwap2Openings().size());
			Message msg(MessageType::BEST_MOVE, engine_settings.getSwap2Openings()[r]);
			outputQueue.push(msg);
			state = ControllerState::IDLE;
		}

		if (state == ControllerState::EVALUATE_FIRST_3_STONES)
		{
			if (time_manager.getElapsedTime() > 0.5 * time_manager.getTimeForOpening(engine_settings) or search_engine.isSearchFinished())
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				const float expectation_value = summary.node.getValue().getExpectation();
				Logger::write("Evaluated 3 stone opening : " + std::to_string(100 * expectation_value) + "% probability of winning");
				if (expectation_value < 0.333f)
				{
					outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
					state = ControllerState::IDLE;
				}
				else
				{
					if (expectation_value > 0.666f)
					{
						BestEdgeSelector selector;
						const Move best_move = selector.select(&summary.node)->getMove();
						outputQueue.push(Message(MessageType::BEST_MOVE, best_move));
						state = ControllerState::IDLE;
					}
					else
					{
						Logger::write("Balancing 3 stone position");
						first_balancing_move = get_balancing_move();
						Logger::write("First balancing move : " + first_balancing_move.toString() + " (" + first_balancing_move.text() + ")");

						// update board and set up the search with new board
						matrix<Sign> board = search_engine.getBoard();
						Board::putMove(board, first_balancing_move);
						search_engine.setPosition(board, invertSign(first_balancing_move.sign));

						// start search to find second balancing move
						start_search(true);

						state = ControllerState::BALANCE_THE_OPENING;
					}
				}
			}
			else
				return;
		}

		if (state == ControllerState::BALANCE_THE_OPENING)
		{
			if (time_manager.getElapsedTime() > 0.5 * time_manager.getTimeForOpening(engine_settings) or search_engine.isSearchFinished())
			{
				stop_search();
				second_balancing_move = get_balancing_move();
				Logger::write("Second balancing move : " + second_balancing_move.toString() + " (" + second_balancing_move.text() + ")");

				std::vector<Move> response = { first_balancing_move, second_balancing_move };
				outputQueue.push(Message(MessageType::BEST_MOVE, response));
				state = ControllerState::IDLE;
			}
			else
				return;
		}

		if (state == ControllerState::EVALUATE_5_STONES)
		{
			if (time_manager.getElapsedTime() > time_manager.getTimeForOpening(engine_settings) or search_engine.isSearchFinished())
			{
				stop_search();
				const SearchSummary summary = search_engine.getSummary( { }, false);
				if (summary.node.getValue().getExpectation() < 0.5f)
					outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
				else
				{
					BestEdgeSelector selector;
					const Move best_move = selector.select(&summary.node)->getMove();
					outputQueue.push(Message(MessageType::BEST_MOVE, best_move));
				}
				state = ControllerState::IDLE;
			}
		}
	}
	/*
	 * private
	 */
	void Swap2Controller::start_search(bool balancing)
	{
		time_manager.startTimer();
		const SearchConfig cfg = engine_settings.getSearchConfig();
		PuctSelector selector(cfg.exploration_constant, engine_settings.getStyleFactor());
		SolverGenerator generator(cfg.expansion_prior_treshold, cfg.max_children);
		if (balancing)
		{
			const int depth = Board::numberOfMoves(search_engine.getBoard());
			search_engine.setEdgeSelector(BalancedSelector(depth + 1, selector));
			search_engine.setEdgeGenerator(BalancedGenerator(depth + 1, generator));
		}
		else
		{
			search_engine.setEdgeSelector(selector);
			search_engine.setEdgeGenerator(generator);
		}

		search_engine.startSearch();
	}
	void Swap2Controller::stop_search()
	{
		search_engine.stopSearch();
		search_engine.logSearchInfo();
		time_manager.stopTimer();
		time_manager.resetTimer();
	}
	Move Swap2Controller::get_balancing_move()
	{
		const SearchSummary summary = search_engine.getSummary( { }, false);
		const int depth = Board::numberOfMoves(search_engine.getBoard());
		BalancedSelector selector(depth + 1, PuctSelector(0.0f));
		Logger::write(summary.node.toString());
		return selector.select(&summary.node)->getMove();
	}

} /* namespace ag */

