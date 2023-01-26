/*
 * Search.cpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <numeric>

namespace ag
{
	SearchStats::SearchStats() :
			select("select  "),
			solve("solve   "),
			schedule("schedule"),
			generate("generate"),
			expand("expand  "),
			backup("backup  ")
	{
	}
	std::string SearchStats::toString() const
	{
		std::string result = "----SearchStats----\n";
		result += "nb_duplicate_nodes     = " + std::to_string(nb_duplicate_nodes) + '\n';
		result += "nb_information_leaks   = " + std::to_string(nb_information_leaks) + '\n';
		result += "nb_wasted_expansions   = " + std::to_string(nb_wasted_expansions) + '\n';
		result += "nb_network_evaluations = " + std::to_string(nb_network_evaluations) + '\n';
		result += "nb_node_count = " + std::to_string(nb_node_count) + '\n';
		result += select.toString() + '\n';
		result += solve.toString() + '\n';
		result += schedule.toString() + '\n';
		result += generate.toString() + '\n';
		result += expand.toString() + '\n';
		result += backup.toString() + '\n';
		return result;
	}
	SearchStats& SearchStats::operator+=(const SearchStats &other) noexcept
	{
		this->select += other.select;
		this->solve += other.solve;
		this->schedule += other.schedule;
		this->generate += other.generate;
		this->expand += other.expand;
		this->backup += other.backup;

		this->nb_duplicate_nodes += other.nb_duplicate_nodes;
		this->nb_information_leaks += other.nb_information_leaks;
		this->nb_wasted_expansions += other.nb_wasted_expansions;
		this->nb_network_evaluations += other.nb_network_evaluations;
		this->nb_node_count += other.nb_node_count;
		return *this;
	}
	SearchStats& SearchStats::operator/=(int i) noexcept
	{
		this->select /= i;
		this->solve /= i;
		this->schedule /= i;
		this->generate /= i;
		this->expand /= i;
		this->backup /= i;

		this->nb_duplicate_nodes /= i;
		this->nb_information_leaks /= i;
		this->nb_wasted_expansions /= i;
		this->nb_network_evaluations /= i;
		this->nb_node_count /= i;
		return *this;
	}
	double SearchStats::getTotalTime() const noexcept
	{
		return select.getTotalTime() + solve.getTotalTime() + schedule.getTotalTime() + generate.getTotalTime() + expand.getTotalTime()
				+ backup.getTotalTime();
	}

	Search::Search(GameConfig gameOptions, SearchConfig searchOptions) :
		solver(gameOptions, searchOptions.solver_max_positions),
			game_config(gameOptions),
			search_config(searchOptions)
	{
	}
	int64_t Search::getMemory() const noexcept
	{
		return solver.getMemory();
	}
	const SearchConfig& Search::getConfig() const noexcept
	{
		return search_config;
	}
	ThreatSpaceSearch& Search::getSolver() noexcept
	{
		return solver;
	}
	void Search::clearStats() noexcept
	{
		stats = SearchStats();
		solver.clearStats();
		last_tuning_point.time = getTime();
		last_tuning_point.node_count = stats.nb_node_count;
	}
	SearchStats Search::getStats() const noexcept
	{
//		ts_search.print_stats();
		return stats;
	}

	void Search::select(Tree &tree, int maxSimulations)
	{
		assert(maxSimulations > 0);
		solver.setSharedTable(tree.getSharedHashTable());

		const int batch_size = get_batch_size(tree.getSimulationCount());

		active_task_count = 0;
		while (active_task_count < batch_size and tree.getSimulationCount() < maxSimulations and not tree.hasAllMovesProven())
		{
			TimerGuard timer(stats.select);
			SearchTask &current_task = get_next_task();

			const SelectOutcome out = tree.select(current_task);

			if (current_task.visitedPathLength() == 0)
				break; // visited only root node
			if (is_duplicate(current_task))
			{
				tree.cancelVirtualLoss(current_task);
				active_task_count--;
				stats.nb_duplicate_nodes++;
				break; // in theory the search can be continued but some neural network evaluations would be wasted
			}
			if (out == SelectOutcome::INFORMATION_LEAK)
			{
				stats.nb_information_leaks++;
				tree.correctInformationLeak(current_task);
				tree.cancelVirtualLoss(current_task);
				active_task_count--;
			}
		}
	}
	void Search::solve(bool full)
	{
		const TssMode level = static_cast<TssMode>(search_config.solver_level);
		const int positions = full ? search_config.solver_max_positions : 100;

		for (int i = 0; i < active_task_count; i++)
			if (not search_tasks[i].isReadySolver())
			{
				TimerGuard timer(stats.solve);
				solver.solve(search_tasks[i], level, positions);
			}
	}
	void Search::scheduleToNN(NNEvaluator &evaluator)
	{
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.schedule);
			if (not search_tasks[i].isReadySolver())
			{ // schedule only those tasks that haven't already been solved by the solver
				stats.nb_network_evaluations++;
				evaluator.addToQueue(search_tasks.at(i));
			}
		}
	}
	void Search::generateEdges(const Tree &tree)
	{
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.generate);
			tree.generateEdges(search_tasks[i]);
		}
	}
	void Search::expand(Tree &tree)
	{
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.expand);
			const ExpandOutcome out = tree.expand(search_tasks[i]);
			stats.nb_wasted_expansions += static_cast<uint64_t>(out == ExpandOutcome::ALREADY_EXPANDED);
		}
	}
	void Search::backup(Tree &tree)
	{
		stats.nb_node_count += active_task_count;
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.backup);
			tree.backup(search_tasks[i]);
		}
		active_task_count = 0; // those task should not be used again
	}
	void Search::cleanup(Tree &tree)
	{
		for (int i = 0; i < active_task_count; i++)
			tree.cancelVirtualLoss(search_tasks[i]);
	}
	void Search::tune()
	{
		const double elapsed_time = getTime() - last_tuning_point.time;
		const int64_t evaluated_nodes = stats.nb_node_count - last_tuning_point.node_count;
		if (elapsed_time >= 0.5 or evaluated_nodes >= 1000)
		{
//			ts_search.print_stats();
			const double speed = evaluated_nodes / elapsed_time;
//			std::cout << speed << '\n';
//			if (stats.nb_node_count > 1)
//				vcf_solver.tune(speed);
//				ts_search.tune(speed);
			last_tuning_point.time = getTime();
			last_tuning_point.node_count = stats.nb_node_count;
		}
	}
	/*
	 * private
	 */
	int Search::get_batch_size(int simulation_count) const noexcept
	{
		return search_config.max_batch_size;
//		const int tmp = std::pow(2.0, std::log10(simulation_count)); // doubling batch size for every 10x increase of simulations count
//		return std::max(1, std::min(search_config.max_batch_size, tmp));
	}
	SearchTask& Search::get_next_task()
	{
		active_task_count++;
		if (active_task_count > static_cast<int>(search_tasks.size()))
			search_tasks.push_back(SearchTask(game_config.rules));
		return search_tasks[active_task_count - 1];
	}
	bool Search::is_duplicate(const SearchTask &task) const noexcept
	{
		for (int i = 0; i < active_task_count - 1; i++)
			if (task.getLastPair().edge == search_tasks[i].getLastPair().edge)
				return true;
		return false;
	}

} /* namespace ag */

