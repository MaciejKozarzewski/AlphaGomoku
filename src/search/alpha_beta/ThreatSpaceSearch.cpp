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

#include <minml/graph/Graph.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <iostream>
#include <cassert>

namespace
{
	using namespace ag;
	int get_max_nodes(GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size - 1) / 2;
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
				return score.convertToValue();
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
//			inference_nnue(gameConfig),
			shared_table(gameConfig.rows, gameConfig.cols, tssConfig.hash_table_size),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions)
	{
		nnue::TrainingNNUE tmp(gameConfig, 1);
		tmp.load("nnue_64x16x1.bin");
//		nnue.load("nnue_256x32x32x1.bin");
		inference_nnue = nnue::InferenceNNUE(gameConfig, tmp.dump());
	}
	int64_t ThreatSpaceSearch::getMemory() const noexcept
	{
		return action_stack.size() * sizeof(Action);
	}
	void ThreatSpaceSearch::clear()
	{
		shared_table.clear();
	}
	void ThreatSpaceSearch::increaseGeneration()
	{
		shared_table.increaseGeneration();
	}
	void ThreatSpaceSearch::solve(SearchTask &task, TssMode mode, int maxPositions)
	{
		{
			TimerGuard tg(stats.setup);
			pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());
			task.getFeatures().encode(pattern_calculator);
			action_stack.resize(maxPositions * game_config.rows * game_config.cols);
			inference_nnue.refresh(pattern_calculator);
		}

		TimerGuard tg(stats.solve);
		max_positions = maxPositions;
		search_mode = mode;
		position_counter = 0;

		ActionList actions = action_stack.create_root();
		Score result;
		switch (mode)
		{
			default:
			case TssMode::BASIC:
			{
//				TimerGuard tg(stats.move_generation);
				result = threat_generator.generate(actions, GeneratorMode::BASIC);
				break;
			}
			case TssMode::STATIC:
			{
//				TimerGuard tg(stats.move_generation);
				result = threat_generator.generate(actions, GeneratorMode::STATIC);
				break;
			}
			case TssMode::RECURSIVE:
			{
				hash_key = shared_table.getHashFunction().getHash(task.getBoard()); // set up hash key
				const int max_depth = std::max(1, std::min(255, game_config.draw_after - pattern_calculator.getCurrentDepth()));
				for (int depth = 0; depth <= max_depth; depth += 4)
				{ // iterative deepening loop
//					double t0 = getTime();
					result = recursive_solve(depth, Score::min_value(), Score::max_value(), actions);
//					std::cout << "max depth=" << depth << ", nodes=" << position_counter << ", score=" << result << ", time=" << (getTime() - t0)
//							<< "s\n";
//					actions.print();
//					std::cout << '\n';
					if (actions.isEmpty() or result.isProven() or position_counter >= max_positions)
						break;
				}
				break;
			}
		}

		for (auto iter = actions.begin(); iter < actions.end(); iter++)
		{
			const Move m(iter->move);
			task.getActionScores().at(m.row, m.col) = iter->score;
			if (iter->score.isProven())
				task.getActionValues().at(m.row, m.col) = convertScoreToValue(iter->score);
			if (actions.must_defend)
				task.addDefensiveMove(m);
		}
		task.setScore(result);
		if (result.isProven())
			task.setValue(convertScoreToValue(result));
		task.markAsProcessedBySolver();

