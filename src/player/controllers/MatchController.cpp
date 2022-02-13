/*
 * MatchController.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/MatchController.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>

namespace ag
{
	MatchController::MatchController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void MatchController::control(MessageQueue &outputQueue)
	{
		if (state == ControllerState::IDLE)
			return;

		if (state == ControllerState::SETUP)
		{
			time_manager.startTimer();
			start_search();
			state = ControllerState::SEARCH;
		}

		if (state == ControllerState::SEARCH)
		{
			SearchSummary summary = search_engine.getSummary( { }, false);
			if (time_manager.getElapsedTime() > time_manager.getTimeForTurn(engine_settings, summary.node.getDepth(), summary.node.getValue())
					or search_engine.isSearchFinished())
			{
				search_engine.stopSearch();
				time_manager.stopTimer();
				time_manager.resetTimer();
				state = ControllerState::GET_BEST_ACTION;
			}
			else
				return;
		}

		if (state == ControllerState::GET_BEST_ACTION)
		{
			search_engine.logSearchInfo();
			SearchSummary summary = search_engine.getSummary( { }, true);
			summary.time_used = time_manager.getLastSearchTime();
			outputQueue.push(Message(MessageType::INFO_MESSAGE, summary));

			BestEdgeSelector selector;
			Move best_move = selector.select(&summary.node)->getMove();
			outputQueue.push(Message(MessageType::BEST_MOVE, best_move));

			if (engine_settings.isUsingAutoPondering() and not engine_settings.isInAnalysisMode())
			{
				matrix<Sign> board = search_engine.getBoard();
				Board::putMove(board, best_move);
				search_engine.setPosition(board, invertSign(best_move.sign));

				start_search();
				state = ControllerState::PONDERING;
			}
			else
				state = ControllerState::IDLE;
		}

		if (state == ControllerState::PONDERING)
		{
			if (search_engine.isSearchFinished())
				state = ControllerState::IDLE;
		}
	}
	/*
	 * private
	 */
	void MatchController::start_search()
	{
		SearchConfig cfg = engine_settings.getSearchConfig();
		search_engine.setEdgeSelector(PuctSelector(cfg.exploration_constant, engine_settings.getStyleFactor()));
		search_engine.setEdgeGenerator(SolverGenerator(cfg.expansion_prior_treshold, cfg.max_children));
		search_engine.startSearch();
	}
} /* namespace ag */

