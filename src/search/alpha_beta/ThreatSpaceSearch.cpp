/*
 * ThreatSpaceSearch.cpp
 *
 *  Created on: Oct 31, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/alpha_beta/ThreatSpaceSearch.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <iostream>
#include <bitset>
#include <cassert>

namespace
{
	using namespace ag;
	int get_max_nodes(GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size - 1) / 2;
	}
	bool is_score_valid(Score score, int searchedDepth) noexcept
	{
		if (score.isProven())
			return searchedDepth >= score.getDistance();
		else
			return true;
	}
	Value convertScoreToValue(Score score) noexcept
	{
		switch (score.getProvenValue())
		{
			case ProvenValue::LOSS:
				return Value::loss();
			case ProvenValue::DRAW:
				return Value::draw();
			default:
			case ProvenValue::UNKNOWN:
				return Value();
			case ProvenValue::WIN:
				return Value::win();
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
			result += "shared cache hits " + std::to_string(cache_hits) + " / " + std::to_string(cache_calls) + " ("
					+ std::to_string(100.0 * cache_hits / (1.0e-6 + cache_calls)) + "%), collisions " + std::to_string(cache_colissions) + "\n";
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
	ThreatSpaceSearch::ThreatSpaceSearch(const GameConfig &gameConfig, const TSSConfig &tssConfig) :
			action_stack(get_max_nodes(gameConfig)),
			max_positions(tssConfig.max_positions),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			threat_generator(gameConfig, pattern_calculator),
			shared_table(gameConfig.rows, gameConfig.cols, tssConfig.hash_table_size),
			local_table(gameConfig.rows, gameConfig.cols, tssConfig.hash_table_size),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions)
	{
	}
	int64_t ThreatSpaceSearch::getMemory() const noexcept
	{
		return action_stack.size() * sizeof(Action);
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
		Result result(Score(), false);
		switch (mode)
		{
			default:
			case TssMode::BASIC:
			{
//				TimerGuard tg(stats.move_generation);
				const Score score = threat_generator.generate(actions, GeneratorMode::BASIC);
				result = Result(score, true);
				break;
			}
			case TssMode::STATIC:
			{
//				TimerGuard tg(stats.move_generation);
				const Score score = threat_generator.generate(actions, GeneratorMode::STATIC);
				result = Result(score, true);
				break;
			}
			case TssMode::RECURSIVE:
			{
//				hash_key = shared_table.getHashFunction().getHash(task.getBoard()); // set up hash key
				hash_key = local_table.getHashFunction().getHash(task.getBoard()); // set up hash key
				const int max_depth = std::max(1, std::min(127, game_config.draw_after - pattern_calculator.getCurrentDepth()));
				for (int depth = 1; depth <= max_depth; depth += 4)
				{ // iterative deepening loop
//					double t0 = getTime();
					result = recursive_solve_v2(depth, Score::min_value(), Score::max_value(), actions, true);

//					std::cout << "max depth=" << depth << ", nodes=" << position_counter << ", score=" << result.score << ", time="
//							<< (getTime() - t0) << "s, is final=" << result.is_final << "\n";
//					actions.print();

					if ((position_counter >= max_positions) or result.is_final)
						break;
				}
				break;
			}
		}

		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move m(iter->move);
			task.getActionScores().at(m.row, m.col) = iter->is_final ? iter->score : Score();
			if (iter->is_final and iter->score.isProven())
				task.getActionValues().at(m.row, m.col) = convertScoreToValue(iter->score);
			if (actions.must_defend)
				task.addDefensiveMove(m);
		}
		task.setScore(result.is_final ? result.score : Score());
		if (result.is_final and result.score.isProven())
			task.setValue(convertScoreToValue(result.score));
		task.markAsProcessedBySolver();

//		if (result.score.getEval() ==  Score::min_value())
//		if (task.getScore().getEval() == Score::min_value())
//		{
//		std::cout << result.score.toString() << " at " << position_counter << '\n';
//		std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//		pattern_calculator.print();
//		pattern_calculator.printAllThreats();
//		actions.print();
//		std::cout << task.toString();
//		std::cout << "\n---------------------------------------------\n";
//			exit(-1);
//		}

		stats.total_positions += position_counter;
		stats.hits += static_cast<int>(result.score.isProven() and result.is_final);
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
	ThreatSpaceSearch::Result ThreatSpaceSearch::recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions, bool isRoot)
	{
//		assert(Score::min_value() <= alpha);
//		assert(alpha < beta);
//		assert(beta <= Score::max_value());

		if (not isRoot)
			actions.clear();

		Move hash_move;
		{ // lookup to the shared hash table
//			TimerGuard tg(stats.hashtable);
			const SharedTableData tt_entry = shared_table.seek(hash_key);
			const Bound tt_bound = tt_entry.bound();

			stats.cache_calls++;
			if (tt_bound != Bound::NONE and tt_entry.move() != Move() and not is_move_legal(tt_entry.move()))
				stats.cache_colissions++;

			if (tt_bound != Bound::NONE and is_move_legal(tt_entry.move()))
			{ // there is some information stored in this entry
				stats.cache_hits++;

				hash_move = tt_entry.move();
				if (not isRoot)
				{ // we must not terminate search at root with just one best action on hash table hit, as the other modules require full list of actions
					const Score tt_score = tt_entry.score();
					if (tt_entry.depth() >= depthRemaining
							and ((tt_bound == Bound::LOWER and tt_score >= beta) or (tt_bound == Bound::UPPER and tt_score <= alpha)))
						return Result(tt_score, false); // to be 100% correct, TT hit shou
//					if (tt_entry.depth() >= depthRemaining
//							and ((tt_bound == Bound::EXACT) or (tt_bound == Bound::LOWER and tt_score >= beta)
//									or (tt_bound == Bound::UPPER and tt_score <= alpha)))
//						return Result(tt_score, true); // to be 100% correct, TT hit should return a score that is not final as it can be a collision.
				}
			}
		}

		if (actions.isEmpty() or not isRoot)
		{ // threat generator is combined with static solver
//			TimerGuard tg(stats.move_generation);
			const Score static_score = threat_generator.generate(actions, GeneratorMode::VCF);
			if (static_score.isProven())
				return Result(static_score, true);
		}

		{ // evaluation
//			TimerGuard tg(stats.evaluate);
			if (depthRemaining <= 0) // if it is a leaf, evaluate the position
				return Result(evaluate(), false);
		}

		const Score original_alpha = alpha;
		Result result(Score::min_value(), true);
		int visited_actions = 0;

		actions.moveCloserToFront(hash_move, 0);
		for (int i = 0; i < actions.size(); i++)
			if (not actions[i].is_final)
			{
				position_counter++;
				if (position_counter > max_positions)
					return Result(evaluate(), false);

				visited_actions++;
				const Move move = actions[i].move;

				shared_table.getHashFunction().updateHash(hash_key, move);
				shared_table.prefetch(hash_key);
				pattern_calculator.addMove(move);

				ActionList next_ply_actions(actions, actions[i]);
				const Result tmp = -recursive_solve(depthRemaining - 1, -beta, -alpha, next_ply_actions, false);

				pattern_calculator.undoMove(move);
				shared_table.getHashFunction().updateHash(hash_key, move);

				actions[i].score = tmp.score; // required for recovering of the search results
				actions[i].score.increaseDistance();
				actions[i].is_final = tmp.is_final; // required for recovering of the search results

				result.score = std::max(result.score, actions[i].score);
				result.is_final &= actions[i].is_final;

				alpha = std::max(alpha, actions[i].score);
				if (actions[i].score >= beta or (actions[i].score.isWin() and actions[i].is_final))
//				if (actions[i].score.isWin() and actions[i].is_final)
					break;
			}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but we don't have to defend
		// we need to override the score with something, for example with static evaluation
		if (visited_actions == 0 or (result.score.isLoss() and not actions.must_defend))
			result.score = evaluate();

		{ // insert new data to the hash table
//			TimerGuard tg(stats.hashtable);
			Bound bound = Bound::NONE;
			if (result.score <= original_alpha)
				bound = Bound::UPPER;
			else
			{
				if (result.score >= beta)
					bound = Bound::LOWER;
				else
					bound = Bound::EXACT;
			}
			SharedTableData entry(actions.must_defend, actions.has_initiative, bound, depthRemaining, result.score, actions.getBestMove());
			shared_table.insert(hash_key, entry);
		}

		return result;
	}
	ThreatSpaceSearch::Result ThreatSpaceSearch::recursive_solve_v2(int depthRemaining, Score alpha, Score beta, ActionList &actions, bool isRoot)
	{
		assert(Score::min_value() <= alpha);
		assert(alpha < beta);
		assert(beta <= Score::max_value());

		if (actions.isEmpty() or not isRoot)
		{ // at root this should be called only once
			const Score static_score = threat_generator.generate(actions, GeneratorMode::VCF);
			if (static_score.isProven())
				return Result(static_score, true);
		}

		if (depthRemaining <= 0) // if it is a leaf, evaluate the position
			return Result(evaluate(), false);

		Result result(Score::min_value(), true);
		int visited_actions = 0;

		const Move hash_move = local_table.seek(hash_key);
		stats.cache_calls++;
		stats.cache_colissions += (hash_move != Move() and not is_move_legal(hash_move));
		stats.cache_hits += (hash_move != Move());

		actions.moveCloserToFront(hash_move, 0);
		for (int i = 0; i < actions.size(); i++)
			if (not actions[i].is_final)
			{
				position_counter++;
				if (position_counter > max_positions)
					return Result(evaluate(), false);

				visited_actions++;
				const Move move = actions[i].move;

				local_table.getHashFunction().updateHash(hash_key, move);
				local_table.prefetch(hash_key);
				pattern_calculator.addMove(move);

				ActionList next_ply_actions(actions, actions[i]);
				const Result tmp = -recursive_solve_v2(depthRemaining - 1, -beta, -alpha, next_ply_actions, false);

				pattern_calculator.undoMove(move);
				local_table.getHashFunction().updateHash(hash_key, move);

				actions[i].score = tmp.score; // required for recovering of the search results
				actions[i].score.increaseDistance();
				actions[i].is_final = tmp.is_final; // required for recovering of the search results

				result.score = std::max(result.score, actions[i].score);
				result.is_final &= actions[i].is_final;

				alpha = std::max(alpha, actions[i].score);
				if (actions[i].score >= beta or (actions[i].score.isWin() and actions[i].is_final))
					break;
			}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but we don't have to defend
		// we need to override the score with something, for example with static evaluation
		if (visited_actions == 0 or (result.score.isLoss() and not actions.must_defend))
			result.score = evaluate();

		local_table.insert(hash_key, actions.getBestMove());

		return result;
	}
	bool ThreatSpaceSearch::is_move_legal(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}
	Score ThreatSpaceSearch::evaluate()
	{
		const Sign own_sign = pattern_calculator.getSignToMove();
		const Sign opponent_sign = invertSign(own_sign);
		const int worst_threat = static_cast<int>(ThreatType::OPEN_3);
		const int best_threat = static_cast<int>(ThreatType::FIVE);

		static const std::array<int, 10> own_values = { 0, 0, 19, 49, 76, 170, 33, 159, 252, 0 };
		static const std::array<int, 10> opp_values = { 0, 0, -1, -50, -45, -135, -14, -154, -496, 0 };
		int result = 12;
		for (int i = worst_threat; i <= best_threat; i++)
		{
			result += own_values[i] * pattern_calculator.getThreatHistogram(own_sign).get(static_cast<ThreatType>(i)).size();
			result += opp_values[i] * pattern_calculator.getThreatHistogram(opponent_sign).get(static_cast<ThreatType>(i)).size();
		}
		return std::max(-4000, std::min(4000, result));
	}

} /* namespace ag */
