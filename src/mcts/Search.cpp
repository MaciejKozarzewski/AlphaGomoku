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
#include <alphagomoku/mcts/EdgeGenerator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <numeric>

namespace ag
{
	SearchStats::SearchStats() :
			select("select  "),
			evaluate("evaluate"),
			schedule("schedule"),
			generate("generate"),
			expand("expand  "),
			backup("backup  ")
	{
	}
	std::string SearchStats::toString() const
	{
		std::string result = "----SearchStats----\n";
		result += "nb_duplicate_nodes   = " + std::to_string(nb_duplicate_nodes) + '\n';
		result += "nb_information_leaks = " + std::to_string(nb_information_leaks) + '\n';
		result += "nb_wasted_expansions = " + std::to_string(nb_wasted_expansions) + '\n';
		result += select.toString() + '\n';
		result += evaluate.toString() + '\n';
		result += schedule.toString() + '\n';
		result += generate.toString() + '\n';
		result += expand.toString() + '\n';
		result += backup.toString() + '\n';
		return result;
	}
	SearchStats& SearchStats::operator+=(const SearchStats &other) noexcept
	{
		this->select += other.select;
		this->evaluate += other.evaluate;
		this->schedule += other.schedule;
		this->generate += other.generate;
		this->expand += other.expand;
		this->backup += other.backup;

		this->nb_duplicate_nodes += other.nb_duplicate_nodes;
		this->nb_information_leaks += other.nb_information_leaks;
		this->nb_wasted_expansions += other.nb_wasted_expansions;
		return *this;
	}
	SearchStats& SearchStats::operator/=(int i) noexcept
	{
		this->select /= i;
		this->evaluate /= i;
		this->schedule /= i;
		this->generate /= i;
		this->expand /= i;
		this->backup /= i;

		this->nb_duplicate_nodes /= i;
		this->nb_information_leaks /= i;
		this->nb_wasted_expansions /= i;
		return *this;
	}

	Search::Search(GameConfig gameOptions, SearchConfig searchOptions) :
			vcf_solver(gameOptions),
			game_config(gameOptions),
			search_config(searchOptions)
	{
	}

	const SearchConfig& Search::getConfig() const noexcept
	{
		return search_config;
	}

	void Search::clearStats() noexcept
	{
		stats = SearchStats();
	}
	void Search::printSolverStats() const
	{
		vcf_solver.print_stats();
	}
	SearchStats Search::getStats() const noexcept
	{
		return stats;
	}

	void Search::select(Tree &tree, int maxSimulations)
	{
		assert(maxSimulations > 0);
		const int batch_size = get_batch_size(tree.getSimulationCount());

		active_task_count = 0;
		while (active_task_count < batch_size and tree.getSimulationCount() < maxSimulations and not tree.isProven())
		{
			TimerGuard timer(stats.select);
			SearchTask &current_task = get_next_task();

			SelectOutcome out = tree.select(current_task);

			if (current_task.visitedPathLength() == 0)
				break; // visited only root node
			if (is_duplicate(current_task))
			{
				tree.cancelVirtualLoss(current_task);
				active_task_count--;
				stats.nb_duplicate_nodes++;
				break;
			}
			if (out == SelectOutcome::INFORMATION_LEAK)
			{
				stats.nb_information_leaks++;
				tree.backup(current_task);
				active_task_count--;
			}
		}
	}
	void Search::tryToSolve()
	{
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.evaluate);
			vcf_solver.solve(search_tasks[i], search_config.vcf_solver_level);
		}
	}
	void Search::scheduleToNN(NNEvaluator &evaluator)
	{
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.schedule);
			if (search_tasks[i].isReady() == false)
				evaluator.addToQueue(search_tasks.at(i));
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
			ExpandOutcome out = tree.expand(search_tasks[i]);
			stats.nb_wasted_expansions += static_cast<uint64_t>(out == ExpandOutcome::ALREADY_EXPANDED);
		}
	}
	void Search::backup(Tree &tree)
	{
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
	/*
	 * private
	 */
	int Search::get_batch_size(int simulation_count) const noexcept
	{
//		int result = 2;
		int tmp = std::pow(2.0, std::log10(simulation_count)); // doubling batch size for every 10x increase of simulations count
		return std::max(1, std::min(search_config.max_batch_size, tmp));
//		for (int i = 1; i < simulation_count; i *= 10)
//			result *= 2; // doubling batch size for every 10x increase of simulations count
//		return std::min(search_config.max_batch_size, result);
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
			if (task.getLastPair().node == search_tasks[i].getLastPair().node)
				return true;
		return false;
	}

} /* namespace ag */

