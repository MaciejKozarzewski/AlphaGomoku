/*
 * VCFSolver.cpp
 *
 *  Created on: Mar 4, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/VCFSolver.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>
#include <alphagomoku/utils/LinearRegression.hpp>
#include <alphagomoku/utils/Logger.hpp>

#include <iostream>
#include <cassert>

namespace
{
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

	SolverStats::SolverStats() :
			setup("setup    "),
			static_solve("static   "),
			recursive_solve("recursive")
	{
	}
	std::string SolverStats::toString() const
	{
		std::string result = "----SolverStats----\n";
		result += setup.toString() + '\n';
		result += static_solve.toString() + '\n';
		result += recursive_solve.toString() + '\n';
		return result;
	}

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

	VCFSolver::VCFSolver(GameConfig gameConfig, int maxPositions) :
			max_positions(maxPositions),
			nodes_buffer(get_max_nodes(gameConfig)),
			game_config(gameConfig),
			feature_extractor(gameConfig),
			hashtable(gameConfig.rows * gameConfig.cols, 4096),
			lower_measurement(max_positions),
			upper_measurement(tuning_step * max_positions)
	{
	}
	void VCFSolver::solve(SearchTask &task, int level)
	{
		stats.setup.startTimer();
		feature_extractor.setBoard(task.getBoard(), task.getSignToMove());
		stats.setup.stopTimer();

		{ // artificial scope for timer guard
			TimerGuard tg(stats.static_solve);
			bool success = static_solve_1ply_win(task);
			if (success)
				return;
			success = static_solve_1ply_draw(task);
			if (success)
				return;

			if (level <= 0)
				return; // only winning or draw moves will be found

			success = static_solve_block_5(task);
			if (success)
				return;
			success = static_solve_3ply_win(task);
			if (success)
				return;
			static_solve_block_4(task);
		}

		if (level <= 1)
			return; // winning and forced moves will be found

		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &own_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, sign_to_move);
		const std::vector<Move> &own_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, sign_to_move);
		if (own_open_four.size() + own_half_open_four.size() == 0)
			return; // there are no threats that we can make

		TimerGuard timer(stats.recursive_solve);

		hashtable.setBoard(task.getBoard());
		hashtable.clear();

		position_counter = 0;
		node_counter = 1; // prepare node stack
		nodes_buffer.front().init(0, 0, invertSign(sign_to_move)); // prepare node stack

//		recursive_solve(nodes_buffer.front(), false);
		recursive_solve_2(nodes_buffer.front(), true);

//		std::cout << "depth = " << root_depth << ", result = " << static_cast<int>(nodes_buffer.front().solved_value) << ", checked "
//				<< position_counter << " positions, total = " << total_positions << "\n";
		if (nodes_buffer.front().solved_value == SolvedValue::LOSS)
		{
			for (auto iter = nodes_buffer.front().begin(); iter < nodes_buffer.front().end(); iter++)
				if (iter->solved_value == SolvedValue::WIN)
					task.addProvenEdge(Move(sign_to_move, iter->move), ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.setReady();
		}
	}
	void VCFSolver::tune(float speed)
	{
//		Logger::write("VCFSolver::tune(" + std::to_string(speed) + ")");
//		Logger::write("Before new measurement");
//		Logger::write(lower_measurement.toString());
//		Logger::write(upper_measurement.toString());
		if (max_positions == lower_measurement.getParamValue())
		{
			lower_measurement.update(step_counter, speed);
			max_positions = upper_measurement.getParamValue();
		}
		else
		{
			upper_measurement.update(step_counter, speed);
			max_positions = lower_measurement.getParamValue();
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

		if (probability > 0.95f) // there is 95% chance that higher value of 'max_positions' gives higher speed
		{
			if (lower_measurement.getParamValue() * tuning_step <= 6400)
			{
//				Logger::write("VCFSolver::tune() increasing positions");
				const int new_max_pos = tuning_step * lower_measurement.getParamValue();
				lower_measurement = Measurement(new_max_pos);
				upper_measurement = Measurement(tuning_step * new_max_pos);
			}
			max_positions = lower_measurement.getParamValue();
		}
		if (probability < 0.05f) // there is 5% chance that higher value of 'max_positions' gives higher speed
		{
			if (lower_measurement.getParamValue() / tuning_step >= 50)
			{
//				Logger::write("VCFSolver::tune() decreasing positions");
				const int new_max_pos = lower_measurement.getParamValue() / tuning_step;
				lower_measurement = Measurement(new_max_pos);
				upper_measurement = Measurement(tuning_step * new_max_pos);
			}
			max_positions = lower_measurement.getParamValue();
		}
	}
	SolverStats VCFSolver::getStats() const
	{
		std::cout << position_counter << " positions" << std::endl;
		return stats;
	}
	void VCFSolver::clearStats()
	{
		stats = SolverStats();
		lower_measurement.clear();
		upper_measurement.clear();
		step_counter = 0;
//		Logger::write("VCFSolver::clearStats() using " + std::to_string(max_positions) + " positions");
	}
	/*
	 * private
	 */
	void VCFSolver::add_move(Move move) noexcept
	{
		feature_extractor.addMove(move);
		hashtable.updateHash(move);
	}
	void VCFSolver::undo_move(Move move) noexcept
	{
		feature_extractor.undoMove(move);
		hashtable.updateHash(move);
	}
	bool VCFSolver::static_solve_1ply_win(SearchTask &task)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &own_five = feature_extractor.getThreats(ThreatType::FIVE, sign_to_move);
		if (own_five.size() > 0) // can make a five
		{
			for (auto iter = own_five.begin(); iter < own_five.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.setReady();
			return true;
		}
		return false;
	}
	bool VCFSolver::static_solve_1ply_draw(SearchTask &task)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();
		const int root_depth = feature_extractor.getRootDepth();
		if (root_depth + 1 == game_config.rows * game_config.cols)
		{
			Move m;
			for (int row = 0; row < game_config.rows; row++)
				for (int col = 0; col < game_config.cols; col++)
					if (feature_extractor.signAt(row, col) == Sign::NONE)
						m = Move(sign_to_move, row, col);
			task.addProvenEdge(m, ProvenValue::DRAW);
			task.setValue(Value(0.0f, 1.0, 0.0f));
			task.setReady();
			return true;
		}
		return false;
	}
	bool VCFSolver::static_solve_block_5(SearchTask &task)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &opponent_five = feature_extractor.getThreats(ThreatType::FIVE, invertSign(sign_to_move));
		switch (opponent_five.size())
		{
			case 0:
				return false;
			case 1:
			{
				for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
					task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);
				return true;
			}
			default:
			{
				for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
					task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::LOSS);
				task.setValue(Value(0.0f, 0.0, 1.0f));
				task.setReady(); // the state is provably losing, there is no need to further evaluate it
				return true;
			}
		}
	}
	bool VCFSolver::static_solve_3ply_win(SearchTask &task)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &own_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, sign_to_move);
		if (own_open_four.size() > 0) // we can make an open four, but it was already checked that opponent cannot make any five
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::WIN); // it is a win in 3 plys
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.setReady();
			return true;
		}
		return false;
	}
	void VCFSolver::static_solve_block_4(SearchTask &task)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &own_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, sign_to_move);
		const std::vector<Move> &opponent_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, invertSign(sign_to_move));
		const std::vector<Move> &opponent_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, invertSign(sign_to_move));
		if (opponent_open_four.size() > 0) // opponent can make an open four, but no fives. We also cannot make any five or open four
		{
			for (auto iter = opponent_open_four.begin(); iter < opponent_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);
			for (auto iter = opponent_half_open_four.begin(); iter < opponent_half_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);

			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++)
			{
				Move move_to_be_added(sign_to_move, *iter);
				bool is_already_added = std::any_of(task.getProvenEdges().begin(), task.getProvenEdges().end(), [move_to_be_added](const Edge &edge)
				{	return edge.getMove() == move_to_be_added;}); // find if such move has been added in any of two loops above
				if (not is_already_added) // move must not be added twice
					task.addProvenEdge(move_to_be_added, ProvenValue::UNKNOWN);
			}
		}
	}
	void VCFSolver::recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth)
	{
		position_counter += 1.0;
//		feature_extractor.print();
//		feature_extractor.printAllThreats();

		const Sign sign_to_move = invertSign(node.move.sign);
		const std::vector<Move> &own_five = feature_extractor.getThreats(ThreatType::FIVE, sign_to_move);
		if (own_five.size() > 0) // if side to move can make a five, mark this node as loss
		{
			node.solved_value = SolvedValue::LOSS;
			return;
		}

		const std::vector<Move> &opponent_five = feature_extractor.getThreats(ThreatType::FIVE, invertSign(sign_to_move));
		switch (opponent_five.size())
		{
			case 0:
			{
				const std::vector<Move> &own_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, sign_to_move);
				if (own_open_four.size() > 0) // if side to move can make an open four but the opponent has no fives, mark this node as loss
				{
					node.solved_value = SolvedValue::LOSS;
					return;
				}
				const std::vector<Move> &own_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, sign_to_move);
				node.number_of_children = static_cast<int>(own_half_open_four.size());
				break;
			}
			case 1:
				node.number_of_children = 1; // there is only one defensive move
				break;
			default:
				node.solved_value = SolvedValue::WIN; // if the other side side to move can make more than one five, mark this node as win
				return;
		}

		node.solved_value = SolvedValue::UNSOLVED;
		if (node.number_of_children == 0)
			return; // no available threats
		if (node_counter + node.number_of_children >= static_cast<int>(nodes_buffer.size()))
			return; // no more nodes in buffer, cannot continue

		node.children = nodes_buffer.data() + node_counter;
		node_counter += node.number_of_children;

		if (opponent_five.size() > 0) // must make this defensive move
		{
			assert(opponent_five.size() == 1); // it was checked earlier that opponent cannot have more than one five
			node.children[0].init(opponent_five.front().row, opponent_five.front().col, sign_to_move);
		}
		else
		{
			const std::vector<Move> &own_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, sign_to_move);
			size_t children_counter = 0;
			for (auto iter = own_half_open_four.begin(); iter < own_half_open_four.end(); iter++, children_counter++)
				node.children[children_counter].init(iter->row, iter->col, sign_to_move);
		}

		int loss_counter = 0;
		bool has_win_child = false;
		bool has_unproven_child = false;
		for (auto iter = node.children; iter < node.children + node.number_of_children; iter++)
		{
//			if (iter != node.children)
//			{
//				feature_extractor.print();
//				feature_extractor.printAllThreats();
//			}
//			std::cout << "positions checked = " << position_counter << std::endl;
//			std::cout << "depth = " << depth << " : state of all children:" << std::endl;
//			for (int i = 0; i < node.number_of_children; i++)
//				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << std::endl;
//			std::cout << "now checking " << iter->move.toString() << std::endl;

			hashtable.updateHash(iter->move);
			const SolvedValue solved_value_from_table = hashtable.get(hashtable.getHash());
			if (solved_value_from_table == SolvedValue::UNKNOWN) // not found
			{
				if (position_counter < max_positions)
				{
					feature_extractor.addMove(iter->move);
					recursive_solve(*iter, not mustProveAllChildren, depth + 1);
					feature_extractor.undoMove(iter->move);
				}
			}
			else
			{
				position_counter += 0.0625;
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
					loss_counter++;
				else // UNKNOWN or UNSOLVED
				{
					has_unproven_child = true;
					if (mustProveAllChildren) // if this is defensive side, it must prove all its child nodes as losing
						break; // so the function can exit now, as there will be at least one unproven node (current one)
				}
			}
		}

