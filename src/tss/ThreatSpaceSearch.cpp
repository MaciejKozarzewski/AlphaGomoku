/*
 * ThreatSpaceSearch.cpp
 *
 *  Created on: Oct 31, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/tss/ThreatSpaceSearch.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <iostream>
#include <bitset>
#include <cassert>

namespace
{
	using namespace ag;

	int get_max_nodes(ag::GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size - 1) / 2;
	}

	ProvenValue convertProvenValue(Score score) noexcept
	{
		switch (score.getProvenScore())
		{
			case ProvenScore::LOSS:
				return ProvenValue::LOSS;
			case ProvenScore::DRAW:
				return ProvenValue::DRAW;
			default:
			case ProvenScore::UNKNOWN:
				return ProvenValue::UNKNOWN;
			case ProvenScore::WIN:
				return ProvenValue::WIN;
		}
	}
	Value convertEvaluation(Score score) noexcept
	{
		switch (score.getProvenScore())
		{
			case ProvenScore::LOSS:
				return Value(0.0f, 0.0f, 1.0f);
			case ProvenScore::DRAW:
				return Value(0.0f, 1.0f, 0.0f);
			default:
			case ProvenScore::UNKNOWN:
				return Value();
//				return Value(0.5f + score.getEval() / 2000.0f);
			case ProvenScore::WIN:
				return Value(1.0f, 0.0f, 0.0f);
		}
	}

	void fill(SearchTask &task, const ActionList &actions, Score score)
	{
		task.getPriorEdges().clear();
		task.setMustDefendFlag(actions.must_defend);
		for (auto iter = actions.begin(); iter < actions.end(); iter++)
			task.addPriorEdge(iter->move, convertEvaluation(iter->score), convertProvenValue(iter->score));

		if (score.isProven())
		{
			task.setProvenValue(convertProvenValue(score));
			task.setValue(convertEvaluation(score));
			task.markAsReadySolver();
		}
	}
}

namespace ag
{
	/*
	 * TSSStats
	 */
	TSSStats::TSSStats() :
			setup("setup    "),
			static_solver("static   "),
			hashtable("hashtable"),
			move_generation("movegen  "),
			solve("solve    "),
			evaluate("evaluate ")
	{
	}
	std::string TSSStats::toString() const
	{
		std::string result = "----ThreatSpaceSearchStats----\n";
		result += setup.toString() + '\n';
		result += solve.toString() + '\n';
		result += static_solver.toString() + '\n';
		result += hashtable.toString() + '\n';
		result += move_generation.toString() + '\n';
		result += evaluate.toString() + '\n';
		if (setup.getTotalCount() > 0)
		{
			if (solve.getTotalCount() > 0)
				result += "total positions = " + std::to_string(total_positions) + " : " + std::to_string(total_positions / solve.getTotalCount())
						+ '\n';
			result += "solved    " + std::to_string(hits) + " / " + std::to_string(setup.getTotalCount()) + " ("
					+ std::to_string(100.0 * hits / setup.getTotalCount()) + "%)\n";
			result += "shared cache hits " + std::to_string(shared_cache_hits) + " / " + std::to_string(shared_cache_calls) + " ("
					+ std::to_string(100.0 * shared_cache_hits / (1.0e-6 + shared_cache_calls)) + "%)\n";
			result += "local cache hits " + std::to_string(local_cache_hits) + " / " + std::to_string(local_cache_calls) + " ("
					+ std::to_string(100.0 * local_cache_hits / (1.0e-6 + local_cache_calls)) + "%)\n";
		}
		return result;
	}

	/*
	 * Measurement
	 */
	Measurement::Measurement(int paramValue) noexcept :
			m_param_value(paramValue)
	{
	}
	void Measurement::clear() noexcept
	{
		m_values.clear();
	}
	int Measurement::getParamValue() const noexcept
	{
		return m_param_value;
	}
	void Measurement::update(int x, float y) noexcept
	{
		if (m_values.size() >= 10)
			m_values.erase(m_values.begin());
		m_values.push_back(std::pair<int, float>( { x, y }));
	}
	std::pair<float, float> Measurement::predict(int x) const noexcept
	{
		if (m_values.size() < 5)
			return std::pair<float, float>( { 0.0f, 1.0e6f });
		else
		{
			LinearRegression<int, float> linreg(m_values);
			return linreg.predict(x);
		}
	}
	std::string Measurement::toString() const
	{
		std::string result = "Measurements for param = " + std::to_string(m_param_value) + "\n";
		for (size_t i = 0; i < m_values.size(); i++)
			result += std::to_string(i) + " : " + std::to_string(m_values[i].first) + ", " + std::to_string(m_values[i].second) + "\n";
		return result;
	}

	/*
	 * ThreatSpaceSearch
	 */
	ThreatSpaceSearch::ThreatSpaceSearch(GameConfig gameConfig, int maxPositions) :
			action_stack(get_max_nodes(gameConfig)),
			number_of_moves_for_draw(gameConfig.rows * gameConfig.cols),
			max_positions(maxPositions),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			threat_generator(gameConfig, pattern_calculator),
