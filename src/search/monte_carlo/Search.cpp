/*
 * Search.cpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Search.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
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
		result += "nb_proven_states       = " + std::to_string(nb_proven_states) + '\n';
		result += "nb_network_evaluations = " + std::to_string(nb_network_evaluations) + '\n';
		result += "nb_node_count          = " + std::to_string(nb_node_count) + '\n';
		result += "avg batch size         = " + std::to_string((double) nb_batch_size / select.getTotalCount()) + '\n';
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
		this->nb_proven_states += other.nb_proven_states;
		this->nb_network_evaluations += other.nb_network_evaluations;
		this->nb_node_count += other.nb_node_count;
		this->nb_batch_size += other.nb_batch_size;
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
		this->nb_proven_states /= i;
		this->nb_network_evaluations /= i;
		this->nb_node_count /= i;
		this->nb_batch_size /= i;
		return *this;
	}
	double SearchStats::getTotalTime() const noexcept
	{
		return select.getTotalTime() + solve.getTotalTime() + schedule.getTotalTime() + generate.getTotalTime() + expand.getTotalTime()
				+ backup.getTotalTime();
	}

	Search::Search(GameConfig gameOptions, SearchConfig searchOptions) :
			tasks_list_buffer_0(gameOptions, searchOptions.max_batch_size),
			tasks_list_buffer_1(gameOptions, searchOptions.max_batch_size),
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
		return stats;
	}

	void Search::select(Tree &tree, int maxSimulations)
	{
		solver.setSharedTable(tree.getSharedHashTable());

		int number_of_trials = 2 * get_buffer().maxSize();

		while (get_buffer().storedElements() < get_buffer().maxSize() and tree.getSimulationCount() <= maxSimulations and not tree.isRootProven())
		{
			stats.nb_batch_size++;
			TimerGuard timer(stats.select);
			SearchTask &current_task = get_buffer().getNext();

			const SelectOutcome out = tree.select(current_task);

			if (current_task.visitedPathLength() == 0)
				break; // visited only root node
			if (is_duplicate(current_task))
			{
				tree.cancelVirtualLoss(current_task);
				get_buffer().removeLast();
				stats.nb_duplicate_nodes++;
				break; // in theory the search can be continued but some neural network evaluations would be wasted
			}
			if (out == SelectOutcome::INFORMATION_LEAK)
			{
				tree.correctInformationLeak(current_task);
				tree.cancelVirtualLoss(current_task);
				stats.nb_information_leaks++;
				get_buffer().removeLast();
			}
			if (out == SelectOutcome::REACHED_PROVEN_STATE)
			{ // this was added to allow the search to correctly continue even if the tree is proven and we re-visit already proven edges
				assert(current_task.visitedPathLength() > 0);
				Score s = -current_task.getLastEdge()->getScore();
				s.decreaseDistance(); // we get score from an edge, so we pretend it comes from a node one ply deeper
				current_task.setScore(s);
				current_task.setValue(s.convertToValue());
				current_task.markAsProcessedBySolver();
				tree.backup(current_task); // we just propagate proven value and score (again)
				stats.nb_proven_states++;
				get_buffer().removeLast();
			}

			// in theory we can keep hitting information leaks of proven states forever
			// this is why we exit the loop after certain number of search tasks
			number_of_trials--;
			if (number_of_trials <= 0)
				break;
		}
	}
	void Search::solve()
	{
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			TimerGuard timer(stats.solve);
			solver.solve(get_buffer().get(i), static_cast<TssMode>(search_config.solver_level), search_config.solver_max_positions);
		}
	}
	void Search::scheduleToNN(NNEvaluator &evaluator)
	{
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			TimerGuard timer(stats.schedule);
			const bool is_root = get_buffer().get(i).visitedPathLength() == 0;
			const bool is_proven = get_buffer().get(i).getScore().isProven();
			if (is_root or not is_proven)
			{ // schedule only those tasks that haven't already been solved by the solver or if it is a root node
				stats.nb_network_evaluations++;
				evaluator.addToQueue(get_buffer().get(i));
			}
		}
	}
	bool Search::areTasksReady() const noexcept
	{
		for (int i = 0; i < get_buffer().storedElements(); i++)
			if (not get_buffer().get(i).isReady())
				return false;
		return true;
	}
	void Search::generateEdges(const Tree &tree)
	{
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			TimerGuard timer(stats.generate);
			tree.generateEdges(get_buffer().get(i));
		}
	}
	void Search::expand(Tree &tree)
	{
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			TimerGuard timer(stats.expand);
			const ExpandOutcome out = tree.expand(get_buffer().get(i));
			stats.nb_wasted_expansions += static_cast<uint64_t>(out == ExpandOutcome::ALREADY_EXPANDED);
		}
	}
	void Search::backup(Tree &tree)
	{
		stats.nb_node_count += get_buffer().storedElements();
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			TimerGuard timer(stats.backup);
			tree.backup(get_buffer().get(i));
		}
		get_buffer().clear();
	}
	void Search::cleanup(Tree &tree)
	{
		for (int b = 0; b < 2; b++)
		{
			useBuffer(b);
			for (int i = 0; i < get_buffer().storedElements(); i++)
				tree.cancelVirtualLoss(get_buffer().get(i));
			get_buffer().clear();
		}
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
	void Search::useBuffer(int index) noexcept
	{
		assert(index == 0 || index == 1);
		current_task_buffer = index;
	}
	void Search::switchBuffer() noexcept
	{
		current_task_buffer = 1 - current_task_buffer;
	}
	void Search::setBatchSize(int batchSize) noexcept
	{
		get_buffer().resize(batchSize);
	}
	/*
	 * private
	 */
	const SearchTaskList& Search::get_buffer() const noexcept
	{
		return (current_task_buffer == 0) ? tasks_list_buffer_0 : tasks_list_buffer_1;
	}
	SearchTaskList& Search::get_buffer() noexcept
	{
		return (current_task_buffer == 0) ? tasks_list_buffer_0 : tasks_list_buffer_1;
	}
	bool Search::is_duplicate(const SearchTask &task) const noexcept
	{
		for (int i = 0; i < get_buffer().storedElements() - 1; i++)
			if (task.getLastPair().edge == get_buffer().get(i).getLastPair().edge)
				return true;
		return false;
	}

} /* namespace ag */

