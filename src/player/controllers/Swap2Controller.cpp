/*
 * Swap2Controller.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/Swap2Controller.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

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
					state = ControllerState::PUT_FIRST_3_STONES;
					break;
				case 3:
					state = ControllerState::EVALUATE_FIRST_3_STONES;
					start_search(false);
					break;
				case 5:
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
				if (summary.node.getValue().getExpectation() < 0.333f)
				{
					outputQueue.push(Message(MessageType::BEST_MOVE, "swap"));
					state = ControllerState::IDLE;
				}
				else
				{
					if (summary.node.getValue().getExpectation() > 0.666f)
					{
						BestEdgeSelector selector;
						const Move best_move = selector.select(&summary.node)->getMove();
						outputQueue.push(Message(MessageType::BEST_MOVE, best_move));
						state = ControllerState::IDLE;
					}
					else
					{
						first_balancing_move = get_balancing_move();

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
		SearchConfig cfg = engine_settings.getSearchConfig();
		PuctSelector puct_selector(cfg.exploration_constant, engine_settings.getStyleFactor());
		if (balancing)
		{
			const int depth = Board::numberOfMoves(search_engine.getBoard());
			search_engine.setEdgeSelector(BalancedSelector(depth + 1, puct_selector));
		}
		else
			search_engine.setEdgeSelector(puct_selector);
		search_engine.setEdgeGenerator(SolverGenerator(cfg.expansion_prior_treshold, cfg.max_children));
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
		return selector.select(&summary.node)->getMove();
	}

} /* namespace ag */

