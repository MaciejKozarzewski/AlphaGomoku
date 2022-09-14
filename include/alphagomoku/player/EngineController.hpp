/*
 * EngineController.hpp
 *
 *  Created on: Feb 5, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_
#define ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_

#include <string>
#include <vector>

namespace ag
{
	class EngineSettings;
	class TimeManager;
	class SearchEngine;
	class MessageQueue;
	class EdgeSelector;
	class EdgeGenerator;
	struct Move;
	struct SearchSummary;
}

namespace ag
{
	Move get_balanced_move(const SearchSummary &summary);
	std::vector<Move> get_multiple_balanced_moves(SearchSummary summary, int number);
	Move get_best_move(const SearchSummary &summary);
	void log_balancing_move(const std::string &whichOne, Move m);
	std::vector<Move> get_renju_opening(int index);

	class EngineController
	{
		protected:
			const EngineSettings &engine_settings;
			TimeManager &time_manager;
			SearchEngine &search_engine;

		public:
			EngineController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine);
			EngineController(const EngineController &other) = delete;
			EngineController(EngineController &&other) = delete;
			EngineController& operator=(const EngineController &other) = delete;
			EngineController& operator=(EngineController &&other) = delete;
			virtual ~EngineController() = default;

			virtual void setup(const std::string &args);
			virtual void control(MessageQueue &outputQueue) = 0;

		protected:
			bool is_search_completed(double timeout) const;
			void start_best_move_search();
			void start_balancing_search(int balancingMoves);
			void start_center_only_search(int centralSquareSize);
			void start_center_excluding_search(int centralSquareSize);
			void start_symmetric_excluding_search();
			void stop_search(bool logSearchInfo = true);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PLAYER_ENGINECONTROLLER_HPP_ */
