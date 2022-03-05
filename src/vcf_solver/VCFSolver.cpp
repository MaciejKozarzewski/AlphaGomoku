/*
 * VCFSolver.cpp
 *
 *  Created on: Mar 4, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/vcf_solver/VCFSolver.hpp>
#include <alphagomoku/mcts/SearchTask.hpp>

#include <iostream>

namespace ag
{
	void VCFSolver::SolverData::clear() noexcept
	{
		actions.clear();
		current_action_index = 0;
		solved_value = SolvedValue::UNKNOWN;
	}
	void VCFSolver::SolverData::updateWith(SolvedValue sv) noexcept
	{
		switch (sv)
		{
			case SolvedValue::UNKNOWN:
			case SolvedValue::UNSOLVED:
				has_unsolved_action = true;
				break;
			case SolvedValue::LOSS:
				losing_actions_count++;
				break;
			case SolvedValue::WIN:
				has_win_action = true;
				break;
		}
	}
	SolvedValue VCFSolver::SolverData::getSolvedValue() const noexcept
	{
		if (has_win_action)
			return SolvedValue::WIN;
		else
		{
			if (losing_actions_count == static_cast<int>(actions.size()))
				return SolvedValue::LOSS;
			else
			{
				if (has_unsolved_action)
					return SolvedValue::UNSOLVED;
				else
					return SolvedValue::UNKNOWN;
			}
		}
	}
	bool VCFSolver::SolverData::isSolved() const noexcept
	{
		return getSolvedValue() != SolvedValue::UNKNOWN;
	}

	VCFSolver::VCFSolver(GameConfig gameConfig) :
			statistics(gameConfig.rows * gameConfig.cols),
			search_path(gameConfig.rows * gameConfig.cols),
			nodes_buffer(10000),
			game_config(gameConfig),
			feature_extractor(gameConfig),
			hashtable(gameConfig.rows * gameConfig.cols, 1024)
	{
	}

	void VCFSolver::solve(SearchTask &task, int level)
	{
		feature_extractor.setBoard(task.getBoard(), task.getSignToMove());
		hashtable.setBoard(task.getBoard());

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
		success = static_solve_block_4(task);
		if (success)
			return;

		if (level <= 1)
			return; // winning and forced moves will be found

		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &own_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, sign_to_move);
		const std::vector<Move> &own_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, sign_to_move);
		if (own_open_four.size() + own_half_open_four.size() == 0)
			return; // there are not threats that we can make

		if (use_caching)
			hashtable.clear();

		const int root_depth = feature_extractor.getRootDepth();
		position_counter = 0;
		node_counter = 1; // prepare node stack
		nodes_buffer.front().init(0, 0, invertSign(sign_to_move)); // prepare node stack
		recursive_solve(nodes_buffer.front(), false, 0);

		total_positions += position_counter;
		statistics[root_depth].add(nodes_buffer.front().solved_value == SolvedValue::LOSS, position_counter);
//		std::cout << "depth = " << root_depth << ", result = " << static_cast<int>(nodes_buffer.front().solved_value) << ", checked "
//				<< position_counter << " positions, total = " << total_positions << "\n";
		if (nodes_buffer.front().solved_value == SolvedValue::LOSS)
		{
			for (auto iter = nodes_buffer[0].children; iter < nodes_buffer[0].children + nodes_buffer[0].number_of_children; iter++)
				if (iter->solved_value == SolvedValue::WIN)
					task.addProvenEdge(Move(sign_to_move, iter->move), ProvenValue::WIN);
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.setReady();
		}
	}

	void VCFSolver::print_stats() const
	{
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
		if (opponent_five.size() > 0) // opponent can make a five
		{
			if (opponent_five.size() > 1)
			{
				for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
					task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::LOSS);
				task.setValue(Value(0.0f, 0.0, 1.0f));
				task.setReady(); // the state is provably losing, there is no need to further evaluate it
			}
			else
			{
				for (auto iter = opponent_five.begin(); iter < opponent_five.end(); iter++)
					task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::UNKNOWN);
			}
			return true;
		}
		return false;
	}
	bool VCFSolver::static_solve_3ply_win(SearchTask &task)
	{
		const Sign sign_to_move = feature_extractor.getSignToMove();
		const std::vector<Move> &own_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, sign_to_move);
		if (own_open_four.size() > 0) // can make an open four, but it was already checked that opponent cannot make any five
		{
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++)
				task.addProvenEdge(Move(sign_to_move, *iter), ProvenValue::WIN); // it is a win in 3 plys
			task.setValue(Value(1.0f, 0.0, 0.0f));
			task.setReady();
			return true;
		}
		return false;
	}
	bool VCFSolver::static_solve_block_4(SearchTask &task)
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
			return true;
		}
		return false;
	}
	void VCFSolver::recursive_solve()
	{
//		for (size_t i = 0; i < search_path.size(); i++)
//			search_path[i].clear();
//
//		const int root_depth = feature_extractor.getRootDepth();
//		int current_depth = root_depth;
//		while (current_depth >= root_depth)
//		{
//			const Sign sign_to_move = feature_extractor.getSignToMove();
//
//			const std::vector<Move> &own_five = feature_extractor.getThreats(ThreatType::FIVE, sign_to_move);
//		}
	}
	void VCFSolver::recursive_solve(InternalNode &node, bool mustProveAllChildren, int depth)
	{
		position_counter++;
//		print();
//		printAllThreats();

		const Sign sign_to_move = invertSign(node.move.sign);
		const std::vector<Move> &own_five = feature_extractor.getThreats(ThreatType::FIVE, sign_to_move);
		if (own_five.size() > 0) // if side to move can make a five, mark this node as loss
		{
			node.solved_value = SolvedValue::LOSS;
			return;
		}

		const std::vector<Move> &opponent_five = feature_extractor.getThreats(ThreatType::FIVE, invertSign(sign_to_move));
		if (opponent_five.size() > 1) // if the other side side to move can make more than one five, mark this node as win
		{
			node.solved_value = SolvedValue::WIN;
			return;
		}

		const std::vector<Move> &own_open_four = feature_extractor.getThreats(ThreatType::OPEN_FOUR, sign_to_move);
		const std::vector<Move> &own_half_open_four = feature_extractor.getThreats(ThreatType::HALF_OPEN_FOUR, sign_to_move);

		if (opponent_five.size() > 0)
			node.number_of_children = 1; // there is only one defensive move
		else
			node.number_of_children = static_cast<int>(own_open_four.size() + own_half_open_four.size());

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
			node.children[0].init(opponent_five[0].row, opponent_five[0].col, sign_to_move);
		}
		else
		{
			size_t children_counter = 0;
			for (auto iter = own_open_four.begin(); iter < own_open_four.end(); iter++, children_counter++)
				node.children[children_counter].init(iter->row, iter->col, sign_to_move);
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
//				print();
//				printAllThreats();
//			}
//			std::cout << "depth = " << depth << " : state of all children:\n";
//			for (int i = 0; i < node.number_of_children; i++)
//				std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';
//			std::cout << "now checking " << iter->move.toString() << '\n';

			hashtable.updateHash(iter->move);
			const SolvedValue solved_value_from_table = use_caching ? hashtable.get(hashtable.getHash()) : SolvedValue::UNKNOWN;
			if (solved_value_from_table == SolvedValue::UNKNOWN) // not found
			{
				if (position_counter < max_positions and depth <= max_depth)
				{
					feature_extractor.addMove(iter->move);
					recursive_solve(*iter, not mustProveAllChildren, depth + 1);
					feature_extractor.undoMove(iter->move);
				}
			}
			else
				iter->solved_value = solved_value_from_table; // cache hit
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
//			std::cout << i << " : " << node.children[i].move.toString() << " = " << toString(node.children[i].solved_value) << '\n';

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
		if (use_caching)
			hashtable.insert(hashtable.getHash(), node.solved_value);
		node_counter -= node.number_of_children;
	}
} /* namespace ag */

