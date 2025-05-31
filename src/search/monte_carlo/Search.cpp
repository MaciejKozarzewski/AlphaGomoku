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
		return *this;
	}
	double SearchStats::getTotalTime() const noexcept
	{
		return select.getTotalTime() + solve.getTotalTime() + schedule.getTotalTime() + generate.getTotalTime() + expand.getTotalTime()
				+ backup.getTotalTime();
	}

	Search::Search(const GameConfig &gameOptions, const SearchConfig &searchOptions) :
			ab_search(gameOptions),
			game_config(gameOptions),
			search_config(searchOptions)
	{
		for (int i = 0; i < 2; i++)
			tasks_list_buffer[i] = SearchTaskList(gameOptions, searchOptions.max_batch_size);
	}
	int64_t Search::getMemory() const noexcept
	{
		return ab_search.getMemory();
	}
	const SearchConfig& Search::getConfig() const noexcept
	{
		return search_config;
	}
	AlphaBetaSearch& Search::getSolver() noexcept
	{
		return ab_search;
	}
	void Search::clearStats() noexcept
	{
		stats = SearchStats();
	}
	SearchStats Search::getStats() const noexcept
	{
//		ab_search.print_stats();
		return stats;
	}
	void Search::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		ab_search.increaseGeneration();
	}
	void Search::select(Tree &tree, int maxSimulations)
	{
		int number_of_trials = 2 * get_buffer().maxSize();

		while (get_buffer().storedElements() < get_buffer().maxSize() and tree.getSimulationCount() <= maxSimulations)
		{
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
				break; // the search can be continued but some neural network evaluations would be wasted
			}
			if (out == SelectOutcome::INFORMATION_LEAK)
			{
				tree.correctInformationLeak(current_task);
				tree.cancelVirtualLoss(current_task);
				stats.nb_information_leaks++;
				get_buffer().removeLast();
			}
			if (out == SelectOutcome::REACHED_PROVEN_EDGE)
			{ // this was added to allow the search to correctly continue even if the tree is proven and we re-visit already proven edges
				assert(current_task.visitedPathLength() > 0);
				const Score s = current_task.getLastEdge()->getScore();
				current_task.setScore(s);
				current_task.setValue(s.convertToValue());
				current_task.markAsProcessedBySolver();
				current_task.markToSkipEdgeGeneration();
				stats.nb_proven_states++;
			}

//			// in theory we can keep hitting information leaks forever
//			// this is why we exit the loop after certain number of search tasks
			number_of_trials--;
			if (number_of_trials <= 0)
				break;
		}
	}
	void Search::solve(double endTime)
	{
		stats.solve.startTimer();
		ab_search.setDepthLimit(100);
		if (endTime < 0.0)
		{
			ab_search.setTimeLimit(std::numeric_limits<double>::max());
			ab_search.setNodeLimit(search_config.tss_config.max_positions);
		}
		else
			ab_search.setNodeLimit(10000);

		for (int i = 0; i < get_buffer().storedElements(); i++)
			if (not get_buffer().get(i).wasProcessedBySolver())
			{
				if (endTime >= 0.0)
				{
//					std::cout << i << "/" << getBatchSize() << ", time = " << 1.0e6 * (endTime - getTime()) / (getBatchSize() - i) << "us ("
//							<< 1.0e6 * (endTime - getTime()) << "us)\n";
					ab_search.setTimeLimit((endTime - getTime()) / (getBatchSize() - i));
				}
				ab_search.solve(get_buffer().get(i));
			}
		stats.solve.stopTimer(get_buffer().storedElements());
	}
	void Search::scheduleToNN(NNEvaluator &evaluator)
	{
		stats.schedule.startTimer();
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			const bool is_root = get_buffer().get(i).visitedPathLength() == 0;
			const bool is_proven = get_buffer().get(i).getScore().isProven();
			if (is_root or not is_proven)
			{ // schedule only those tasks that haven't already been solved by the solver unless it's a root node
				stats.nb_network_evaluations++;
				evaluator.addToQueue(get_buffer().get(i));
			}
		}
		stats.schedule.stopTimer(get_buffer().storedElements());
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
		stats.generate.startTimer();
		for (int i = 0; i < get_buffer().storedElements(); i++)
			if (not get_buffer().get(i).skipEdgeGeneration())
				tree.generateEdges(get_buffer().get(i));
		stats.generate.stopTimer(get_buffer().storedElements());
	}
	void Search::expand(Tree &tree)
	{
		stats.expand.startTimer();
		for (int i = 0; i < get_buffer().storedElements(); i++)
		{
			const ExpandOutcome out = tree.expand(get_buffer().get(i));
			stats.nb_wasted_expansions += static_cast<uint64_t>(out == ExpandOutcome::ALREADY_EXPANDED);
		}
		stats.expand.stopTimer(get_buffer().storedElements());
	}
	void Search::backup(Tree &tree)
	{
		stats.nb_node_count += get_buffer().storedElements();
		stats.backup.startTimer();
		for (int i = 0; i < get_buffer().storedElements(); i++)
			tree.backup(get_buffer().get(i));
		stats.backup.stopTimer(get_buffer().storedElements());
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
	int Search::getBatchSize() const noexcept
	{
		return get_buffer().storedElements();
	}
	/*
	 * private
	 */
	const SearchTaskList& Search::get_buffer() const noexcept
	{
		assert(current_task_buffer == 0 || current_task_buffer == 1);
		return tasks_list_buffer[current_task_buffer];
	}
	SearchTaskList& Search::get_buffer() noexcept
	{
		assert(current_task_buffer == 0 || current_task_buffer == 1);
		return tasks_list_buffer[current_task_buffer];
	}
	bool Search::is_duplicate(const SearchTask &task) const noexcept
	{
		for (int i = 0; i < get_buffer().storedElements() - 1; i++)
			if (task.getLastPair().edge == get_buffer().get(i).getLastPair().edge)
				return true;
		return false;
	}

} /* namespace ag */

