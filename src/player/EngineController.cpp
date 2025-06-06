/*
 * EngineController.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/TimeManager.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/augmentations.hpp>

namespace
{
	using namespace ag;
	std::unique_ptr<EdgeSelector> get_base_selector(const EngineSettings &settings)
	{
		return EdgeSelector::create(settings.getSearchConfig().mcts_config.edge_selector_config);
	}
	UnifiedGenerator get_base_generator(const EngineSettings &settings)
	{
		return UnifiedGenerator(settings.getSearchConfig().mcts_config.max_children,
				settings.getSearchConfig().mcts_config.policy_expansion_threshold);
	}
}

namespace ag
{
	Move get_balanced_move(const SearchSummary &summary)
	{
		BalancedSelector selector;
		return selector.select(&summary.node)->getMove();
	}
	std::vector<Move> get_multiple_balanced_moves(SearchSummary summary, int number)
	{
		assert(summary.node.numberOfEdges() >= number);
		BalancedSelector selector;
		std::vector<Move> result;
		for (int i = 0; i < number; i++)
		{
			const Move selected = selector.select(&summary.node)->getMove();
			result.push_back(selected);
			const int idx = std::distance(summary.node.begin(), std::find_if(summary.node.begin(), summary.node.end(), [selected](const Edge &edge)
			{	return edge.getMove() == selected;}));
			summary.node.removeEdge(idx);
		}
		return result;
	}
	Move get_best_move(const SearchSummary &summary)
	{
		EdgeSelectorConfig config;
		config.exploration_constant = 0.2f;
		LCBSelector selector(config);
		return selector.select(&summary.node)->getMove();
	}
	void log_balancing_move(const std::string &whichOne, Move m)
	{
		Logger::write(whichOne + " balancing move : " + m.toString() + " (" + m.text() + ")");
	}
	std::vector<Move> get_renju_opening(int index)
	{
		// @formatter:off
		const std::vector<std::array<Move, 3>> openings = {
				{ Move("Xh7"), Move("Oi6"), Move("Xj5") }, // Chosei
				{ Move("Xh7"), Move("Oi6"), Move("Xj6") }, // Kyogetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xj7") }, // Kosei
				{ Move("Xh7"), Move("Oi6"), Move("Xj8") }, // Suigetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xj9") }, // Ryusei
				{ Move("Xh7"), Move("Oi6"), Move("Xi7") }, // Ungetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xi8") }, // Hogetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xi9") }, // Rangetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xh8") }, // Gingetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xh9") }, // Myojo
				{ Move("Xh7"), Move("Oi6"), Move("Xg8") }, // Shagetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xg9") }, // Meigetsu
				{ Move("Xh7"), Move("Oi6"), Move("Xf9") }, // Suisei
				{ Move("Xh7"), Move("Oh6"), Move("Xh5") }, // Kansei
				{ Move("Xh7"), Move("Oh6"), Move("Xi5") }, // Keigetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xj5") }, // Sosei
				{ Move("Xh7"), Move("Oh6"), Move("Xi6") }, // Kagetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xj6") }, // Zangetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xi7") }, // Ugetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xj7") }, // Kinsei
				{ Move("Xh7"), Move("Oh6"), Move("Xh8") }, // Shogetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xi8") }, // Kyugetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xj8") }, // Shingetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xh9") }, // Zuigetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xi9") }, // Sangetsu
				{ Move("Xh7"), Move("Oh6"), Move("Xj9") }};// Yusei
// @formatter:on
		std::vector<Move> result(openings.at(index).begin(), openings.at(index).end());
		const int mode = randInt(4);
		for (size_t i = 0; i < result.size(); i++)
			result[i] = augment(result[i], 15, 15, mode);
		return result;
	}

	EngineController::EngineController(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			engine_settings(settings),
			time_manager(manager),
			search_engine(engine)
	{
	}
	void EngineController::setup(const std::string &args)
	{
	}
	/*
	 * protected
	 */
	bool EngineController::is_search_completed(double timeout) const
	{
		if (not search_engine.isRootEvaluated())
			return false;
		if (time_manager.getElapsedTime() > timeout or search_engine.isSearchFinished())
			return true;

		Node root_node = search_engine.getRootCopy();
		if (root_node.numberOfEdges() == 1)
			return true;
		if (root_node.getVisits() <= 1)
			return false;

		int most_visits = 0;
		int total_visits = 0;

		for (Edge *edge = root_node.begin(); edge < root_node.end(); edge++)
		{
			const int v = std::max(1, edge->getVisits());
			total_visits += v;
			most_visits = std::max(most_visits, v);
		}

		return most_visits > 0.99 * total_visits; // early stopping
	}
	void EngineController::start_best_move_search()
	{
		time_manager.startTimer();
		search_engine.setEdgeSelector(*get_base_selector(engine_settings));
		search_engine.setEdgeGenerator(get_base_generator(engine_settings));
		search_engine.startSearch();
	}
	void EngineController::start_balancing_search(int balancingMoves)
	{
		time_manager.startTimer();
		const int move_number = search_engine.getTree().getMoveNumber();
		search_engine.setEdgeSelector(BalancedSelector(move_number + balancingMoves, *get_base_selector(engine_settings)));
		search_engine.setEdgeGenerator(BalancedGenerator(move_number + balancingMoves, get_base_generator(engine_settings)));
		search_engine.startSearch();
	}
	void EngineController::start_center_only_search(int centralSquareSize)
	{
		time_manager.startTimer();
		const int move_number = search_engine.getTree().getMoveNumber();
		search_engine.setEdgeSelector(BalancedSelector(move_number + 1, *get_base_selector(engine_settings)));
		search_engine.setEdgeGenerator(CenterOnlyGenerator(centralSquareSize, get_base_generator(engine_settings)));
		search_engine.startSearch();
	}
	void EngineController::start_center_excluding_search(int centralSquareSize)
	{
		time_manager.startTimer();
		const int move_number = search_engine.getTree().getMoveNumber();
		search_engine.setEdgeSelector(BalancedSelector(move_number + 1, *get_base_selector(engine_settings)));
		search_engine.setEdgeGenerator(CenterExcludingGenerator(centralSquareSize, get_base_generator(engine_settings)));
		search_engine.startSearch();
	}
	void EngineController::start_symmetric_excluding_search()
	{
		time_manager.startTimer();
		const int move_number = search_engine.getTree().getMoveNumber();
		search_engine.setEdgeSelector(BalancedSelector(move_number + 1, *get_base_selector(engine_settings)));
		search_engine.setEdgeGenerator(SymmetricalExcludingGenerator(get_base_generator(engine_settings)));
		search_engine.startSearch();
	}
	void EngineController::stop_search(bool logSearchInfo)
	{
		search_engine.stopSearch();
		if (logSearchInfo)
			search_engine.logSearchInfo();
		time_manager.stopTimer();
		time_manager.resetTimer();
	}

} /* namespace ag */

