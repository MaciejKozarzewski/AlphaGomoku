/*
 * VCTSolver.cpp
 *
 *  Created on: Apr 21, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/solver/VCTSolver.hpp>

#include <iostream>
#include <bitset>
#include <cassert>

namespace
{
	using namespace ag;
	using namespace ag::experimental;

	constexpr int get_row_step(Direction dir) noexcept
	{
		return (dir == HORIZONTAL) ? 0 : 1;
	}
	constexpr int get_col_step(Direction dir) noexcept
	{
		switch (dir)
		{
			case HORIZONTAL:
				return 1;
			case VERTICAL:
				return 0;
			case DIAGONAL:
				return 1;
			case ANTIDIAGONAL:
				return -1;
			default:
				return 0;
		}
	}
	template<typename T>
	bool contains(const std::vector<T> &v, T element)
	{
		return std::find(v.begin(), v.end(), element) != v.end();
	}

	SolvedValue invertSolvedValue(SolvedValue sv) noexcept
	{
		switch (sv)
		{
			default:
				return sv;
			case SolvedValue::LOSS:
				return SolvedValue::WIN;
			case SolvedValue::WIN:
				return SolvedValue::LOSS;
		}
	}
	ProvenValue convert(SolvedValue sv) noexcept
	{
		switch (sv)
		{
			default:
				return ProvenValue::UNKNOWN;
			case SolvedValue::LOSS:
				return ProvenValue::LOSS;
			case SolvedValue::DRAW:
				return ProvenValue::DRAW;
			case SolvedValue::WIN:
				return ProvenValue::WIN;
		}
	}

	int get_max_nodes(ag::GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size - 1) / 2;
	}
	Direction find_direction_of(PatternType pattern, std::array<PatternType, 4> patternGroup)
	{
		for (Direction dir = 0; dir < 4; dir++)
			if (patternGroup[dir] == pattern)
				return dir;
		assert(false); // the search pattern must exist in the group
		return 0;
	}

	class SubSolverFork4x3
	{
			const PatternCalculator &calc;

			Direction find_direction_of(PatternType pattern, Move move) const
			{
				const std::array<PatternType, 4> own_patterns = calc.getPatternTypeAt(move.sign, move.row, move.col);
				for (Direction dir = 0; dir < 4; dir++)
					if (own_patterns[dir] == pattern)
						return dir;
				assert(false); // actually this should never happen as there is always a half open four in 4x3 fork
				return 0;
			}
			Move find_response_to(Move move, Direction dir) const
			{
				assert(move.sign != Sign::NONE);
				assert(calc.getPatternTypeAt(move.sign, move.row, move.col, dir) == PatternType::HALF_OPEN_4);

				const Sign defender_sign = invertSign(move.sign);
				const BitMask<uint16_t> defensive_moves = calc.getDefensiveMoves(defender_sign, move.row, move.col, dir);
				for (int i = -calc.getPadding(); i <= calc.getPadding(); i++)
					if (i != 0 and defensive_moves.get(i + calc.getPadding())) // defensive move will never be outside board
						return Move(move.row + i * get_row_step(dir), move.col + i * get_col_step(dir), defender_sign);
				assert(false); // actually this should never happen as there always is a defensive move to half open four
				return Move();
			}
		public:
			SubSolverFork4x3(const PatternCalculator &calc) :
					calc(calc)
			{
			}
			SolvedValue trySolve(Move move) const
			{
				assert(move.sign != Sign::NONE);
				const Direction direction_of_own_four = find_direction_of(PatternType::HALF_OPEN_4, move);
				const Move opponent_response = find_response_to(move, direction_of_own_four);
				const ThreatType opponent_threat = calc.getThreatAt(opponent_response.sign, opponent_response.row, opponent_response.col);
				switch (opponent_threat)
				{
					default:
					case ThreatType::NONE:
					case ThreatType::HALF_OPEN_3:
					case ThreatType::OPEN_3:
					case ThreatType::FORK_3x3:
						return SolvedValue::WIN; // opponent cannot make any four in response to our threat
					case ThreatType::HALF_OPEN_4:
					{
						const Direction direction_of_opp_four = find_direction_of(PatternType::HALF_OPEN_4, opponent_response);
						const Move our_response = find_response_to(opponent_response, direction_of_opp_four);
						const ThreatType our_threat = calc.getThreatAt(our_response.sign, our_response.row, our_response.col);
						if (our_threat == ThreatType::OPEN_3) // check if our response to the opponent four creates an open four for us (3-plys earlier it will be just open three)
						{
							const Direction direction_of_our_three = find_direction_of(PatternType::OPEN_3, our_response);
							const int dr = get_row_step(direction_of_our_three);
							const int dc = get_col_step(direction_of_our_three);
							if ((move.row - dr == our_response.row and move.col - dc == our_response.col)
									or (move.row + dr == our_response.row and move.col + dc == our_response.col))
							{
								return SolvedValue::WIN;
							}
						}
						return SolvedValue::UNCHECKED;
					}
					case ThreatType::FORK_4x3:
						return SolvedValue::UNCHECKED; // checking this statically would be too complicated
					case ThreatType::FORK_4x4:
					case ThreatType::OPEN_4:
					case ThreatType::FIVE:
						return SolvedValue::LOSS; // opponent can make either 4x4 fork, open four or a five
				}
			}
	};
	class SubSolverForbiddenMoves
	{
			PatternCalculator &calc;

			template<int Pad>
			bool is_straight_four(int row, int col, Direction direction) const
			{
				assert(calc.signAt(row, col) == Sign::NONE);
				uint32_t result = static_cast<uint32_t>(Sign::CROSS) << (2 * Pad);
				uint32_t shift = 0;
				switch (direction)
				{
					case HORIZONTAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (static_cast<uint32_t>(calc.signAt(row, col + i)) << shift);
						break;
					case VERTICAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (static_cast<uint32_t>(calc.signAt(row + i, col)) << shift);
						break;
					case DIAGONAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (static_cast<uint32_t>(calc.signAt(row + i, col + i)) << shift);
						break;
					case ANTIDIAGONAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (static_cast<uint32_t>(calc.signAt(row + i, col - i)) << shift);
						break;
				}
				for (int i = 0; i < (2 * Pad + 1 - 4); i++, result /= 4)
					if ((result & 255u) == 85u)
						return true;
				return false;
			}
		public:
			SubSolverForbiddenMoves(PatternCalculator &calc) :
					calc(calc)
			{
			}
			bool isForbidden(int row, int col) const
			{
				constexpr int Pad = 5;

				if (calc.signAt(row, col) != Sign::NONE)
					return false; // moves on occupied spots are not considered forbidden (they are simply illegal)

				const Threat threat = calc.getThreatAt(Sign::CROSS, row, col);
				if (threat.forCross() == ThreatType::OVERLINE)
					return true;
				if (threat.forCross() == ThreatType::FORK_4x4)
					return true;
				if (threat.forCross() == ThreatType::FORK_3x3)
				{
					int open3_count = 0;
					for (Direction dir = 0; dir < 4; dir++)
						if (calc.getPatternTypeAt(Sign::CROSS, row, col, dir) == PatternType::OPEN_3)
						{
							const BitMask<uint16_t> defensive_moves = calc.getDefensiveMoves(Sign::CIRCLE, row, col, dir);
							for (int i = -Pad; i <= Pad; i++)
							{
								const int x = row + i * get_row_step(dir);
								const int y = col + i * get_col_step(dir);
								if (i != 0 and defensive_moves.get(Pad + i) == true and is_straight_four<Pad>(x, y, dir))
								{ // minor optimization as 'is_straight_four' works without adding new move to the pattern calculator
									calc.addMove(Move(row, col, Sign::CROSS));
									const bool is_forbidden = isForbidden(x, y);
									calc.undoMove(Move(row, col, Sign::CROSS));

									if (not is_forbidden)
									{
										open3_count++;
										break;
									}
								}
							}

						}
					if (open3_count >= 2)
						return true;
				}
				return false;
			}
	};
}

namespace ag::experimental
{
	SolverStats::SolverStats() :
			setup("setup "),
			solve("solve ")
	{
	}
	std::string SolverStats::toString() const
	{
		std::string result = "----SolverStats----\n";
		result += setup.toString() + '\n';
		result += solve.toString() + '\n';
		if (setup.getTotalCount() > 0)
		{
			if (solve.getTotalCount() > 0)
				result += "total positions = " + std::to_string(total_positions) + " : " + std::to_string(total_positions / solve.getTotalCount())
						+ '\n';
			result += "solved    " + std::to_string(hits) + " / " + std::to_string(setup.getTotalCount()) + " ("
					+ std::to_string(100.0 * hits / setup.getTotalCount()) + "%)\n";
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
	 * VCTSolver
	 */
	VCTSolver::VCTSolver(GameConfig gameConfig, int maxPositions) :
			node_stack(get_max_nodes(gameConfig)),
			number_of_moves_for_draw(gameConfig.rows * gameConfig.cols),
			max_positions(maxPositions),
			game_config(gameConfig),
			pattern_calculator(gameConfig),
			hashtable(gameConfig.rows * gameConfig.cols, 4096),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions)
	{
	}
	void VCTSolver::solve(SearchTask &task, int level)
	{
		stats.setup.startTimer();
		pattern_calculator.setBoard(task.getBoard());
		depth = Board::numberOfMoves(task.getBoard());
		attacking_sign = task.getSignToMove();
		stats.setup.stopTimer();

//		std::cout << "sign to move = " << toString(task.getSignToMove()) << '\n';
//		pattern_calculator.print();
//		pattern_calculator.printAllThreats();

		TimerGuard tg(stats.solve);
		const Move last_move = (task.getLastEdge() == nullptr) ? Move(0, 0, invertSign(task.getSignToMove())) : task.getLastEdge()->getMove();
		InternalNode &root_node = node_stack.front();
		root_node.init(last_move.row, last_move.col, last_move.sign); // prepare node stack
		position_counter = 0;
		node_counter = 1;

		switch (level)
		{
			case 0: // only check win or draw conditions
			{
				root_node.children = node_stack.data() + node_counter; // setup pointer to child nodes
				if (try_win_in_1(root_node))
					break;
				if (try_draw_in_1(root_node))
					break;
				break;
			}
			case 1: // statically solve position
			{
				generate_moves(root_node);
				break;
			}
			case 2: // perform recursive search
			default:
			{
				hashtable.clear();
				hashtable.setBoard(task.getBoard());
				recursive_solve(root_node);
				break;
			}
		}

		calculate_solved_value(root_node);
//		root_node.print();

		if (root_node.must_defend)
		{
			for (auto iter = root_node.begin(); iter < root_node.end(); iter++)
				task.addNonLosingEdge(iter->move);
		}
		else
		{
			for (auto iter = root_node.begin(); iter < root_node.end(); iter++)
				if (iter->hasSolution())
					task.addProvenEdge(iter->move, convert(iter->solved_value));
		}
		switch (root_node.solved_value)
		{
			default:
				break;
			case SolvedValue::LOSS:
				task.setValue(Value(1.0f, 0.0f, 0.0f));
				task.setProvenValue(ProvenValue::WIN);
				task.markAsReady();
				break;
			case SolvedValue::DRAW:
				task.setValue(Value(0.0f, 1.0f, 0.0f));
				task.setProvenValue(ProvenValue::DRAW);
				task.markAsReady();
				break;
			case SolvedValue::WIN:
				if (root_node.must_defend)
				{
					task.setValue(Value(0.0f, 0.0f, 1.0f));
					task.setProvenValue(ProvenValue::LOSS);
					task.markAsReady();
				}
				break;
		}

		stats.hits += static_cast<int>(task.isReady());
	}
	void VCTSolver::tune(float speed)
	{
//		Logger::write("VCFSolver::tune(" + std::to_string(speed) + ")");
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

		std::pair<float, float> lower_mean_and_stddev = lower_measurement.predict(step_counter);
		std::pair<float, float> upper_mean_and_stddev = upper_measurement.predict(step_counter);

//		Logger::write("Predicted");
//		Logger::write("Lower : " + std::to_string(lower_mean_and_stddev.first) + " +/-" + std::to_string(lower_mean_and_stddev.second));
//		Logger::write("Upper : " + std::to_string(upper_mean_and_stddev.first) + " +/-" + std::to_string(upper_mean_and_stddev.second));

		float mean = lower_mean_and_stddev.first - upper_mean_and_stddev.first;
		float stddev = std::hypot(lower_mean_and_stddev.second, upper_mean_and_stddev.second);
//		Logger::write("mean = " + std::to_string(mean) + ", stddev = " + std::to_string(stddev));

		float probability = 1.0f - gaussian_cdf(mean / stddev);
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
						"VCTSolver increasing positions to " + std::to_string(max_positions) + " at probability " + std::to_string(probability));
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
						"VCTSolver decreasing positions to " + std::to_string(max_positions) + " at probability " + std::to_string(probability));
			}
		}
	}
	SolverStats VCTSolver::getStats() const
	{
		return stats;
	}
	void VCTSolver::clearStats()
	{
		stats = SolverStats();
		max_positions = lower_measurement.getParamValue();
		lower_measurement.clear();
		upper_measurement.clear();
		step_counter = 0;
		Logger::write("VCTSolver is using " + std::to_string(max_positions) + " positions");
	}
	/*
	 * private
	 */
	void VCTSolver::get_forbidden_moves(SearchTask &task)
	{
//		if (game_config.rules == GameRules::RENJU and task.getSignToMove() == Sign::CROSS)
//		{
//			const ThreatHistogram &threats = pattern_calculator.getThreatHistogram(Sign::CROSS);
//
//			for (auto iter = threats.get(ThreatType::OVERLINE).begin(); iter < threats.get(ThreatType::OVERLINE).end(); iter++)
//				task.getForbiddenEdges().push_back(Edge(*iter));
//			for (auto iter = threats.get(ThreatType::FORK_4x4).begin(); iter < threats.get(ThreatType::FORK_4x4).end(); iter++)
//				task.getForbiddenEdges().push_back(Edge(*iter));
//
//			SubSolverForbiddenMoves sub_solver(pattern_calculator);
//			for (auto iter = threats.get(ThreatType::FORK_3x3).begin(); iter < threats.get(ThreatType::FORK_3x3).end(); iter++)
//				if (sub_solver.isForbidden(iter->row, iter->col))
//					task.getForbiddenEdges().push_back(Edge(*iter));
//		}
	}
	bool VCTSolver::try_win_in_1(InternalNode &node)
	{
		const std::vector<Move> &own_five = get_own_threats(node, ThreatType::FIVE);
		if (own_five.size() > 0) // if attacker can make a five, it is a win in 1 ply
		{
			add_children_nodes(node, own_five, SolvedValue::WIN);
			return true;
		}
		return false;
	}
	bool VCTSolver::try_draw_in_1(InternalNode &node)
	{
		if (depth + 1 >= number_of_moves_for_draw) // detect draw
		{
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
					if (pattern_calculator.signAt(row, col) == Sign::NONE)
						add_child_node(node, Move(row, col), SolvedValue::DRAW);
			return true;
		}
		return false;
	}
	bool VCTSolver::try_defend_opponent_five(InternalNode &node)
	{
		const std::vector<Move> &opponent_five = get_opp_threats(node, ThreatType::FIVE);
		switch (opponent_five.size())
		{
			case 0:
				return false; // continue to check other threats
			case 1:
			{
				const Move five_move = opponent_five[0];
				// check if the defensive move created a win for us
				if (contains(get_own_threats(node, ThreatType::OPEN_4), five_move)
						or contains(get_own_threats(node, ThreatType::FORK_4x4), five_move))
				{
					add_child_node(node, five_move, SolvedValue::WIN);
					return true;
				}
				if (contains(get_own_threats(node, ThreatType::FORK_4x3), five_move))
				{
					SubSolverFork4x3 sub_solver(pattern_calculator);
					SolvedValue sv = sub_solver.trySolve(Move(node.getSignToMove(), five_move));
					add_child_node(node, five_move, sv);
					return true;
				}
				if (contains(get_own_threats(node, ThreatType::FORK_3x3), five_move))
				{
					if (pattern_calculator.getThreatHistogram(invertSign(node.getSignToMove())).hasAnyFour() == false) // opponent has no fours
					{
						add_child_node(node, five_move, SolvedValue::WIN);
						return true;
					}
				}
				add_child_node(node, five_move);

//				if (node.getSignToMove() == attacking_sign)
//				{
//					if (contains(get_own_threats(node, ThreatType::HALF_OPEN_4), five_move))
//						add_child_node(node, five_move);
//				}
//				else
//				{
//					add_child_node(node, five_move);
//				}
				node.must_defend = true;
				return true;
			}
			default: // multiple fives
			{
				node.must_defend = true;
				add_children_nodes(node, opponent_five, SolvedValue::LOSS);
				return true;
			}
		}
	}
	bool VCTSolver::try_win_in_3(InternalNode &node)
	{
		const std::vector<Move> &own_open_four = get_own_threats(node, ThreatType::OPEN_4);
		const std::vector<Move> &own_fork_4x4 = get_own_threats(node, ThreatType::FORK_4x4);
		if (own_open_four.size() > 0 or own_fork_4x4.size() > 0)
		{
			add_children_nodes(node, own_open_four, SolvedValue::WIN);
			add_children_nodes(node, own_fork_4x4, SolvedValue::WIN);
			return true;
		}
		return false;
	}
	bool VCTSolver::try_solve_forks_if_opponent_has_no_fours(InternalNode &node)
	{
		if (pattern_calculator.getThreatHistogram(invertSign(node.getSignToMove())).hasAnyFour() == false) // opponent has no fours
		{
			const std::vector<Move> &own_fork_4x3 = get_own_threats(node, ThreatType::FORK_4x3);
			const std::vector<Move> &own_fork_3x3 = get_own_threats(node, ThreatType::FORK_3x3);
			if (own_fork_4x3.size() > 0 or own_fork_3x3.size() > 0)
			{
				add_children_nodes(node, own_fork_4x3, SolvedValue::WIN);
				add_children_nodes(node, own_fork_3x3, SolvedValue::WIN);
				return true;
			}
		}
		return false;
	}
	bool VCTSolver::try_solve_4x3_forks(InternalNode &node)
	{
		SubSolverFork4x3 sub_solver(pattern_calculator);
		const std::vector<Move> &own_fork_4x3 = get_own_threats(node, ThreatType::FORK_4x3);
		const Sign sign_to_move = node.getSignToMove();
		bool any_4x3_fork_was_solved = false;

		for (auto move = own_fork_4x3.begin(); move < own_fork_4x3.end(); move++)
		{
			SolvedValue sv = sub_solver.trySolve(Move(sign_to_move, *move));
			any_4x3_fork_was_solved |= (sv == SolvedValue::WIN);
			add_child_node(node, *move, sv);
		}
		return any_4x3_fork_was_solved;
	}
	bool VCTSolver::try_defend_opponent_fours(InternalNode &node)
	{
		const Sign own_sign = node.getSignToMove();
		const Sign opp_sign = invertSign(own_sign);

		const std::vector<Move> opp_open_four = get_opp_threats(node, ThreatType::OPEN_4);
		for (auto move = opp_open_four.begin(); move < opp_open_four.end(); move++)
		{
			const std::array<PatternType, 4> patterns = pattern_calculator.getPatternTypeAt(opp_sign, move->row, move->col);
			const Direction direction = find_direction_of(PatternType::OPEN_4, patterns);
			add_defensive_moves(node, *move, direction);
		}

		const std::vector<Move> opp_fork_4x4 = get_opp_threats(node, ThreatType::FORK_4x4);
		for (auto move = opp_fork_4x4.begin(); move < opp_fork_4x4.end(); move++)
		{
			const std::array<PatternType, 4> patterns = pattern_calculator.getPatternTypeAt(opp_sign, move->row, move->col);
			for (Direction direction = 0; direction < 4; direction++)
				if (patterns[direction] == PatternType::OPEN_4 or patterns[direction] == PatternType::HALF_OPEN_4)
					add_defensive_moves(node, *move, direction);
		}

		const std::vector<Move> opp_fork_4x3 = get_opp_threats(node, ThreatType::FORK_4x3);
		for (auto move = opp_fork_4x3.begin(); move < opp_fork_4x3.end(); move++)
		{
			const std::array<PatternType, 4> patterns = pattern_calculator.getPatternTypeAt(opp_sign, move->row, move->col);
			for (Direction direction = 0; direction < 4; direction++)
				if (patterns[direction] == PatternType::OPEN_3 or patterns[direction] == PatternType::HALF_OPEN_4)
					add_defensive_moves(node, *move, direction);
		}

		if (opp_open_four.size() > 0 or opp_fork_4x4.size() > 0 or opp_fork_4x3.size() > 0)
		{
			node.must_defend = true;
			return true;
		}

		const std::vector<Move> opp_half_open_4 = get_opp_threats(node, ThreatType::HALF_OPEN_4);
//		for (auto move = opp_half_open_4.begin(); move < opp_half_open_4.end(); move++)
//		{
//			const std::array<PatternType, 4> patterns = pattern_calculator.getPatternTypeAt(opp_sign, move->row, move->col);
//			const Direction direction = find_direction_of(PatternType::HALF_OPEN_4, patterns);
//			add_defensive_moves(node, *move, direction);
//		}
		return true;
	}
	void VCTSolver::generate_moves(InternalNode &node)
	{
		node.children = node_stack.data() + node_counter; // setup pointer to child nodes

		if (try_win_in_1(node))
			return;
		if (try_draw_in_1(node))
			return;
		if (try_defend_opponent_five(node))
			return;
		if (try_win_in_3(node))
			return;
		if (try_solve_forks_if_opponent_has_no_fours(node))
			return;
		if (try_solve_4x3_forks(node))
			return;

		add_children_nodes(node, get_own_threats(node, ThreatType::HALF_OPEN_4), SolvedValue::UNCHECKED, true);
//		if (node.getSignToMove() != attacking_sign)
		try_defend_opponent_fours(node);
//		add_children_nodes(node, get_own_threats(node, ThreatType::OPEN_3), SolvedValue::UNCHECKED, true);

		if (game_config.rules == GameRules::RENJU and node.getSignToMove() == Sign::CROSS)
		{
			SubSolverForbiddenMoves sub_solver(pattern_calculator);
			for (auto iter = node.begin(); iter < node.end(); iter++)
				if (sub_solver.isForbidden(iter->move.row, iter->move.col))
					iter->solved_value = SolvedValue::FORBIDDEN;
		}
	}

	void VCTSolver::recursive_solve(InternalNode &node)
	{
//		pattern_calculator.print(node.move);
//		pattern_calculator.printAllThreats();

		generate_moves(node);
		calculate_solved_value(node);
//		node.print();

		if (not node.hasSolution())
		{
			for (auto iter = node.begin(); iter < node.end(); iter++)
			{
//				std::cout << "positions checked = " << position_counter << "/" << max_positions << std::endl;
//				std::cout << "state of all children:" << std::endl;
//				for (int i = 0; i < node.number_of_children; i++)
//					std::cout << i << " : " << node.children[i].move.toString() << " : " << node.children[i].move.text() << " = "
//							<< toString(node.children[i].solved_value) << " " << (int) node.children[i].prior_value << std::endl;
				if (not iter->hasSolution()) // some nodes will already have some value assigned
				{
//					std::cout << "---now checking " << iter->move.toString() << std::endl;
					hashtable.updateHash(iter->move);
					const SolvedValue solved_value_from_table = hashtable.get(hashtable.getHash());
					if (solved_value_from_table == SolvedValue::UNCHECKED) // not found
					{
						if (position_counter < max_positions)
						{
							pattern_calculator.addMove(iter->move);
							depth++;

							position_counter += 1.0;
							recursive_solve(*iter);

							depth--;
							pattern_calculator.undoMove(iter->move);
						}
						else
							iter->solved_value = SolvedValue::NO_SOLUTION;
					}
					else
					{
//						std::cout << "cache hit " << toString(solved_value_from_table) << '\n';
//						position_counter += 0.0625; // the position counter is increased by 1/16 to avoid infinite search over cached states
						iter->solved_value = solved_value_from_table; // cache hit
					}
					hashtable.updateHash(iter->move); // revert back to original hash

					if (iter->solved_value == SolvedValue::WIN) // found winning move, can skip remaining nodes
						break;
				}
			}
			calculate_solved_value(node);
		}

//		std::cout << "state of all children:\n";
//		for (int i = 0; i < node.number_of_children; i++)
//			std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << std::endl;

		delete_children_nodes(node);
		hashtable.insert(hashtable.getHash(), node.solved_value);
	}
	const std::vector<Move>& VCTSolver::get_own_threats(const InternalNode &node, ThreatType tt) const noexcept
	{
		return pattern_calculator.getThreatHistogram(node.getSignToMove()).get(tt);
	}
	const std::vector<Move>& VCTSolver::get_opp_threats(const InternalNode &node, ThreatType tt) const noexcept
	{
		return pattern_calculator.getThreatHistogram(invertSign(node.getSignToMove())).get(tt);
	}
	void VCTSolver::add_defensive_moves(InternalNode &node, const Move move, Direction dir)
	{
		const Sign own_sign = node.getSignToMove();
		const BitMask<uint16_t> defensive_moves = pattern_calculator.getDefensiveMoves(own_sign, move.row, move.col, dir);
		for (int i = -pattern_calculator.getPadding(); i <= pattern_calculator.getPadding(); i++)
			if (defensive_moves.get(i + pattern_calculator.getPadding())) // defensive move will never be outside board
			{
				Move m(move.row + i * get_row_step(dir), move.col + i * get_col_step(dir), own_sign);
				add_child_node(node, m, SolvedValue::UNCHECKED, true);
			}
	}
	void VCTSolver::add_child_node(InternalNode &node, Move move, SolvedValue sv, bool excludeDuplicates) noexcept
	{
		assert(node_counter + 1 <= node_stack.size());
		if (excludeDuplicates and node.containsChild(move))
			return;
		node_counter += 1;
		node.end()->init(move.row, move.col, node.getSignToMove(), sv);
		node.number_of_children += 1;
	}
	void VCTSolver::add_children_nodes(InternalNode &node, const std::vector<Move> &moves, SolvedValue sv, bool excludeDuplicates) noexcept
	{
		assert(node_counter + moves.size() <= node_stack.size());
		const Sign sign_to_move = node.getSignToMove();
		if (excludeDuplicates)
		{
			for (size_t i = 0; i < moves.size(); i++)
				if (not node.containsChild(moves[i]))
				{
					node_counter++;
					node.children[node.number_of_children].init(moves[i].row, moves[i].col, sign_to_move, sv);
					node.number_of_children++;
				}
		}
		else
		{
			for (size_t i = 0; i < moves.size(); i++)
				node.children[node.number_of_children + i].init(moves[i].row, moves[i].col, sign_to_move, sv);
			node_counter += moves.size();
			node.number_of_children += moves.size();
		}
	}
	void VCTSolver::delete_children_nodes(InternalNode &node) noexcept
	{
		assert(node_counter >= static_cast<size_t>(node.number_of_children));
		node_counter -= node.number_of_children;
	}
	void VCTSolver::calculate_solved_value(InternalNode &node) noexcept
	{
		if (node.number_of_children == 0)
		{
			node.solved_value = SolvedValue::NO_SOLUTION;
			return;
		}
		int num_losing_children = 0;
		bool has_draw_child = false, has_unchecked_child = false;
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
			num_losing_children += static_cast<int>(iter->solved_value == SolvedValue::LOSS);
			has_draw_child |= (iter->solved_value == SolvedValue::DRAW);
			has_unchecked_child |= (iter->solved_value == SolvedValue::UNCHECKED);
			if (iter->solved_value == SolvedValue::WIN) // found winning move, can skip remaining nodes
			{
				node.solved_value = SolvedValue::LOSS;
				return;
			}
		}
		if (num_losing_children == node.number_of_children and node.must_defend)
			node.solved_value = SolvedValue::WIN;
		else
		{
			if (has_unchecked_child)
				node.solved_value = SolvedValue::UNCHECKED;
			else
			{
				if (has_draw_child)
					node.solved_value = SolvedValue::DRAW;
				else
					node.solved_value = SolvedValue::NO_SOLUTION;
			}
		}
	}

} /* namespace ag */

