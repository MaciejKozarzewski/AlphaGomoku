/*
 * SearchDataStorage_v2.cpp
 *
 *  Created on: Mar 12, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/SearchDataStorage_v2.hpp>
#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <minml/utils/serialization.hpp>

#include <map>
#include <iostream>

namespace ag
{
	SearchDataStorage_v2::SearchDataStorage_v2(const SerializedObject &binary_data, size_t &offset)
	{
		minimax_score = binary_data.load<Score>(offset);
		offset += sizeof(Score);

		move_number = binary_data.load<uint16_t>(offset);
		offset += sizeof(uint16_t);

		unserializeVector(storage, binary_data, offset);
	}
	int SearchDataStorage_v2::getMoveNumber() const noexcept
	{
		return move_number;
	}
	void SearchDataStorage_v2::loadFrom(const SearchDataPack &pack)
	{
		move_number = 0;
		size_t entries_count = 0;
		for (int i = 0; i < pack.board.size(); i++)
		{
			entries_count += static_cast<int>(pack.visit_count[i] > 0 or pack.action_scores[i].isProven());
			move_number += static_cast<int>(pack.board[i] != Sign::NONE);
		}
		storage.resize(entries_count);

		minimax_score = pack.minimax_score;
		entries_count = 0;
		for (int row = 0; row < pack.board.rows(); row++)
			for (int col = 0; col < pack.board.cols(); col++)
				if (pack.visit_count.at(row, col) > 0 or pack.action_scores.at(row, col).isProven())
				{
					const int visits = pack.visit_count.at(row, col);
					assert(visits <= std::numeric_limits<uint16_t>::max());
					const Value value = pack.action_values.at(row, col);
					const Score score = pack.action_scores.at(row, col);

					assert(entries_count < storage.size());
					storage[entries_count].location = Location(row, col);
					storage[entries_count].visit_count = visits;
					storage[entries_count].score = score;
					storage[entries_count].policy_prior = pack.policy_prior.at(row, col);
					storage[entries_count].win_rate = value.win_rate;
					storage[entries_count].draw_rate = value.draw_rate;
					entries_count++;
				}
	}
	void SearchDataStorage_v2::storeTo(SearchDataPack &pack) const
	{
		float win_rate = 0.0f, draw_rate = 0.0f;
		int sum_visits = 0;
		// now loop over stored mcts results and put them at appropriate places
		for (size_t i = 0; i < storage.size(); i++)
		{
			const int row = storage[i].location.row;
			const int col = storage[i].location.col;
			assert(pack.board.at(row, col) == Sign::NONE);
			const int visits = storage[i].visit_count;
			pack.visit_count.at(row, col) = storage[i].visit_count;
			pack.policy_prior.at(row, col) = storage[i].policy_prior;
			const Value q(storage[i].win_rate, storage[i].draw_rate);
			pack.action_values.at(row, col) = q;
			const Score s = storage[i].score;
			pack.action_scores.at(row, col) = s;

			sum_visits += visits;
			win_rate += q.win_rate * visits;
			draw_rate += q.draw_rate * visits;
		}

		pack.minimax_score = minimax_score;
		if (sum_visits == 0)
		{
			assert(minimax_score.isProven());
			pack.minimax_value = minimax_score.convertToValue();
		}
		else
			pack.minimax_value = Value(win_rate / sum_visits, draw_rate / sum_visits);
	}
	void SearchDataStorage_v2::serialize(SerializedObject &binary_data) const
	{
		binary_data.save(minimax_score);
		binary_data.save(move_number);
		serializeVector(storage, binary_data);
	}
	void SearchDataStorage_v2::print() const
	{
		std::cout << "move number = " << move_number << '\n';
		for (size_t i = 0; i < storage.size(); i++)
			std::cout << sfill(i, 3, false) << "  " << storage[i].location.text() << "  " << sfill(storage[i].visit_count, 3, false) << "  S="
					<< storage[i].score.toFormattedString() << "  P=" << (float) storage[i].policy_prior << "  Q=" << (float) storage[i].win_rate
					<< ", " << (float) storage[i].draw_rate << '\n';
		std::cout << "minimax score = " << minimax_score.toFormattedString() << '\n';
		std::cout << '\n';
	}

} /* namespace ag */