//				evaluator(gameConfig),
			killers(gameConfig),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions)
	{
	}
	int64_t ThreatSpaceSearch::getMemory() const noexcept
	{
		return action_stack.size() * sizeof(Action);
	}
	void ThreatSpaceSearch::setSharedTable(std::shared_ptr<SharedHashTable<4>> table) noexcept
	{
		shared_table = table;
	}
	void ThreatSpaceSearch::solve(SearchTask &task, TssMode mode, int maxPositions)
	{
		stats.setup.startTimer();
		pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());
		task.getFeatures().encode(pattern_calculator);
		stats.setup.stopTimer();

		TimerGuard tg(stats.solve);
		max_positions = maxPositions;
		search_mode = mode;
		position_counter = 0;

		ActionList actions = action_stack.getRoot();
		Score score;
		switch (mode)
		{
			case TssMode::BASIC:
			case TssMode::STATIC:
			{
				TimerGuard tg(stats.move_generation);
				score = threat_generator.generate(actions, static_cast<GeneratorMode>(search_mode));
				if (score.isProven() or actions.must_defend)
					fill(task, actions, score);
				break;
			}
			case TssMode::RECURSIVE:
			default:
			{
				if (shared_table == nullptr)
					throw std::logic_error("ThreatSpaceSearch::solve() : Pointer to the shared hash table has not been initialized");

//					stats.evaluate.startTimer();
//					evaluator.refresh(pattern_calculator); // set up NNUE state
//					stats.evaluate.stopTimer();

				hash_key = shared_table->getHashFunction().getHash(task.getBoard()); // set up hash key
				const int max_depth = std::min(127, game_config.rows * game_config.cols - pattern_calculator.getCurrentDepth());
				for (int depth = 1; depth <= max_depth; depth += 4)
				{ // iterative deepening loop
//						double t0 = getTime();
//						score = vcf_search(depth, actions, true);
					score = recursive_solve(depth, Score::min_value(), Score::max_value(), actions, true);
//						std::cout << "depth " << depth << ", nodes " << position_counter << ", score " << score << ", time " << (getTime() - t0)
//								<< "s\n";
					if (score.isProven())
						break; // there is no point in further search if we have found some proven score
					if (position_counter >= max_positions)
						break; // the limit of nodes was exceeded
					if (actions.size() == 0)
						break; // apparently there are no threats to make
				}
				fill(task, actions, score);
				break;
			}
		}

