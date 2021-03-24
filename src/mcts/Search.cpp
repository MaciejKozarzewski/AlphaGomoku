/*
 * Search.cpp
 *
 *  Created on: Mar 20, 2021
 *      Author: maciek
 */

#include <alphagomoku/mcts/Search.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/mcts/Tree.hpp>
#include <alphagomoku/mcts/Cache.hpp>
#include <alphagomoku/mcts/EvaluationQueue.hpp>

namespace ag
{
	std::string SearchStats::toString() const
	{
		std::string result;
		result += "----SearchStats----\n";
		result += "nb_cache_hits = " + std::to_string(nb_cache_hits) + '\n';
		result += "nb_cache_calls = " + std::to_string(nb_cache_calls) + '\n';
		result += "nb_duplicate_nodes = " + std::to_string(nb_duplicate_nodes) + '\n';
		result += printStatistics("select", nb_select, time_select);
		result += printStatistics("expand", nb_expand, time_expand);
		result += printStatistics("backup", nb_backup, time_backup);
		result += printStatistics("evaluate", nb_evaluate, time_evaluate);
		result += printStatistics("game_rules", nb_game_rules, time_game_rules);
		return result;
	}
	SearchStats& SearchStats::operator+=(const SearchStats &other) noexcept
	{
		this->nb_cache_hits += other.nb_cache_hits;
		this->nb_cache_calls += other.nb_cache_calls;
		this->nb_select += other.nb_select;
		this->nb_expand += other.nb_expand;
		this->nb_backup += other.nb_backup;
		this->nb_evaluate += other.nb_evaluate;
		this->nb_game_rules += other.nb_game_rules;
		this->nb_duplicate_nodes += other.nb_duplicate_nodes;

		this->time_select += other.time_select;
		this->time_expand += other.time_expand;
		this->time_backup += other.time_backup;
		this->time_evaluate += other.time_evaluate;
		this->time_game_rules += other.time_game_rules;
		return *this;
	}

	Search::Search(GameConfig gameOptions, SearchConfig searchOptions, Tree &tree, Cache &cache, EvaluationQueue &queue) :
			cache(cache),
			tree(tree),
			eval_queue(queue),
			current_board(gameOptions.rows, gameOptions.cols),
			sign_to_move(Sign::NONE),
			game_config(gameOptions),
			search_config(searchOptions)
	{
		moves_to_add.reserve(gameOptions.rows * gameOptions.cols);
		search_buffer.reserve(search_config.batch_size);
	}
	SearchStats Search::getStats() const noexcept
	{
		return stats;
	}
	SearchConfig Search::getConfig() const noexcept
	{
		return search_config;
	}
	int Search::getSimulationCount() const noexcept
	{
		return simulation_count;
	}

	void Search::setBoard(const matrix<Sign> &board)
	{
		current_board = board;

		Sign root_node_move_sign = Move::getSign(tree.getRootNode().getMove());
		assert(root_node_move_sign != Sign::NONE);
		sign_to_move = invertSign(root_node_move_sign);
	}
	matrix<Sign> Search::getBoard() const noexcept
	{
		return current_board;
	}

	void Search::handleEvaluation()
	{
		for (size_t i = 0; i < search_buffer.size(); i++)
			if (search_buffer[i].position.isReady() == false)
				return; // cannot continue as there are unprocessed requests

		for (auto entry = search_buffer.begin(); entry < search_buffer.end(); entry++)
		{
			evaluate(entry->position);
			expand(entry->position);
			backup(*entry);
			cache.insert(entry->position);
		}
		search_buffer.clear();
	}
	void Search::simulate(int maxSimulations, bool verbose)
	{
		assert(maxSimulations > 0);
		while (static_cast<int>(search_buffer.size()) < search_config.batch_size and simulation_count < maxSimulations and not tree.isProven())
		{
			if (verbose)
			{
				std::cout << '\n';
				tree.printSubtree(tree.getRootNode(), -1, false, 5);
			}
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
					return;
				}
				else
				{
					stats.nb_cache_calls++; // statistics
					if (cache.seek(request.position))
					{
						// node is not terminal, not a duplicate, but found in cache
						if (tree.isRootNode(request.position.getNode()))
							request.position.setProvenValue(ProvenValue::UNKNOWN); // erase proven value from root to force at least 1-ply search
						expand(request.position);
						backup(request);
						search_buffer.pop_back();
						stats.nb_cache_hits++; // statistics
					}
					else
					{
						// node is not terminal, not a duplicate and not in cache
						if (search_config.augment_position == true)
							request.position.augment();
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
	}
	void Search::cleanup()
	{
		simulation_count = 0;
		for (auto entry = search_buffer.begin(); entry < search_buffer.end(); entry++)
			tree.cancelVirtualLoss(entry->trajectory);
		search_buffer.clear();
	}
	void Search::clearStats() noexcept
	{
		stats = SearchStats();
	}
//private
	Search::SearchRequest& Search::select()
	{
		double start = getTime(); // statistics
		simulation_count++;
		search_buffer.push_back(SearchRequest(game_config.rows, game_config.cols)); // add new request
		tree.select(search_buffer.back().trajectory, search_config.exploration_constant); // select node to evaluate
		search_buffer.back().position.setTrajectory(current_board, search_buffer.back().trajectory); // update board in evaluation request

		stats.nb_select++; //statistics
		stats.time_select += getTime() - start; //statistics
		return search_buffer.back();
	}
	GameOutcome Search::evaluateFromGameRules(EvaluationRequest &position)
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
		stats.nb_game_rules++; // statistics
		stats.time_game_rules += getTime() - start; //statistics
		return outcome;
	}
	void Search::evaluate(EvaluationRequest &position)
	{
		double start = getTime(); //statistics
		if (search_config.augment_position == true)
			position.augment();

		if (tree.isRootNode(position.getNode()))
			addNoise(position.getBoard(), position.getPolicy(), search_config.noise_weight);

		stats.nb_evaluate++; // statistics
		stats.time_evaluate += getTime() - start; //statistics
	}
	void Search::expand(EvaluationRequest &position)
	{
		if (position.getProvenValue() != ProvenValue::UNKNOWN)
			return; // do not expand proven nodes

		double start = getTime(); // statistics
		const int cols = position.getBoard().cols();
		maskIllegalMoves(position.getBoard(), position.getPolicy());

		moves_to_add.clear();
		for (int i = 0; i < position.getBoard().size(); i++)
			if (position.getBoard().data()[i] == Sign::NONE and position.getPolicy().data()[i] >= search_config.expansion_prior_treshold)
				moves_to_add.push_back( { Move::move_to_short(i / cols, i % cols, position.getSignToMove()), position.getPolicy().data()[i] });
		tree.expand(*position.getNode(), moves_to_add);

		stats.nb_expand++; // statistics
		stats.time_expand += getTime() - start; //statistics
	}
	void Search::backup(SearchRequest &request)
	{
		double start = getTime();
		tree.backup(request.trajectory, request.position.getValue(), request.position.getProvenValue());
		stats.nb_backup++; // statistics
		stats.time_backup += getTime() - start; //statistics
	}
	bool Search::isDuplicate(EvaluationRequest &position)
	{
		for (size_t i = 0; i < search_buffer.size() - 1; i++)
			if (position.getNode() == search_buffer[i].position.getNode())
				return true;
		return false;
	}

} /* namespace ag */

