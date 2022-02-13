/*
 * Search.cpp
 *
 *  Created on: Mar 20, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>
#include <alphagomoku/mcts/EdgeSelector.hpp>
#include <alphagomoku/mcts/EdgeGenerator.hpp>

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
//		std::cout << "-----------------------------------------------------------------------------------\n";
		for (int i = 0; i < active_task_count; i++)
		{
			TimerGuard timer(stats.expand);
//			std::cout << search_tasks[i].getLastEdge() << '\n';
//			std::cout << search_tasks[i].toString() << '\n' << std::endl;
			ExpandOutcome out = tree.expand(search_tasks[i]);
			stats.nb_wasted_expansions += static_cast<uint64_t>(out == ExpandOutcome::ALREADY_EXPANDED);
		}

//		if (search_config.use_vcf_solver)
//		{
//			double start = getTime(); // statistics
//			vcf_solver.setBoard(position.getBoard(), position.getSignToMove());
//			ProvenValue pv = vcf_solver.solve(position.getPolicy(), moves_to_add);
//			if (pv != ProvenValue::UNKNOWN and search_config.use_endgame_solver)
//				position.setProvenValue(pv);
//			switch (pv)
//			{
//				case ProvenValue::LOSS:
//					position.setValue( { 0.0f, 0.0f, 1.0f });
//					break;
//				case ProvenValue::DRAW:
//					position.setValue( { 0.0f, 1.0f, 0.0f });
//					break;
//				case ProvenValue::WIN:
//					position.setValue( { 1.0f, 0.0f, 0.0f });
//					break;
//				default:
//					break;
//			}
//			stats.nb_vcf_solver++; // statistics
//			stats.time_vcf_solver += getTime() - start; //statistics
//		}
//
//		double start = getTime(); // statistics
//		const int cols = position.getBoard().cols();
//		if (moves_to_add.size() == 0) // if there are no immediate threats from VCF solver
//		{
//			for (int i = 0; i < position.getBoard().size(); i++)
//				if (position.getBoard().data()[i] == Sign::NONE and position.getPolicy().data()[i] >= search_config.expansion_prior_treshold)
//					moves_to_add.push_back( { Move::move_to_short(i / cols, i % cols, position.getSignToMove()), position.getPolicy().data()[i] });
//
//			if (static_cast<int>(moves_to_add.size()) > search_config.max_children)
//			{
//				std::partial_sort(moves_to_add.begin(), moves_to_add.begin() + search_config.max_children, moves_to_add.end(),
//						[](const std::pair<uint16_t, float> &lhs, const std::pair<uint16_t, float> &rhs)
//						{	return lhs.second > rhs.second;});
//				moves_to_add.erase(moves_to_add.begin() + search_config.max_children, moves_to_add.end());
//			}
//		}
//
//		if (static_cast<int>(moves_to_add.size()) < position.getBoard().size()) // renormalization of policy priors
//		{
//			float sum_priors = std::accumulate(moves_to_add.begin(), moves_to_add.end(), 0.0f, [](float acc, const std::pair<uint16_t, float> &p)
//			{	return acc + p.second;});
//			if (sum_priors > 0.0f)
//			{
//				sum_priors = 1.0f / sum_priors;
//				std::for_each(moves_to_add.begin(), moves_to_add.end(), [sum_priors](std::pair<uint16_t, float> &p)
//				{	p.second *= sum_priors;});
//			}
//		}
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

	Search_old::Search_old(GameConfig gameOptions, SearchConfig searchOptions, Tree_old &tree, Cache &cache, EvaluationQueue &queue) :
			cache(cache),
			tree(tree),
			eval_queue(queue),
			vcf_solver(gameOptions),
			current_board(gameOptions.rows, gameOptions.cols),
			sign_to_move(Sign::NONE),
			game_config(gameOptions),
			search_config(searchOptions)
	{
		moves_to_add.reserve(gameOptions.rows * gameOptions.cols);
		search_buffer.reserve(search_config.max_batch_size);
	}
	void Search_old::clearStats() noexcept
	{
		stats = SearchStats();
	}
	void Search_old::printSolverStats() const
	{
		vcf_solver.print_stats();
	}
	SearchStats Search_old::getStats() const noexcept
	{
		return stats;
	}
	SearchConfig Search_old::getConfig() const noexcept
	{
		return search_config;
	}
	int Search_old::getSimulationCount() const noexcept
	{
		return simulation_count;
	}

	void Search_old::setBoard(const matrix<Sign> &board)
	{
		current_board = board;

		Sign root_node_move_sign = Move::getSign(tree.getRootNode().getMove());
		assert(root_node_move_sign != Sign::NONE);
		sign_to_move = invertSign(root_node_move_sign);
	}
	matrix<Sign> Search_old::getBoard() const noexcept
	{
		return current_board;
	}

	void Search_old::handleEvaluation()
	{
		for (size_t i = 0; i < search_buffer.size(); i++)
			if (search_buffer[i].position.isReady() == false)
				return; // cannot continue as there are unprocessed requests

		for (auto entry = search_buffer.begin(); entry < search_buffer.end(); entry++)
		{
			evaluate(entry->position);
			bool flag = expand(entry->position);
			if (flag)
				backup(*entry);
			cache.insert(entry->position);
		}
		search_buffer.clear();
	}
	bool Search_old::simulate(int maxSimulations)
	{
		assert(maxSimulations > 0);
		while (static_cast<int>(search_buffer.size()) < get_batch_size())
		{
			if (simulation_count >= maxSimulations or tree.isProven())
				return false; // search cannot be continued
			SearchRequest &request = select();
			if (evaluateFromGameRules(request.position) == GameOutcome::UNKNOWN)
			{
				// schedule for network evaluation
				if (isDuplicate(request.position))
				{
					// node is not terminal but has already been scheduled
					tree.cancelVirtualLoss(request.trajectory);
					search_buffer.pop_back();
					simulation_count--;
					stats.nb_duplicate_nodes++; //statistics
					return true;
				}
				else
				{
					if (cache.seek(request.position))
					{
						// node is not terminal, not a duplicate, but found in cache
						if (tree.isRootNode(request.position.getNode()))
							request.position.setProvenValue(ProvenValue::UNKNOWN); // erase proven value from root to force at least 1-ply search
						bool flag = expand(request.position);
						if (flag)
							backup(request);
						search_buffer.pop_back();
					}
					else
					{
						// node is not terminal, not a duplicate and not in cache
						eval_queue.addToQueue(request.position);
					}
				}
			}
			else
			{
				// node is terminal and has already been evaluated from game rules
				backup(request);
				search_buffer.pop_back();
			}
		}
		return true;
	}
	void Search_old::cleanup()
	{
		simulation_count = 0;
		for (auto entry = search_buffer.begin(); entry < search_buffer.end(); entry++)
			tree.cancelVirtualLoss(entry->trajectory);
		search_buffer.clear();
	}
//private
	int Search_old::get_batch_size() const noexcept
	{
		int result = 2;
		for (int i = 1; i < simulation_count; i *= 10)
			result *= 2; // doubling batch size for every 10x increase of simulations count
		return std::min(search_config.max_batch_size, result);
	}
	Search_old::SearchRequest& Search_old::select()
	{
		double start = getTime(); // statistics
		simulation_count++;
		search_buffer.push_back(SearchRequest(game_config.rows, game_config.cols)); // add new request
		tree.select(search_buffer.back().trajectory, search_config.exploration_constant); // select node to evaluate
		search_buffer.back().position.setTrajectory(current_board, search_buffer.back().trajectory); // update board in evaluation request

//		stats.nb_select++; //statistics
//		stats.time_select += getTime() - start; //statistics
		return search_buffer.back();
	}
	GameOutcome Search_old::evaluateFromGameRules(EvaluationRequest &position)
	{
		double start = getTime(); // statistics
		GameOutcome outcome = getOutcome(game_config.rules, position.getBoard(), position.getLastMove());
		if (outcome != GameOutcome::UNKNOWN)
		{
			position.setReady();
			position.setValue(convertOutcome(outcome, position.getSignToMove()));
			if (search_config.use_endgame_solver)
				position.setProvenValue(convertProvenValue(outcome, position.getSignToMove()));
		}
//		stats.nb_game_rules++; // statistics
//		stats.time_game_rules += getTime() - start; //statistics
		return outcome;
	}
	void Search_old::evaluate(EvaluationRequest &position)
	{
		double start = getTime(); //statistics

		if (tree.isRootNode(position.getNode()))
			addNoise(position.getBoard(), position.getPolicy(), search_config.noise_weight);

//		stats.nb_evaluate++; // statistics
//		stats.time_evaluate += getTime() - start; //statistics
	}
	bool Search_old::expand(EvaluationRequest &position)
	{
		if (position.getProvenValue() != ProvenValue::UNKNOWN)
			return true; // do not expand proven nodes

		moves_to_add.clear();
		if (search_config.vcf_solver_level)
		{
			double start = getTime(); // statistics
			vcf_solver.setBoard(position.getBoard(), position.getSignToMove());
			ProvenValue pv = vcf_solver.solve(position.getPolicy(), moves_to_add);
			if (pv != ProvenValue::UNKNOWN and search_config.use_endgame_solver)
				position.setProvenValue(pv);
			switch (pv)
			{
				case ProvenValue::LOSS:
					position.setValue( { 0.0f, 0.0f, 1.0f });
					break;
				case ProvenValue::DRAW:
					position.setValue( { 0.0f, 1.0f, 0.0f });
					break;
				case ProvenValue::WIN:
					position.setValue( { 1.0f, 0.0f, 0.0f });
					break;
				default:
					break;
			}
//			stats.nb_vcf_solver++; // statistics
//			stats.time_vcf_solver += getTime() - start; //statistics
		}

		double start = getTime(); // statistics
		const int cols = position.getBoard().cols();
		if (moves_to_add.size() == 0) // if there are no immediate threats from VCF solver
		{
			for (int i = 0; i < position.getBoard().size(); i++)
				if (position.getBoard().data()[i] == Sign::NONE and position.getPolicy().data()[i] >= search_config.expansion_prior_treshold)
					moves_to_add.push_back( { Move::move_to_short(i / cols, i % cols, position.getSignToMove()), position.getPolicy().data()[i] });

			if (static_cast<int>(moves_to_add.size()) > search_config.max_children)
			{
				std::partial_sort(moves_to_add.begin(), moves_to_add.begin() + search_config.max_children, moves_to_add.end(),
						[](const std::pair<uint16_t, float> &lhs, const std::pair<uint16_t, float> &rhs)
						{	return lhs.second > rhs.second;});
				moves_to_add.erase(moves_to_add.begin() + search_config.max_children, moves_to_add.end());
			}
		}

		if (static_cast<int>(moves_to_add.size()) < position.getBoard().size()) // renormalization of policy priors
		{
			float sum_priors = std::accumulate(moves_to_add.begin(), moves_to_add.end(), 0.0f, [](float acc, const std::pair<uint16_t, float> &p)
			{	return acc + p.second;});
			if (sum_priors > 0.0f)
			{
				sum_priors = 1.0f / sum_priors;
				std::for_each(moves_to_add.begin(), moves_to_add.end(), [sum_priors](std::pair<uint16_t, float> &p)
				{	p.second *= sum_priors;});
			}
		}

		bool result = tree.expand(*position.getNode(), moves_to_add);

//		stats.nb_expand++; // statistics
//		stats.time_expand += getTime() - start; //statistics
		return result;
	}
	void Search_old::backup(SearchRequest &request)
	{
		double start = getTime();
		tree.backup(request.trajectory, request.position.getValue(), request.position.getProvenValue());
//		stats.nb_backup++; // statistics
//		stats.time_backup += getTime() - start; //statistics
	}
	bool Search_old::isDuplicate(EvaluationRequest &position)
	{
		for (size_t i = 0; i < search_buffer.size() - 1; i++)
			if (position.getNode() == search_buffer[i].position.getNode())
				return true;
		return false;
	}

} /* namespace ag */

