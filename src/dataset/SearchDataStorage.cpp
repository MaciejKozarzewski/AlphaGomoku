/*
 * SearchDataStorage.cpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <minml/utils/serialization.hpp>

#include <map>
#include <iostream>

namespace ag
{
	SearchDataStorage::SearchDataStorage(const SerializedObject &binary_data, size_t &offset)
	{
		base_policy_prior = binary_data.load<float>(offset);
		offset += sizeof(base_policy_prior);

		base_score = binary_data.load<Score>(offset);
		offset += sizeof(base_score);

		move_number = binary_data.load<uint16_t>(offset);
		offset += sizeof(move_number);

		unserializeVector(mcts_storage, binary_data, offset);
		unserializeVector(score_storage, binary_data, offset);
	}
	int SearchDataStorage::getMoveNumber() const noexcept
	{
		return move_number;
	}
	void SearchDataStorage::loadFrom(const SearchDataPack &pack)
	{
		// first create a map of all values of score that appear in the data pack
		base_policy_prior = 0.0f;
		move_number = 0;
		std::map<Score, int> scores_count;
		size_t mcts_entries_count = 0;
		size_t score_entries_count = 0;
		int discarded_actions = 0;
		for (int i = 0; i < pack.board.size(); i++)
			if (pack.board[i] == Sign::NONE)
			{
				if (pack.visit_count[i] > 0 or pack.policy_prior[i] > 0.01f)
					mcts_entries_count++;
				else
				{
					base_policy_prior += pack.policy_prior[i];
					discarded_actions++;
				}
				score_entries_count++;
				const Score score = pack.action_scores[i];
				auto iter = scores_count.find(score);
				if (iter == scores_count.end())
					scores_count.insert( { score, 1 });
				else
					iter->second++;
			}
			else
				move_number++;
		if (discarded_actions > 0)
			base_policy_prior /= discarded_actions; // the discarded policy is evenly distributed among unvisited actions (TODO can it be better?)

		// now loop over all distinct scores to pick the most frequent one as a baseline
		if (scores_count.size() > 0)
		{
			int most_common_score_count = 0;
			for (auto iter = scores_count.begin(); iter != scores_count.end(); iter++)
				if (iter->second >= most_common_score_count)
				{
					most_common_score_count = iter->second;
					base_score = iter->first;
				}
			score_entries_count -= most_common_score_count; // now calculate final number of score entries to be saved
		}

		// prepare storage space
		mcts_storage.resize(mcts_entries_count);
		score_storage.resize(score_entries_count);

		// now store the remaining elements
		mcts_entries_count = 0;
		score_entries_count = 0;
		for (int row = 0; row < pack.board.rows(); row++)
			for (int col = 0; col < pack.board.cols(); col++)
				if (pack.board.at(row, col) == Sign::NONE)
				{
					if (pack.visit_count.at(row, col) > 0 or pack.policy_prior.at(row, col) > 0.01f)
					{
						const int visits = pack.visit_count.at(row, col);
						assert(visits <= std::numeric_limits<uint16_t>::max());
						const Value value = (visits > 0) ? pack.action_values.at(row, col) : Value();

						assert(mcts_entries_count < mcts_storage.size());
						mcts_storage[mcts_entries_count].location = Location(row, col);
						mcts_storage[mcts_entries_count].visit_count = visits;
						mcts_storage[mcts_entries_count].policy_prior = pack.policy_prior.at(row, col);
						mcts_storage[mcts_entries_count].win_rate = value.win_rate;
						mcts_storage[mcts_entries_count].draw_rate = value.draw_rate;
						mcts_entries_count++;
					}
					const Score score = pack.action_scores.at(row, col);
					if (score != base_score)
					{
						assert(score_entries_count < score_storage.size());
						score_storage[score_entries_count].score = score;
						score_storage[score_entries_count].location = Location(row, col);
						score_entries_count++;
					}
				}
	}
	void SearchDataStorage::storeTo(SearchDataPack &pack) const
	{
		// first set base score and policy on all empty spots
		for (int row = 0; row < pack.board.rows(); row++)
			for (int col = 0; col < pack.board.cols(); col++)
				if (pack.board.at(row, col) == Sign::NONE)
				{
					pack.action_scores.at(row, col) = base_score;
					pack.policy_prior.at(row, col) = base_policy_prior;
				}

		float win_rate = 0.0f, draw_rate = 0.0f;
		int sum_visits = 0;
		// now loop over stored mcts results and put them at appropriate places
		for (size_t i = 0; i < mcts_storage.size(); i++)
		{
			const int row = mcts_storage[i].location.row;
			const int col = mcts_storage[i].location.col;
			assert(pack.board.at(row, col) == Sign::NONE);
			const int visits = mcts_storage[i].visit_count;
			pack.visit_count.at(row, col) = mcts_storage[i].visit_count;
			pack.policy_prior.at(row, col) = mcts_storage[i].policy_prior;
			const Value q(mcts_storage[i].win_rate, mcts_storage[i].draw_rate);
			pack.action_values.at(row, col) = q;

			sum_visits += visits;
			win_rate += q.win_rate * visits;
			draw_rate += q.draw_rate * visits;
		}
		pack.minimax_value = (sum_visits > 0) ? Value(win_rate / sum_visits, draw_rate / sum_visits) : Value();

		// now loop over stored scores and put them at appropriate places
		Score minimax_score = Score::min_value();
		for (size_t i = 0; i < score_storage.size(); i++)
		{
			const int row = score_storage[i].location.row;
			const int col = score_storage[i].location.col;
			assert(pack.board.at(row, col) == Sign::NONE);
			const Score s = score_storage[i].score;
			pack.action_scores.at(row, col) = s;

			minimax_score = std::max(minimax_score, s);
		}
		pack.minimax_score = minimax_score;
		if (minimax_score.isProven())
			pack.minimax_value = minimax_score.convertToValue();
	}
	void SearchDataStorage::serialize(SerializedObject &binary_data) const
	{
		binary_data.save(base_policy_prior);
		binary_data.save(base_score);
		binary_data.save(move_number);
		serializeVector(mcts_storage, binary_data);
		serializeVector(score_storage, binary_data);
	}
	void SearchDataStorage::print() const
	{
		std::cout << "move number = " << move_number << '\n';
		std::cout << "base policy prior = " << base_policy_prior << '\n';
		for (size_t i = 0; i < mcts_storage.size(); i++)
			std::cout << sfill(i, 3, false) << "  " << mcts_storage[i].location.text() << "  " << sfill(mcts_storage[i].visit_count, 3, false)
					<< "  P=" << (float) mcts_storage[i].policy_prior << "  Q=" << (float) mcts_storage[i].win_rate << ", "
					<< (float) mcts_storage[i].draw_rate << '\n';
		std::cout << "base score = " << base_score.toFormattedString() << '\n';
		for (size_t i = 0; i < score_storage.size(); i++)
			std::cout << sfill(i, 3, false) << "  " << score_storage[i].location.text() << " " << score_storage[i].score.toFormattedString() << '\n';
	}

} /* namespace ag */
