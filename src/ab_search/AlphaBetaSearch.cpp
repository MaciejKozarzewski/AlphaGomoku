/*
 * AlphaBetaSearch.cpp
 *
 *  Created on: Oct 5, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/ab_search/AlphaBetaSearch.hpp>
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
				return Value(0.5f + score.getEval() / 2000.0f);
			case ProvenScore::WIN:
				return Value(1.0f, 0.0f, 0.0f);
		}
	}

	void fill(SearchTask &task, const ActionList &actions, Score score, MoveGeneratorMode mode)
	{
		if (score.isProven())
			assert(actions.size() > 0);
		task.setMustDefendFlag(actions.is_in_danger);
		for (auto iter = actions.begin(); iter < actions.end(); iter++)
//			task.addPriorEdge(iter->move, convertEvaluation(iter->score), convertProvenValue(iter->score));
			task.addPriorEdge(iter->move, Value(), convertProvenValue(iter->score));

		task.setProvenValue(convertProvenValue(score));
		if ((mode == MoveGeneratorMode::THREATS and score.isProven()) or mode != MoveGeneratorMode::THREATS)
		{
			task.setValue(convertEvaluation(score));
			if (score.isLoss() or score.isDraw() or score.isWin())
				task.markAsReadySolver();
		}
	}
}

namespace ag
{
	/*
	 * AlphaBetaSearchStats
	 */
	AlphaBetaSearchStats::AlphaBetaSearchStats() :
			setup("setup    "),
			static_solver("static   "),
			hashtable("hashtable"),
			move_generation("movegen  "),
			evaluate("evaluate "),
			solve("solve    ")
	{
	}
	std::string AlphaBetaSearchStats::toString() const
	{
		std::string result = "----AlphaBetaSearchStats----\n";
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
			result += "local cache hits  " + std::to_string(local_cache_hits) + " / " + std::to_string(local_cache_calls) + " ("
					+ std::to_string(100.0 * local_cache_hits / (1.0e-6 + local_cache_calls)) + "%)\n";
			result += "shared cache hits " + std::to_string(shared_cache_hits) + " / " + std::to_string(shared_cache_calls) + " ("
					+ std::to_string(100.0 * shared_cache_hits / (1.0e-6 + shared_cache_calls)) + "%)\n";
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
	 * AlphaBetaSearch
	 */
	AlphaBetaSearch::AlphaBetaSearch(GameConfig gameConfig, int maxPositions) :
			action_stack(get_max_nodes(gameConfig)),
			number_of_moves_for_draw(gameConfig.rows * gameConfig.cols),
			max_positions(maxPositions),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			static_solver(gameConfig, pattern_calculator),
			evaluator(gameConfig),
			local_table(1024),
			killers(gameConfig),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions),
			mode(MoveGeneratorMode::THREATS)
	{
		temporary_storage.reserve(256);
		shared_table = new SharedHashTable<4>(gameConfig.rows, gameConfig.cols, 1 << 24);
	}
	AlphaBetaSearch::~AlphaBetaSearch()
	{
//		std::cout << all_actions << " actions created" << '\n';
		delete shared_table;
	}
	int64_t AlphaBetaSearch::getMemory() const noexcept
	{
		return sizeof(this) + action_stack.size() * sizeof(Action);
	}
	void AlphaBetaSearch::setSharedTable(SharedHashTable<4> &table) noexcept
	{
//		shared_table = &table;
	}
	SharedHashTable<4>& AlphaBetaSearch::getSharedTable() noexcept
	{
		return *shared_table;
	}
	std::pair<Move, Score> AlphaBetaSearch::solve(SearchTask &task, int level)
	{
		if (shared_table == nullptr)
			throw std::logic_error("AlphaBetaSearch::solve() : Pointer to the shared hash table has not been initialized");

		stats.setup.startTimer();
		pattern_calculator.setBoard(task.getBoard(), task.getSignToMove());
		stats.setup.stopTimer();

		TimerGuard tg(stats.solve);

		position_counter = 0;

		ActionList actions = action_stack.getRoot();
		Score score;
		if (level == 0)
		{  // only check win or draw conditions using static solver
//			TimerGuard tg(stats.static_solver);
			score = static_solver.solve(actions, 0);
			if (score.isProven())
				fill(task, actions, score, mode);
		}
		if (level == 1)
		{  // try to statically solve position
//			TimerGuard tg(stats.static_solver);
			score = static_solver.solve(actions);
			if (score.isProven())
				fill(task, actions, score, mode);
		}
		if (level == 2)
		{  // try to statically solve position and generate moves
//			stats.static_solver.startTimer();
			score = static_solver.solve(actions);
//			stats.static_solver.stopTimer();

			if (not score.isProven())
			{
//				TimerGuard tg(stats.move_generation);
				MoveGenerator move_generator(game_config, pattern_calculator, actions, mode);
				move_generator.generateAll();
				for (int i = 0; i < actions.size(); i++)
					actions[i].score = Score(-1000); // set low value to be consistent with edge generation in init to loss mode
			}
			fill(task, actions, score, mode);
		}
		if (level >= 3)
		{
//			stats.evaluate.startTimer();
			evaluator.refresh(pattern_calculator); // set up NNUE state
//			stats.evaluate.stopTimer();

			hash_key = shared_table->getHashFunction().getHash(task.getBoard()); // set up hash key

			const int starting_depth = (mode == MoveGeneratorMode::THREATS) ? 1 : 0;
			const int max_depth = (level == 3) ? 1 : game_config.rows * game_config.cols - pattern_calculator.getCurrentDepth();
			const int step = (mode == MoveGeneratorMode::THREATS) ? 8 : 1;
			for (int depth = starting_depth; depth <= max_depth; depth += step)
			{
//				double t0 = getTime();
				score = recursive_solve(depth, Score::min_value(), Score::max_value(), actions);
//				score = threat_space_search(depth, Score::min_value(), Score::max_value(), actions);
//				std::cout << "depth " << depth << ", nodes " << position_counter << ", score " << score << ", time " << (getTime() - t0) << "s\n";
				if (score.isProven())
					break; // there is no point in further search if we have found some proven score
				if (position_counter >= max_positions)
					break; // the limit of nodes was exceeded
				if (mode == MoveGeneratorMode::THREATS and actions.size() == 0)
					break; // apparently there are no threats to make
			}
			fill(task, actions, score, mode);
		}
//		if (actions.size() == 0)
		if (score.isProven())
		{
//			std::cout << score.toString() << " at " << position_counter << '\n';
//			std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//			pattern_calculator.print();
//			pattern_calculator.printAllThreats();
//			actions.print();
//			exit(-1);
		}

		stats.total_positions += position_counter;
		stats.hits += static_cast<int>(score.isProven());

		const Move best_move = (actions.size() > 0) ? std::max_element(actions.begin(), actions.end())->move : Move();
		return std::pair<Move, Score>(best_move, score);
	}
	void AlphaBetaSearch::tune(float speed)
	{
//		Logger::write("AlphaBetaSearch::tune(" + std::to_string(speed) + ")");
//		Logger::write("Before new measurement");
//		Logger::write(lower_measurement.toString());
//		Logger::write(upper_measurement.toString());
		if (max_positions == lower_measurement.getParamValue())
		{
			lower_measurement.update(step_counter, speed);
			max_positions = upper_measurement.getParamValue();
//			Logger::write("using upper " + std::to_string(max_positions));
		}
		else
		{
			upper_measurement.update(step_counter, speed);
			max_positions = lower_measurement.getParamValue();
//			Logger::write("using lower " + std::to_string(max_positions));
		}

//		Logger::write("After new measurement");
//		Logger::write(lower_measurement.toString());
//		Logger::write(upper_measurement.toString());

		step_counter++;

		const std::pair<float, float> lower_mean_and_stddev = lower_measurement.predict(step_counter);
		const std::pair<float, float> upper_mean_and_stddev = upper_measurement.predict(step_counter);

//		Logger::write("Predicted");
//		Logger::write("Lower : " + std::to_string(lower_mean_and_stddev.first) + " +/-" + std::to_string(lower_mean_and_stddev.second));
//		Logger::write("Upper : " + std::to_string(upper_mean_and_stddev.first) + " +/-" + std::to_string(upper_mean_and_stddev.second));

		const float mean = lower_mean_and_stddev.first - upper_mean_and_stddev.first;
		const float stddev = std::hypot(lower_mean_and_stddev.second, upper_mean_and_stddev.second);
//		Logger::write("mean = " + std::to_string(mean) + ", stddev = " + std::to_string(stddev));

		const float probability = 1.0f - gaussian_cdf(mean / stddev);
//		Logger::write("probability = " + std::to_string(probability));

		if (probability > 0.95f) // there is more than 95% chance that higher value of 'max_positions' gives higher speed
		{
			if (lower_measurement.getParamValue() * tuning_step <= 6400)
			{
				const int new_max_pos = tuning_step * lower_measurement.getParamValue();
				lower_measurement = Measurement(new_max_pos);
				upper_measurement = Measurement(tuning_step * new_max_pos);
				max_positions = lower_measurement.getParamValue();
				Logger::write(
						"AlphaBetaSearch increasing positions to " + std::to_string(max_positions) + " at probability "
								+ std::to_string(probability));
			}
		}
		if (probability < 0.05f) // there is less than 5% chance that higher value of 'max_positions' gives higher speed
		{
			if (lower_measurement.getParamValue() / tuning_step >= 50)
			{
				const int new_max_pos = lower_measurement.getParamValue() / tuning_step;
				lower_measurement = Measurement(new_max_pos);
				upper_measurement = Measurement(tuning_step * new_max_pos);
				max_positions = lower_measurement.getParamValue();
				Logger::write(
						"AlphaBetaSearch decreasing positions to " + std::to_string(max_positions) + " at probability "
								+ std::to_string(probability));
			}
		}
	}
	AlphaBetaSearchStats AlphaBetaSearch::getStats() const
	{
		return stats;
	}
	void AlphaBetaSearch::clearStats()
	{
		stats = AlphaBetaSearchStats();
		max_positions = lower_measurement.getParamValue();
		lower_measurement.clear();
		upper_measurement.clear();
		step_counter = 0;
		Logger::write("AlphaBetaSearch is using " + std::to_string(max_positions) + " positions");
	}
	/*
	 * private
	 */
	Score AlphaBetaSearch::recursive_solve(int depthRemaining, Score alpha, Score beta, ActionList &actions)
	{
		assert(depthRemaining >= 0);
		actions.clear();
		{ // at first try to statically solve the position
//			TimerGuard tg(stats.static_solver);
			const Score static_solution = static_solver.solve(actions);
			if (static_solution.isProven())
				return static_solution;
			assert(actions.size() == 0);
		}

		{ // lookup to the shared hash table
//			TimerGuard tg(stats.hashtable);
			const SharedTableData tt_entry = shared_table->seek(hash_key);
			const ScoreType tt_score_type = tt_entry.getScoreType();

			stats.shared_cache_calls++;
			stats.shared_cache_hits += (tt_score_type != ScoreType::NONE);

			const Move hash_move = get_hash_move(hash_key);
			const bool is_hash_move_valid = (hash_move.sign == Sign::NONE) ? true : is_move_valid(hash_move); // if sign is NONE it means that there is no hash move to check, assume true

			if (tt_score_type != ScoreType::NONE and is_hash_move_valid)
			{ // the hashtable entry was calculated with more depth than we currently have -> we can use it now
				const Score tt_score = tt_entry.getScore();
				actions.is_in_danger = tt_entry.isInDanger();
				actions.is_threatening = tt_entry.isThreatening();

				if (tt_entry.getDepth() >= depthRemaining
						and ((tt_score_type == ScoreType::EXACT) or (tt_score_type == ScoreType::LOWER_BOUND and tt_score >= beta)
								or (tt_score_type == ScoreType::UPPER_BOUND and tt_score <= alpha)))
				{
					actions.add(get_hash_move(hash_key)); // we must add at least one action, otherwise the search may end without producing any moves
					actions[0].score = tt_score; // we must assign a score to this action
					return tt_score;
				}
			}
		}

//		stats.evaluate.startTimer();
		evaluator.update(pattern_calculator); // update the accumulator of NNUE
//		stats.evaluate.stopTimer();

		if (depthRemaining == 0)
		{ // if it is a leaf, evaluate the position
//			TimerGuard tg(stats.evaluate);
			return Score((int) (2000 * evaluator.forward()) - 1000);
//			return evaluate();
		}

		// if not leaf, continue with move generation
//		stats.move_generation.startTimer();
		MoveGenerator move_generator(game_config, pattern_calculator, actions, mode);
		move_generator.setExternalStorage(temporary_storage);
		move_generator.set(get_hash_move(hash_key), killers.get(pattern_calculator.getCurrentDepth()));
		move_generator.generate();
//		stats.move_generation.stopTimer();

		const Score original_alpha = alpha;
		const double original_position_count = position_counter;

		Score score;
		if (mode == MoveGeneratorMode::THREATS and (actions.size() == 0 or not actions.is_in_danger))
			score = Score::not_loss(); // if we don't have to defend then this position cannot be proved as loss because there are some other moves that we don't check
		else
		{
			score = Score::min_value();
			assert(actions.size() > 0);
		}
		for (int i = 0; i < actions.size(); i++)
		{
			position_counter++;
			if (position_counter > max_positions)
				depthRemaining = 1; // a hack to make each node look like a leaf, so the search will complete correctly but will exit as soon as possible

			const Move move = actions[i].move;
			Score action_score;

			shared_table->getHashFunction().updateHash(hash_key, move);
			shared_table->prefetch(hash_key); // TODO check if this improves anything
			{
				ActionList next_ply_actions(actions);
				pattern_calculator.addMove(move);
				/* below is Principal Variation Search */
//				if (i == 0) // first action
//					action_score = -recursive_solve(depthRemaining - 1, -beta, -alpha, next_ply_actions);
//				else
//				{
//					action_score = -recursive_solve(depthRemaining - 1, -alpha - 1, -alpha, next_ply_actions); // search with a null window
//					if (alpha < action_score and action_score < beta)
//						action_score = -recursive_solve(depthRemaining - 1, -beta, -alpha, next_ply_actions); // if it failed high, do a full re-search
//				}
				/* below is regular alpha-beta search */
				action_score = -recursive_solve(depthRemaining - 1, -beta, -alpha, next_ply_actions);
				pattern_calculator.undoMove(move);
			}
			shared_table->getHashFunction().updateHash(hash_key, move);

//			stats.evaluate.startTimer();
			evaluator.update(pattern_calculator); // updating in reverse direction (undoing moves) does not cost anything but is required to correctly set current depth
//			stats.evaluate.stopTimer();

			action_score.increaseDistanceToWinOrLoss();
			actions[i].score = action_score; // required for recovering the search results
			score = std::max(score, action_score);
			alpha = std::max(alpha, score);
			if (alpha >= beta or action_score.isWin())
			{
				killers.insert(actions[i].move, pattern_calculator.getCurrentDepth());
				break;
			}

			if (i == (actions.size() - 1))
			{ // we have reached the last action
//				TimerGuard tg(stats.move_generation);
				move_generator.generate(); // try generating more actions
			}
		}
		if (actions.size() == 0)
			score = Score::not_win();

		{ // insert new data to the hash table
//			TimerGuard tg(stats.hashtable);
			const int nodes = position_counter - original_position_count;
			const Move best_move = std::max_element(actions.begin(), actions.end())->move;
			ScoreType score_type = ScoreType::NONE;
			if (score <= original_alpha)
				score_type = ScoreType::UPPER_BOUND;
			else
			{
				if (score >= beta)
					score_type = ScoreType::LOWER_BOUND;
				else
					score_type = ScoreType::EXACT;
			}
			shared_table->insert(hash_key,
					SharedTableData(nodes, best_move, score, score_type, depthRemaining, actions.is_in_danger, actions.is_threatening));
		}

		return score;
	}
	Score AlphaBetaSearch::threat_space_search(int depthRemaining, Score alpha, Score beta, ActionList &actions)
	{
		assert(mode == MoveGeneratorMode::THREATS);
		assert(depthRemaining >= 0);
		actions.clear();
		{ // at first try to statically solve the position
//			TimerGuard tg(stats.static_solver);
			const Score static_solution = static_solver.solve(actions);
			if (static_solution.isProven())
				return static_solution;
			assert(actions.size() == 0);
		}

		{ // lookup to the shared hash table
//			TimerGuard tg(stats.hashtable);
			const SharedTableData tt_entry = shared_table->seek(hash_key);
			const ScoreType tt_score_type = tt_entry.getScoreType();

			stats.shared_cache_calls++;
			stats.shared_cache_hits += (tt_score_type != ScoreType::NONE);

			const Move hash_move = get_hash_move(hash_key);
			const bool is_hash_move_valid = (hash_move.sign == Sign::NONE) ? true : is_move_valid(hash_move); // if sign is NONE it means that there is no hash move to check, assume true

			if (tt_score_type != ScoreType::NONE and is_hash_move_valid)
			{ // the hashtable entry was calculated with more depth than we currently have -> we can use it now
				const Score tt_score = tt_entry.getScore();
				actions.is_in_danger = tt_entry.isInDanger();
				actions.is_threatening = tt_entry.isThreatening();

				if (tt_entry.getDepth() >= depthRemaining
						and ((tt_score_type == ScoreType::EXACT) or (tt_score_type == ScoreType::LOWER_BOUND and tt_score >= beta)
								or (tt_score_type == ScoreType::UPPER_BOUND and tt_score <= alpha)))
				{
					actions.add(get_hash_move(hash_key)); // we must add at least one action, otherwise the search may end without producing any moves
					actions[0].score = tt_score; // we must assign a score to this action
					return tt_score;
				}
			}
		}

//		stats.evaluate.startTimer();
		evaluator.update(pattern_calculator); // update the accumulator of NNUE
//		stats.evaluate.stopTimer();

		if (depthRemaining == 0)
		{ // if it is a leaf, evaluate the position
//			TimerGuard tg(stats.evaluate);
			return Score((int) (2000 * evaluator.forward()) - 1000);
//			return evaluate();
		}

		// if not leaf, continue with move generation
//		stats.move_generation.startTimer();
		MoveGenerator move_generator(game_config, pattern_calculator, actions, mode);
		move_generator.setExternalStorage(temporary_storage);
		move_generator.set(get_hash_move(hash_key), killers.get(pattern_calculator.getCurrentDepth()));
//		move_generator.set(Move(), killers.get(pattern_calculator.getCurrentDepth()));
//		move_generator.set(get_hash_move(hash_key), ShortVector<Move, 4>());
		move_generator.generate();
//		stats.move_generation.stopTimer();

		const Score original_alpha = alpha;
		const double original_position_count = position_counter;

		// if we don't have to defend then this position cannot be proved as loss because there are some other moves that we don't check
		Score score = actions.is_in_danger ? Score::min_value() : Score::not_loss();
		for (int i = 0; i < actions.size(); i++)
		{
			position_counter++;
			if (position_counter > max_positions)
				depthRemaining = 1; // a hack to make each node look like a leaf, so the search will complete correctly but will exit as soon as possible

			const Move move = actions[i].move;
			Score action_score;

			shared_table->getHashFunction().updateHash(hash_key, move);
			shared_table->prefetch(hash_key); // TODO check if this improves anything
			{
				ActionList next_ply_actions(actions);
				pattern_calculator.addMove(move);
				action_score = -threat_space_search(depthRemaining - 1, -beta, -alpha, next_ply_actions);
				pattern_calculator.undoMove(move);
			}
			shared_table->getHashFunction().updateHash(hash_key, move);

//			stats.evaluate.startTimer();
			evaluator.update(pattern_calculator); // updating in reverse direction (undoing moves) does not cost anything but is required to correctly set current depth
//			stats.evaluate.stopTimer();

			action_score.increaseDistanceToWinOrLoss();
			actions[i].score = action_score; // required for recovering the search results
			score = std::max(score, action_score);
			alpha = std::max(alpha, score);
			if (alpha >= beta or action_score.isWin())
			{
				killers.insert(actions[i].move, pattern_calculator.getCurrentDepth());
				break;
			}

			if (i == (actions.size() - 1))
			{ // we have reached the last action
//				TimerGuard tg(stats.move_generation);
				move_generator.generate(); // try generating more actions
			}
		}
		if (actions.size() == 0)
			score = Score::not_win();

//		if (actions.size() > 0)
		{ // insert new data to the hash table
//			TimerGuard tg(stats.hashtable);
			const int nodes = position_counter - original_position_count;
			const Move best_move = std::max_element(actions.begin(), actions.end())->move;
			ScoreType score_type = ScoreType::NONE;
			if (score <= original_alpha)
				score_type = ScoreType::UPPER_BOUND;
			else
			{
				if (score >= beta)
					score_type = ScoreType::LOWER_BOUND;
				else
					score_type = ScoreType::EXACT;
			}
			shared_table->insert(hash_key,
					SharedTableData(nodes, best_move, score, score_type, depthRemaining, actions.is_in_danger, actions.is_threatening));
		}

		return score;
	}
	Score AlphaBetaSearch::evaluate()
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
		return std::max(-1000, std::min(1000, result));
	}
	Move AlphaBetaSearch::get_hash_move(HashKey<64> key) noexcept
	{
//		TimerGuard tg(stats.hashtable);
		const SharedTableData tt_entry = shared_table->seek(key);
		return (tt_entry.getScoreType() != ScoreType::NONE) ? tt_entry.getMove() : Move();
	}
	bool AlphaBetaSearch::is_move_valid(Move m) const noexcept
	{
		return m.sign == pattern_calculator.getSignToMove() and pattern_calculator.signAt(m.row, m.col) == Sign::NONE;
	}

} /* namespace ag */