//		if (result.score.getEval() ==  Score::min_value())
//		if (task.getScore().getEval() == Score::min_value())
//		{
//		std::cout << result.toString() << " at " << position_counter << '\n';
//		std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//		pattern_calculator.print();
//		pattern_calculator.printAllThreats();
//		pattern_calculator.printForbiddenMoves();
//		std::sort(actions.begin(), actions.end());
//		actions.print();
//		std::cout << task.toString();
//		std::cout << "\n---------------------------------------------\n";
//		exit(-1);
//		}

		stats.total_positions += position_counter;
		stats.hits += static_cast<int>(result.isProven());
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
	void ThreatSpaceSearch::print_stats() const
	{
		pattern_calculator.print_stats();
		std::cout << stats.toString() << '\n';
		std::cout << "SharedHashTable load factor = " << shared_table.loadFactor(true) << '\n';
		inference_nnue.print_stats();
	}
	/*
	 * private
	 */
	class ScopedCounter
	{
			std::string name;
			size_t counter = 0;
		public:
			ScopedCounter(const std::string &name) :
					name(name)
			{
			}
			~ScopedCounter()
			{
				std::cout << "ScopedCounter \"" << name << "\' = " << counter << '\n';
			}
			void increment() noexcept
			{
				counter++;
			}
			void increment(size_t inc) noexcept
			{
				counter += inc;
			}
	};
	Score ThreatSpaceSearch::recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions)
	{
		assert(depthRemaining >= 0);
		assert(Score::min_value() <= alpha);
		assert(alpha < beta);
		assert(beta <= Score::max_value());

		Move hash_move;
		{ // lookup to the shared hash table
			TimerGuard tg(stats.hashtable);
			const SharedTableData tt_entry = shared_table.seek(hash_key);
			const Bound tt_bound = tt_entry.bound();

			stats.cache_calls++;
			if (tt_bound != Bound::NONE and tt_entry.move() != Move() and not is_move_legal(tt_entry.move()))
				stats.cache_colissions++;

			if (tt_bound != Bound::NONE and is_move_legal(tt_entry.move()))
			{ // there is some information stored in this entry
				stats.cache_hits++;

				hash_move = tt_entry.move();
				if (not actions.isRoot())
				{ // we must not terminate search at root with just one best action on hash table hit, as the other modules may require full list of actions
					const Score tt_score = tt_entry.score();
					if (tt_score.isProven())
						return tt_score;
					if (tt_entry.depth() >= depthRemaining
							and ((tt_bound == Bound::EXACT) or (tt_bound == Bound::LOWER and tt_score >= beta)
									or (tt_bound == Bound::UPPER and tt_score <= alpha)))
						return tt_score;
				}
			}
		}

		if (actions.isEmpty())
		{ // threat generator is combined with static solver
			TimerGuard tg(stats.move_generation);
			const GeneratorMode mode = actions.isRoot() ? GeneratorMode::REDUCED : GeneratorMode::VCF;
			const Score static_score = threat_generator.generate(actions, mode);
			action_stack.increment(actions.size());
			if (static_score.isProven())
				return static_score;
		}

		{ // evaluation
			if (depthRemaining == 0) // if it is a leaf, evaluate the position
				return evaluate();
		}

		const Score original_alpha = alpha;
		Score result = Score::min_value();

		// assign artificially high score to the hash move so it will be checked first
		for (int i = 0; i < actions.size(); i++)
			if (actions[i].move == hash_move)
			{
				actions[i].score = Score::max_eval();
				break;
			}
		for (int i = 0; i < actions.size(); i++)
		{
			// perform move sorting
			int idx = i;
			for (int j = i; j < actions.size(); j++)
				if (actions[idx] < actions[j])
					idx = j;
			std::swap(actions[i], actions[idx]);

			if (position_counter < max_positions and not actions[i].score.isProven())
			{
				position_counter++;

				const Move move = actions[i].move;

				shared_table.getHashFunction().updateHash(hash_key, move);
				shared_table.prefetch(hash_key);
				pattern_calculator.addMove(move);
				inference_nnue.update(pattern_calculator);

				ActionList next_ply_actions = action_stack.create_from_action(actions[i]);
				Score tmp = -recursive_solve(depthRemaining - 1, -beta, -alpha, next_ply_actions);
				tmp.increaseDistance();

				pattern_calculator.undoMove(move);
				inference_nnue.update(pattern_calculator);

				shared_table.getHashFunction().updateHash(hash_key, move);
				actions[i].score = tmp; // required for recovering of the search results
			}

			result = std::max(result, actions[i].score);

			alpha = std::max(alpha, actions[i].score);
			if (actions[i].score >= beta or actions[i].score.isWin())
				break;
		}
		// if either
		//  - no actions were generated, or
		//  - all actions are losing but the state is not fully expanded (we cannot prove a loss)
		// we need to override the score with something, for example with static evaluation
		if (actions.size() == 0 or (result.isLoss() and not actions.is_fully_expanded))
			result = evaluate();

		{ // insert new data to the hash table
			TimerGuard tg(stats.hashtable);
			Bound bound = Bound::NONE;
			if (result <= original_alpha)
				bound = Bound::UPPER;
			else
			{
				if (result >= beta)
					bound = Bound::LOWER;
				else
					bound = Bound::EXACT;
			}
			SharedTableData entry(bound, depthRemaining, result, actions.getBestMove());
			shared_table.insert(hash_key, entry);
		}

		return result;
	}
	bool ThreatSpaceSearch::is_move_legal(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

	Score ThreatSpaceSearch::evaluate()
	{
		return Score(static_cast<int>(2000 * inference_nnue.forward() - 1000));

//		TimerGuard tg(stats.evaluate);
//		const Sign own_sign = pattern_calculator.getSignToMove();
//		const Sign opponent_sign = invertSign(own_sign);
//		const int worst_threat = static_cast<int>(ThreatType::OPEN_3);
//		const int best_threat = static_cast<int>(ThreatType::FIVE);
//
//		static const std::array<int, 10> own_values = { 0, 0, 19, 49, 76, 170, 33, 159, 252, 0 };
//		static const std::array<int, 10> opp_values = { 0, 0, -1, -50, -45, -135, -14, -154, -496, 0 };
//		int result = 12;
//		for (int i = worst_threat; i <= best_threat; i++)
//		{
//			result += own_values[i] * pattern_calculator.getThreatHistogram(own_sign).numberOf(static_cast<ThreatType>(i));
//			result += opp_values[i] * pattern_calculator.getThreatHistogram(opponent_sign).numberOf(static_cast<ThreatType>(i));
//		}
//		return std::max(-4000, std::min(4000, result));
	}

} /* namespace ag */
