/*
 * SwapController.cpp
 *
 *  Created on: Apr 5, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/SwapController.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

namespace ag
{
	SwapController::SwapController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void SwapController::control(MessageQueue &outputQueue)
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
					start_search();
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
			else
				return;
		}
	}
	/*
	 * private
	 */
	void SwapController::start_search()
	{
		time_manager.startTimer();
		const SearchConfig cfg = engine_settings.getSearchConfig();
		search_engine.setEdgeSelector(PuctSelector(cfg.exploration_constant, engine_settings.getStyleFactor()));
		search_engine.setEdgeGenerator(SolverGenerator(cfg.expansion_prior_treshold, cfg.max_children));
		search_engine.startSearch();
	}
	void SwapController::stop_search()
	{
		search_engine.stopSearch();
		search_engine.logSearchInfo();
		time_manager.stopTimer();
		time_manager.resetTimer();
	}

} /* namespace ag */
