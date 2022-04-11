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

namespace
{
	using namespace ag;
	Move get_balancing_move(const SearchSummary &summary)
	{
		BalancedSelector selector(std::numeric_limits<int>::max(), PuctSelector(0.0f));
		return selector.select(&summary.node)->getMove();
	}
	void log_balancing_move(const std::string &whichOne, Move m)
	{
		Logger::write(whichOne + " balancing move : " + m.toString() + " (" + m.text() + ")");
	}
}

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
					start_search(0);
					break;
				case 5:
					Logger::write("Evaluating first 5 stones");
					state = ControllerState::EVALUATE_5_STONES;
					start_search(0);
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
						BestEdgeSelector selector;
						const Move best_move = selector.select(&summary.node)->getMove();
						outputQueue.push(Message(MessageType::BEST_MOVE, best_move));
						state = ControllerState::IDLE;
					}
					else
					{
						Logger::write("Balancing 3 stone position");
						if (summary.node.getVisits() < 1000)
						{ // variant with little time
							first_balancing_move = get_balancing_move(summary);
							log_balancing_move("First", first_balancing_move);

							// update board and set up the search with new board
							matrix<Sign> board = search_engine.getBoard();
							Board::putMove(board, first_balancing_move);
							search_engine.setPosition(board, invertSign(first_balancing_move.sign));

							// start search to find second balancing move
							start_search(1);
						}
						else
						{
							first_balancing_move.sign = Sign::ILLEGAL;
							start_search(2);
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
			if (time_manager.getElapsedTime() > 0.5 * time_manager.getTimeForOpening(engine_settings) or search_engine.isSearchFinished())
			{
				stop_search();
				const SearchSummary summary_at_root = search_engine.getSummary( { }, false);
				Logger::write(summary_at_root.node.toString());

				if (first_balancing_move.sign == Sign::ILLEGAL)
				{ // choosing both moves at once
					first_balancing_move = get_balancing_move(summary_at_root);
					log_balancing_move("First", first_balancing_move);
					const SearchSummary summary2 = search_engine.getSummary( { first_balancing_move }, false);
					second_balancing_move = get_balancing_move(summary2);
					log_balancing_move("Second", second_balancing_move);
				}
				else
				{
					second_balancing_move = get_balancing_move(summary_at_root);
					log_balancing_move("Second", second_balancing_move);
				}

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
	void Swap2Controller::start_search(int balancingDepth)
	{
		time_manager.startTimer();
		const SearchConfig cfg = engine_settings.getSearchConfig();
		PuctSelector selector(cfg.exploration_constant, engine_settings.getStyleFactor());
		SolverGenerator generator(cfg.expansion_prior_treshold, cfg.max_children);
		if (balancingDepth > 0)
		{
			const int depth = Board::numberOfMoves(search_engine.getBoard());
			search_engine.setEdgeSelector(BalancedSelector(depth + balancingDepth, selector));
			search_engine.setEdgeGenerator(BalancedGenerator(depth + balancingDepth, generator));
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

} /* namespace ag */