//		std::cout << "depth = " << depth << " : state of all children:\n";
//		for (int i = 0; i < node.number_of_children; i++)
//			std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << std::endl;

		if (has_win_child)
			node.solved_value = SolvedValue::LOSS;
		else
		{
			if (has_unproven_child and mustProveAllChildren)
				node.solved_value = SolvedValue::UNSOLVED;
			else
			{
				if (loss_counter == node.number_of_children)
					node.solved_value = SolvedValue::WIN;
				else
					node.solved_value = SolvedValue::UNSOLVED;
			}
		}
		hashtable.insert(hashtable.getHash(), node.solved_value);
		node_counter -= node.number_of_children;
	}

	void VCFSolver::recursive_solve_2(InternalNode &node, bool isAttackingSide)
	{
//		feature_extractor.print();
//		feature_extractor.printAllThreats();

		const Sign sign_to_move = invertSign(node.move.sign);
		if (isAttackingSide)
		{
			const std::vector<Move> &attacker_five = get_attacker_threats(ThreatType::FIVE);
			if (attacker_five.size() > 0) // if attacker can make a five, mark this node as loss
			{
				node.solved_value = SolvedValue::LOSS;
				return;
			}
			const std::vector<Move> &defender_five = get_defender_threats(ThreatType::FIVE);
			switch (defender_five.size())
			{
				case 0:
				{
					const std::vector<Move> &attacker_open_four = get_attacker_threats(ThreatType::OPEN_FOUR);
					if (attacker_open_four.size() > 0) // if attacker can make an open four but defender has no fives, mark this node as loss
					{
						node.solved_value = SolvedValue::LOSS;
						return;
					}

					const std::vector<Move> &attacker_half_open_four = get_attacker_threats(ThreatType::HALF_OPEN_FOUR);
					if (attacker_half_open_four.size() == 0) // attacker has no threats to make
					{
						node.solved_value = SolvedValue::UNSOLVED;
						return;
					}

					create_node_stack(node, attacker_half_open_four.size());
					size_t children_counter = 0;
					for (auto iter = attacker_half_open_four.begin(); iter < attacker_half_open_four.end(); iter++, children_counter++)
						node.children[children_counter].init(iter->row, iter->col, sign_to_move);
					break;
				}
				case 1:
				{
					create_node_stack(node, 1);
					node.children[0].init(defender_five.front().row, defender_five.front().col, sign_to_move);
					break;
				}
				default:
				{
					node.solved_value = SolvedValue::WIN; // if defender can make multiple fives, mark this node as win
					return;
				}
			}
		}
		else
		{
			const std::vector<Move> &attacker_five = get_attacker_threats(ThreatType::FIVE);
			switch (attacker_five.size())
			{
				case 0: // attacker lost initiative
				{
					node.solved_value = SolvedValue::UNSOLVED;
					return;
				}
				case 1: // single forced move
				{
					create_node_stack(node, 1);
					node.children[0].init(attacker_five.front().row, attacker_five.front().col, sign_to_move);
					break;
				}
				default: // attacker can make multiple fives, mark this node as win
				{
					node.solved_value = SolvedValue::WIN;
					return;
				}
			}
		}

		node.solved_value = SolvedValue::UNSOLVED;

		int num_losing_children = 0;
		bool has_win_child = false;
		for (auto iter = node.begin(); iter < node.end(); iter++)
		{
//			if (iter != node.children)
//			{
//				feature_extractor.print();
//				feature_extractor.printAllThreats();
//			}
//			std::cout << "positions checked = " << position_counter << std::endl;
//			std::cout << "state of all children:" << std::endl;
//			for (int i = 0; i < node.number_of_children; i++)
//				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << std::endl;
//			std::cout << "now checking " << iter->move.toString() << std::endl;

			hashtable.updateHash(iter->move);
			const SolvedValue solved_value_from_table = hashtable.get(hashtable.getHash());
			if (solved_value_from_table == SolvedValue::UNKNOWN) // not found
			{
				position_counter += 1.0;
				if (position_counter < max_positions)
				{
					feature_extractor.addMove(iter->move);
					recursive_solve_2(*iter, not isAttackingSide);
					feature_extractor.undoMove(iter->move);
				}
			}
			else
			{
				position_counter += 0.0625; // the position counter is increased to avoid infinite search over cached states
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
				else // UNKNOWN or UNSOLVED
				{
					if (not isAttackingSide) // if this is defensive side, it must prove all its child nodes as losing
						break; // so the function can exit now, as there will be at least one unproven node (current one)
				}
			}
		}

//		std::cout << "state of all children:\n";
//		for (int i = 0; i < node.number_of_children; i++)
//			std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << std::endl;

		if (has_win_child)
			node.solved_value = SolvedValue::LOSS;
		else
		{
			if (num_losing_children == node.number_of_children)
				node.solved_value = SolvedValue::WIN;
			else
				node.solved_value = SolvedValue::UNSOLVED;
		}
		hashtable.insert(hashtable.getHash(), node.solved_value);
		delete_node_stack(node);
	}

	const std::vector<Move>& VCFSolver::get_attacker_threats(ThreatType tt) noexcept
	{
		return feature_extractor.getThreats(tt, feature_extractor.getSignToMove());
	}
	const std::vector<Move>& VCFSolver::get_defender_threats(ThreatType tt) noexcept
	{
		return feature_extractor.getThreats(tt, invertSign(feature_extractor.getSignToMove()));
	}
	void VCFSolver::create_node_stack(InternalNode &node, int numberOfNodes) noexcept
	{
		assert(node_counter + numberOfNodes <= static_cast<int>(nodes_buffer.size()));
//		std::cout << "reserving " << numberOfNodes << " nodes for this level" << std::endl;
		node.number_of_children = numberOfNodes;
		node.children = nodes_buffer.data() + node_counter;
		node_counter += numberOfNodes;
	}
	void VCFSolver::delete_node_stack(InternalNode &node) noexcept
	{
		assert(node_counter >= node.number_of_children);
		node_counter -= node.number_of_children;
	}

} /* namespace ag */

