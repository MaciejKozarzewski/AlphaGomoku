/*
 * VCFSolver_v3.cpp
 *
 *  Created on: Apr 13, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/VCFSolver_v3.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <iostream>
#include <cassert>

namespace
{
	using namespace ag;

	template<typename T>
	T gaussian_cdf(T x) noexcept
	{
		return static_cast<T>(0.5) * (static_cast<T>(1.0) + std::erf(x / std::sqrt(2.0)));
	}
	int get_max_nodes(ag::GameConfig cfg) noexcept
	{
		const int size = cfg.rows * cfg.cols;
		return size * (size - 1) / 2;
	}
}

namespace ag
{

	VCFSolver_v3::VCFSolver_v3(GameConfig gameConfig, int maxPositions) :
			number_of_moves_for_draw(gameConfig.rows * gameConfig.cols),
			max_positions(maxPositions),
			nodes_buffer(get_max_nodes(gameConfig)),
			game_config(gameConfig),
			feature_extractor(gameConfig),
			hashtable(gameConfig.rows * gameConfig.cols, 4096),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions)
	{
	}
	void VCFSolver_v3::solve(SearchTask &task, int level)
	{
		stats.setup.startTimer();
		feature_extractor.setBoard(task.getBoard(), task.getSignToMove());
		stats.setup.stopTimer();

		{ // artificial scope for timer guard
			TimerGuard tg(stats.static_solve);
			static_solve(task, level);
			if (task.isReady())
			{
				stats.static_hits++;
				return;
			}
		}

		if (level < 2)
			return;

		int available_attacks = 0;
		for (int i = static_cast<int>(ThreatType_v3::HALF_OPEN_4); i <= static_cast<int>(ThreatType_v3::FIVE); i++)
			available_attacks += get_attacker_threats(static_cast<ThreatType_v3>(i)).size();
		if (available_attacks == 0)
			return;

		TimerGuard tg(stats.recursive_solve);
		hashtable.setBoard(task.getBoard());
		hashtable.clear();

		const Sign sign_to_move = feature_extractor.getSignToMove();

		node_counter = 1; // prepare node stack
		InternalNode &root_node = nodes_buffer.front();
		root_node.init(0, 0, invertSign(sign_to_move), SolvedValue::UNKNOWN); // prepare node stack

		position_counter = 0;
		recursive_solve(nodes_buffer.front(), true);
		stats.total_positions += position_counter;
		if (root_node.solved_value == SolvedValue::LOSS) // solver found a win
		{
			stats.recursive_hits++;
			task.getProvenEdges().clear(); // delete any edges that could have been added in 'static_solve_block_4()'
			for (auto iter = root_node.begin(); iter < root_node.end(); iter++)
				if (iter->solved_value == SolvedValue::WIN)
					task.addProvenEdge(Move(sign_to_move, iter->move), ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.markAsReady();
		}
	}
	void VCFSolver_v3::tune(float speed)
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
						"VCFSolver increasing positions to " + std::to_string(max_positions) + " at probability " + std::to_string(probability));
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
						"VCFSolver decreasing positions to " + std::to_string(max_positions) + " at probability " + std::to_string(probability));
			}
		}
	}
	SolverStats VCFSolver_v3::getStats() const
	{
		return stats;
	}
	void VCFSolver_v3::clearStats()
	{
		stats = SolverStats();
		max_positions = lower_measurement.getParamValue();
		lower_measurement.clear();
		upper_measurement.clear();
		step_counter = 0;
		Logger::write("VCFSolver is using " + std::to_string(max_positions) + " positions");
	}
	void VCFSolver_v3::print_stats() const
	{
		feature_extractor.print_stats();
		std::cout << stats.toString() << '\n';
	}
	/*
	 * private
	 */
	void VCFSolver_v3::add_move(Move move) noexcept
	{
		feature_extractor.addMove(move);
		hashtable.updateHash(move);
	}
	void VCFSolver_v3::undo_move(Move move) noexcept
	{
		feature_extractor.undoMove(move);
		hashtable.updateHash(move);
	}
	void VCFSolver_v3::static_solve(SearchTask &task, int level)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();

		const std::vector<Move> &attacker_five = get_attacker_threats(ThreatType_v3::FIVE);
		if (attacker_five.size() > 0) // if attacker can make a five, it is a win in 1 ply
		{
			task.addProvenEdges(attacker_five, sign_to_move, ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.markAsReady();
			return;
		}
		if (feature_extractor.getRootDepth() + 1 >= number_of_moves_for_draw) // detect draw
		{
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
					if (feature_extractor.signAt(row, col) == Sign::NONE)
					{
						task.addProvenEdge(Move(sign_to_move, row, col), ProvenValue::DRAW);
						task.setValue(Value(0.0f, 1.0, 0.0f));
					}
			task.markAsReady();
			return;
		}
		if (level < 1)
			return;
		const std::vector<Move> &defender_five = get_defender_threats(ThreatType_v3::FIVE);
		switch (defender_five.size())
		{
			case 0: // the defender has no fives
			{
				const std::vector<Move> &attacker_open_four = get_attacker_threats(ThreatType_v3::OPEN_4);
				const std::vector<Move> &attacker_fork_4x4 = get_attacker_threats(ThreatType_v3::FORK_4x4);

				if (attacker_open_four.size() > 0 or attacker_fork_4x4.size() > 0) // if attacker can make an open four or 4x4 fork, it is a win in 3 plys
				{
					task.addProvenEdges(attacker_open_four, sign_to_move, ProvenValue::WIN);
					task.addProvenEdges(attacker_fork_4x4, sign_to_move, ProvenValue::WIN);
					task.setValue(Value(1.0f, 0.0, 0.0f));
					task.markAsReady();
					return;
				}

				const std::vector<Move> &attacker_fork_4x3 = get_attacker_threats(ThreatType_v3::FORK_4x3);

				const std::vector<Move> &defender_half_open_4 = get_defender_threats(ThreatType_v3::HALF_OPEN_4);
				const std::vector<Move> &defender_open_4 = get_defender_threats(ThreatType_v3::OPEN_4);
				const std::vector<Move> &defender_fork_4x4 = get_defender_threats(ThreatType_v3::FORK_4x4);
				if (defender_half_open_4.empty() and defender_open_4.empty() and defender_fork_4x4.empty()) // defender has no five and no four of any kind
				{
					const std::vector<Move> &attacker_fork_3x3 = get_attacker_threats(ThreatType_v3::FORK_3x3);
					if (attacker_fork_4x3.size() > 0 or attacker_fork_3x3.size() > 0) // if attacker can make 4x3 or 3x3 fork, it is a win in 5 plys
					{
						task.addProvenEdges(attacker_fork_4x3, sign_to_move, ProvenValue::WIN);
						task.addProvenEdges(attacker_fork_3x3, sign_to_move, ProvenValue::WIN);
						task.setValue(Value(1.0f, 0.0, 0.0f));
						task.markAsReady();
						return;
					}
				}
				else // defender can make a four of any kind
				{
					task.addProvenEdges(defender_open_4, sign_to_move, ProvenValue::UNKNOWN);
					task.addProvenEdges(defender_fork_4x4, sign_to_move, ProvenValue::UNKNOWN);
					task.addProvenEdges(defender_half_open_4, sign_to_move, ProvenValue::UNKNOWN);

					const std::vector<Move> &attacker_half_open_4 = get_attacker_threats(ThreatType_v3::HALF_OPEN_4);
					for (auto iter = attacker_half_open_4.begin(); iter < attacker_half_open_4.end(); iter++)
					{
						Move move_to_be_added(sign_to_move, *iter);
						bool is_already_added = std::any_of(task.getProvenEdges().begin(), task.getProvenEdges().end(),
								[move_to_be_added](const Edge &edge)
								{	return edge.getMove() == move_to_be_added;}); // find if such move has been added in any of two loops above
						if (not is_already_added) // move must not be added twice
							task.addProvenEdge(move_to_be_added, ProvenValue::UNKNOWN);
					}
				}
				break;
			}
			case 1: // defender can make one five
			{
				task.addProvenEdge(Move(sign_to_move, defender_five.front()), ProvenValue::UNKNOWN);
				break;
			}
			default: // defender can make multiple fives, it is a loss in 3 plys
			{
				task.addProvenEdges(defender_five, sign_to_move, ProvenValue::LOSS);
				task.setValue(Value(0.0f, 0.0, 1.0f));
				task.markAsReady(); // the state is provably losing, there is no need to further evaluate it
				break;
			}
		}
	}
	void VCFSolver_v3::prepare_moves_for_attacker(InternalNode &node)
	{
		const std::vector<Move> &attacker_five = get_attacker_threats(ThreatType_v3::FIVE);
		if (attacker_five.size() > 0) // if attacker can make a five, it is a win in 1 ply
		{
			node.solved_value = SolvedValue::LOSS;
			return;
		}
		const std::vector<Move> &defender_five = get_defender_threats(ThreatType_v3::FIVE);
		switch (defender_five.size())
		{
			case 0: // the defender has no fives
			{
				const std::vector<Move> &attacker_open_four = get_attacker_threats(ThreatType_v3::OPEN_4);
				const std::vector<Move> &attacker_fork_4x4 = get_attacker_threats(ThreatType_v3::FORK_4x4);

				if (attacker_open_four.size() > 0 or attacker_fork_4x4.size() > 0) // if attacker can make an open four or 4x4 fork, it is a win in 3 plys
				{
					node.solved_value = SolvedValue::LOSS;
					return;
				}

				const std::vector<Move> &attacker_fork_4x3 = get_attacker_threats(ThreatType_v3::FORK_4x3);
				const std::vector<Move> &attacker_fork_3x3 = get_attacker_threats(ThreatType_v3::FORK_3x3);

				const std::vector<Move> &defender_half_open_4 = get_defender_threats(ThreatType_v3::HALF_OPEN_4);
				const std::vector<Move> &defender_open_4 = get_defender_threats(ThreatType_v3::OPEN_4);
				const std::vector<Move> &defender_fork_4x4 = get_defender_threats(ThreatType_v3::FORK_4x4);
				if (attacker_fork_4x3.size() > 0 or attacker_fork_3x3.size() > 0)
				{
					if (defender_half_open_4.empty() and defender_open_4.empty() and defender_fork_4x4.empty()) // defender cannot make a four of any kind
					{
						node.solved_value = SolvedValue::LOSS; // if attacker can make 4x3 or 3x3 fork, it is a win in 5 plys
						return;
					}
					else // defender can make a four of some kind
					{
						add_children_nodes(node, attacker_fork_4x3, SolvedValue::UNKNOWN, true); // check if a 4x3 fork can be converted to a win (depends on defensive moves)
					}
				}
				const std::vector<Move> &attacker_half_open_4 = get_attacker_threats(ThreatType_v3::HALF_OPEN_4);
//				const std::vector<Move> &attacker_open_3 = get_attacker_threats(ThreatType_v3::OPEN_3);
				add_children_nodes(node, attacker_half_open_4, SolvedValue::UNKNOWN, true);
				add_children_nodes(node, attacker_fork_3x3, SolvedValue::UNKNOWN, true);
//				add_children_nodes(node, attacker_open_3, SolvedValue::UNKNOWN, true);

				if (node.number_of_children == 0)
					node.solved_value = SolvedValue::UNSOLVED;
				else
					node.solved_value = SolvedValue::UNKNOWN;
				break;
			}
			case 1: // defender can make one five
			{
				add_children_nodes(node, defender_five, SolvedValue::UNKNOWN);
				node.solved_value = SolvedValue::UNKNOWN;
				break;
			}
			default: // defender can make multiple fives, it is a loss in 3 plys
			{
				node.solved_value = SolvedValue::WIN;
				break;
			}
		}
	}
	void VCFSolver_v3::prepare_moves_for_defender(InternalNode &node)
	{
		const std::vector<Move> &attacker_five = get_attacker_threats(ThreatType_v3::FIVE);
		switch (attacker_five.size())
		{
			case 0:
			{
				const std::vector<Move> &attacker_open_4 = get_attacker_threats(ThreatType_v3::OPEN_4);

				const std::vector<Move> &defender_half_open_4 = get_defender_threats(ThreatType_v3::HALF_OPEN_4);
				const std::vector<Move> &defender_open_4 = get_defender_threats(ThreatType_v3::OPEN_4);
				const std::vector<Move> &defender_fork_4x4 = get_defender_threats(ThreatType_v3::FORK_4x4);
				const std::vector<Move> &defender_five = get_defender_threats(ThreatType_v3::FIVE);

				if (defender_half_open_4.empty() and defender_open_4.empty() and defender_fork_4x4.empty() and defender_five.empty()
						and attacker_open_4.size() > 0)
				{ // defender cannot make neither five nor a four of any kind while the attacker can make an open four
					add_children_nodes(node, attacker_open_4, SolvedValue::UNKNOWN, true);
					node.solved_value = SolvedValue::UNKNOWN;
				}
				else
					node.solved_value = SolvedValue::UNSOLVED; // attacker lost initiative
				break;
			}
			case 1: // single forced move
			{
				add_children_nodes(node, attacker_five, SolvedValue::UNKNOWN);
				node.solved_value = SolvedValue::UNKNOWN;
				break;
			}
			default: // attacker can make multiple fives, mark this node as win
			{
				node.solved_value = SolvedValue::WIN;
				break;
			}
		}
	}
	void VCFSolver_v3::recursive_solve(InternalNode &node, bool isAttackingSide)
	{
//		feature_extractor.print();
//		feature_extractor.printAllThreats();

		node.children = nodes_buffer.data() + node_counter;

		if (isAttackingSide)
			prepare_moves_for_attacker(node);
		else
			prepare_moves_for_defender(node);

		if (node.solved_value == SolvedValue::UNKNOWN)
		{
			assert(node.number_of_children > 0);
			int num_losing_children = 0;
			bool has_win_child = false;
			for (auto iter = node.begin(); iter < node.end(); iter++)
			{
//				if (iter != node.children)
//				{
//					feature_extractor.print();
//					feature_extractor.printAllThreats();
//				}
//				std::cout << "positions checked = " << position_counter << std::endl;
//				std::cout << "state of all children:" << std::endl;
//				for (int i = 0; i < node.number_of_children; i++)
//					std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << " "
//							<< (int) node.children[i].prior_value << std::endl;
//				std::cout << "---now checking " << iter->move.toString() << std::endl;

				hashtable.updateHash(iter->move);
				const SolvedValue solved_value_from_table = hashtable.get(hashtable.getHash());
				if (solved_value_from_table == SolvedValue::UNKNOWN) // not found
				{
					position_counter += 1.0;
					if (position_counter < max_positions)
					{
						feature_extractor.addMove(iter->move);
						recursive_solve(*iter, not isAttackingSide);
						feature_extractor.undoMove(iter->move);
					}
				}
				else
				{
//					position_counter += 0.0625; // the position counter is increased by 1/16 to avoid infinite search over cached states
					iter->solved_value = solved_value_from_table; // cache hit
				}
				hashtable.updateHash(iter->move); // revert back to original hash

				if (iter->solved_value == SolvedValue::WIN) // found winning move, can skip remaining nodes
				{
					has_win_child = true;
					break;
				}
				else
				{
					if (iter->solved_value == SolvedValue::LOSS) // child node is losing
						num_losing_children++;
					else // UNKNOWN, DRAW or UNSOLVED
					{
						if (not isAttackingSide) // if this is defensive side, it must prove all its child nodes as losing
							break; // so the function can exit now, as there will be at least one unproven node (current one)
					}
				}
			}

//			std::cout << "state of all children:\n";
//			for (int i = 0; i < node.number_of_children; i++)
//				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << std::endl;

			if (has_win_child)
				node.solved_value = SolvedValue::LOSS;
			else
			{
				if (num_losing_children == node.number_of_children)
					node.solved_value = SolvedValue::WIN;
				else
					node.solved_value = SolvedValue::UNSOLVED;
			}
		}
		hashtable.insert(hashtable.getHash(), node.solved_value);
		delete_children_nodes(node);
//		if (node.solved_value == SolvedValue::LOSS) // node represents a move on the winning path
//			feature_extractor.updateValueStatistics(node.move);
	}

	const std::vector<Move>& VCFSolver_v3::get_attacker_threats(ThreatType_v3 tt) const noexcept
	{
		return feature_extractor.getThreatHistogram(feature_extractor.getSignToMove()).get(tt);
	}
	const std::vector<Move>& VCFSolver_v3::get_defender_threats(ThreatType_v3 tt) const noexcept
	{
		return feature_extractor.getThreatHistogram(invertSign(feature_extractor.getSignToMove())).get(tt);
	}
	void VCFSolver_v3::add_children_nodes(InternalNode &node, const std::vector<Move> &moves, SolvedValue sv, bool sort) noexcept
	{
		assert(node_counter + moves.size() <= nodes_buffer.size());
		node_counter += moves.size();

		const Sign sign_to_move = invertSign(node.move.sign);
		for (size_t i = 0; i < moves.size(); i++)
			node.children[node.number_of_children + i].init(moves[i].row, moves[i].col, sign_to_move, sv);

		if (sort)
		{
			// TODO add sorting newly added nodes -> sort(node.end(), node.end() + moves.size())
//			for (size_t i = 0; i < moves.size(); i++)
//				node.children[node.number_of_children + i].prior_value = feature_extractor.getValue(node.children[node.number_of_children + i].move);
//			std::sort(node.end(), node.end() + moves.size(), [](const InternalNode &lhs, const InternalNode &rhs)
//			{	return lhs.prior_value > rhs.prior_value;});
		}
		node.number_of_children += moves.size();
	}
	void VCFSolver_v3::delete_children_nodes(InternalNode &node) noexcept
	{
		assert(node_counter >= static_cast<size_t>(node.number_of_children));
		node_counter -= node.number_of_children;
	}

} /* namespace ag */