//			if (actions.size() == 0)
//			{
//			std::cout << score.toString() << " at " << position_counter << '\n';
//			std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//			pattern_calculator.print();
//			pattern_calculator.printAllThreats();
//			actions.print();
//			std::cout << task.toString();
//			std::cout << "\n---------------------------------------------\n";
//				exit(-1);
//			}

		stats.total_positions += position_counter;
		stats.hits += static_cast<int>(score.isProven());
	}
	void ThreatSpaceSearch::tune(float speed)
	{
		Logger::write("ThreatSpaceSearch::tune(" + std::to_string(speed) + ")");
		Logger::write("Before new measurement");
		Logger::write(lower_measurement.toString());
		Logger::write(upper_measurement.toString());
		if (max_positions == lower_measurement.getParamValue())
		{
			lower_measurement.update(step_counter, speed);
			max_positions = upper_measurement.getParamValue();
			Logger::write("using upper " + std::to_string(max_positions));
		}
		else
		{
			upper_measurement.update(step_counter, speed);
			max_positions = lower_measurement.getParamValue();
			Logger::write("using lower " + std::to_string(max_positions));
		}

		Logger::write("After new measurement");
		Logger::write(lower_measurement.toString());
		Logger::write(upper_measurement.toString());

		step_counter++;

		const std::pair<float, float> lower_mean_and_stddev = lower_measurement.predict(step_counter);
		const std::pair<float, float> upper_mean_and_stddev = upper_measurement.predict(step_counter);

		Logger::write("Predicted");
		Logger::write("Lower : " + std::to_string(lower_mean_and_stddev.first) + " +/-" + std::to_string(lower_mean_and_stddev.second));
		Logger::write("Upper : " + std::to_string(upper_mean_and_stddev.first) + " +/-" + std::to_string(upper_mean_and_stddev.second));

		const float mean = lower_mean_and_stddev.first - upper_mean_and_stddev.first;
		const float stddev = std::hypot(lower_mean_and_stddev.second, upper_mean_and_stddev.second);
		Logger::write("mean = " + std::to_string(mean) + ", stddev = " + std::to_string(stddev));

		const float probability = 1.0f - gaussian_cdf(mean / stddev);
		Logger::write("probability = " + std::to_string(probability));

		if (probability > 0.95f) // there is more than 95% chance that higher value of 'max_positions' gives higher speed
		{
			if (lower_measurement.getParamValue() * tuning_step <= 6400)
			{
				const int new_max_pos = tuning_step * lower_measurement.getParamValue();
				lower_measurement = Measurement(new_max_pos);
				upper_measurement = Measurement(tuning_step * new_max_pos);
				max_positions = lower_measurement.getParamValue();
				Logger::write(
						"ThreatSpaceSearch increasing positions to " + std::to_string(max_positions) + " at probability "
								+ std::to_string(probability));
			}
		}
		if (probability < 0.05f) // there is less than 5% chance that higher value of 'max_positions' gives higher speed
		{
			if (lower_measurement.getParamValue() / tuning_step >= 25)
			{
				const int new_max_pos = lower_measurement.getParamValue() / tuning_step;
				lower_measurement = Measurement(new_max_pos);
				upper_measurement = Measurement(tuning_step * new_max_pos);
				max_positions = lower_measurement.getParamValue();
				Logger::write(
						"ThreatSpaceSearch decreasing positions to " + std::to_string(max_positions) + " at probability "
								+ std::to_string(probability));
			}
		}
	}
	TSSStats ThreatSpaceSearch::getStats() const
	{
		return stats;
	}
	void ThreatSpaceSearch::clearStats()
	{
		stats = TSSStats();
		max_positions = lower_measurement.getParamValue();
		lower_measurement.clear();
		upper_measurement.clear();
		step_counter = 0;
		Logger::write("ThreatSpaceSearch is using " + std::to_string(max_positions) + " positions");
	}
	/*
	 * private
	 */
	Score ThreatSpaceSearch::recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions, bool isRoot)
	{
		actions.clear();
		assert(Score::min_value() <= alpha);
		assert(alpha < beta);
		assert(beta <= Score::max_value());

		Move hash_move;
		{ // lookup to the shared hash table
//				TimerGuard tg(stats.hashtable);
			const SharedTableData tt_entry = shared_table->seek(hash_key);
			const Bound tt_bound = tt_entry.bound();

			stats.shared_cache_calls++;
			stats.shared_cache_hits += (tt_bound != Bound::NONE);

			if (tt_bound != Bound::NONE)
			{ // there is some information stored in this entry
				hash_move = tt_entry.move();
				if (is_move_legal(hash_move) and not isRoot)
				{
					const Score tt_score = tt_entry.score();
					if (tt_score.isProven()
							or (tt_entry.depth() >= depthRemaining
									and ((tt_bound == Bound::EXACT) or (tt_bound == Bound::LOWER and tt_score >= beta)
											or (tt_bound == Bound::UPPER and tt_score <= alpha))))
						return tt_score;
				}
			}
		}

		{ // threat generator is combined with static solver
//				TimerGuard tg(stats.move_generation);
			const Score static_score = threat_generator.generate(actions, static_cast<GeneratorMode>(search_mode));
			if (static_score.isProven())
				return static_score;
			apply_ordering(actions, hash_move, killers.get(pattern_calculator.getCurrentDepth()));
		}

//			if (isAttacker and not actions.has_initiative)
//				depthRemaining -= 4;
//			if (not isAttacker and not actions.must_defend)
//				depthRemaining -= 2;

		{ // evaluation
//				TimerGuard tg(stats.evaluate);
//				evaluator.update(pattern_calculator); // update the accumulator of NNUE
			if (depthRemaining <= 0) // if it is a leaf, evaluate the position
//					return Score((int) (2000 * evaluator.forward()) - 1000);
				return evaluate();
		}

		const Score original_alpha = alpha;
//			Score best_score = actions.must_defend ? Score::min_value() : Score::no_solution(); // setup initial value of the score
		Score best_score = Score::min_value(); // setup initial value of the score
		for (int i = 0; i < actions.size(); i++)
		{
			position_counter++;
			if (position_counter > max_positions)
				depthRemaining = 1; // a hack to make each node look like a leaf, so the search will complete correctly but will exit as soon as possible

			const Move move = actions[i].move;
			Score action_score;

			shared_table->getHashFunction().updateHash(hash_key, move);
			shared_table->prefetch(hash_key); // TODO check if this improves anything

//				const SharedTableData tt_entry = shared_table->seek(hash_key);
//				if (tt_entry.score().isProven())
//					action_score = -tt_entry.score();
//				else
			{
				ActionList next_ply_actions(actions, actions[i]);
				pattern_calculator.addMove(move);
				action_score = -recursive_solve(depthRemaining - 1, -beta, -alpha, next_ply_actions, false);
				pattern_calculator.undoMove(move);
			}
			shared_table->getHashFunction().updateHash(hash_key, move);

//				stats.evaluate.startTimer();
//				evaluator.update(pattern_calculator); // updating in reverse direction (undoing moves) does not cost anything but is required to correctly set current depth
//				stats.evaluate.stopTimer();

			action_score.increaseDistanceToWinOrLoss();
			actions[i].score = action_score; // required for recovering of the search results

			best_score = std::max(best_score, action_score);
			alpha = std::max(alpha, action_score);
			if (action_score >= beta or action_score.isWin())
//				if (action_score.isWin())
				break;
		}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but we don't have to defend
		// we need to override the score with something, for example with static evaluation
		if (actions.size() == 0 or (best_score.isLoss() and not actions.must_defend))
			best_score = evaluate();
//			score = Score((int) (2000 * evaluator.forward()) - 1000);

		if (best_score.isProven())
		{ // insert new data to the hash table
//				TimerGuard tg(stats.hashtable);
			const Move best_move = (actions.size() > 0) ? std::max_element(actions.begin(), actions.end())->move : Move();
			Bound bound = Bound::NONE;
			if (best_score <= original_alpha)
				bound = Bound::UPPER;
			else
			{
				if (best_score >= beta)
					bound = Bound::LOWER;
				else
					bound = Bound::EXACT;
			}
			shared_table->insert(hash_key,
					SharedTableData(actions.must_defend, actions.has_initiative, bound, depthRemaining, best_score, best_move));

			killers.insert(best_move, pattern_calculator.getCurrentDepth());
		}

		return best_score;
	}
	bool ThreatSpaceSearch::is_move_legal(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}
	void ThreatSpaceSearch::apply_ordering(ActionList &actions, Move hashMove, const ShortVector<Move, 4> &killers) const noexcept
	{
		int offset = static_cast<int>(actions.moveCloserToFront(hashMove, 0));
		for (int i = 0; i < killers.size(); i++)
			offset += static_cast<int>(actions.moveCloserToFront(killers[i], offset));
	}
	Score ThreatSpaceSearch::evaluate()
	{
		const Sign own_sign = pattern_calculator.getSignToMove();
		const Sign opponent_sign = invertSign(own_sign);
		const int worst_threat = static_cast<int>(ThreatType::OPEN_3);
		const int best_threat = static_cast<int>(ThreatType::FIVE);

		static const std::array<int, 10> own_threat_values = { 0, 0, 1, 5, 10, 50, 100, 100, 1000, 0 };
		static const std::array<int, 10> opp_threat_values = { 0, 0, 0, 0, 1, 5, 10, 10, 100, 0 };
		int result = 0;
		for (int i = worst_threat; i <= best_threat; i++)
		{
			result += own_threat_values[i] * pattern_calculator.getThreatHistogram(own_sign).get(static_cast<ThreatType>(i)).size();
			result -= opp_threat_values[i] * pattern_calculator.getThreatHistogram(opponent_sign).get(static_cast<ThreatType>(i)).size();
		}
		return std::max(-4000, std::min(4000, result));
	}

} /* namespace ag */
