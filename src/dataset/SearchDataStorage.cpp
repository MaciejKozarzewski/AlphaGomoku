/*
 * SearchDataStorage_v2.cpp
 *
 *  Created on: Mar 12, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/low_precision.hpp>

#include <minml/utils/serialization.hpp>

#include <iostream>

namespace
{
	using namespace ag;

	using score_format = LowFP<1, 3, 2, -8>;

	uint8_t score_to_int8(Score s) noexcept
	{
		const uint32_t pv = static_cast<uint32_t>(s.getProvenValue()) << 6;
		if (s.isProven())
			return pv | static_cast<uint32_t>(clamp(s.getDistance(), 0, 63));
		else
			return pv | score_format::to_lowp(s.getEval() / 1000.0f);
	}
	Score int8_to_score(uint8_t x) noexcept
	{
		const ProvenValue pv = static_cast<ProvenValue>(x >> 6);
		const uint32_t eval = x & 63u;
		switch (pv)
		{
			case ProvenValue::LOSS:
				return Score::loss_in(eval);
			case ProvenValue::DRAW:
				return Score::draw_in(eval);
			case ProvenValue::UNKNOWN:
				return Score(1000.0f * score_format::to_fp32(eval) + 0.5f);
			case ProvenValue::WIN:
				return Score::win_in(eval);
			default:
				return Score();
		}
	}

	Value get_valid_value(float winrate, float drawrate) noexcept
	{
		const float tmp = winrate + drawrate;
		if (tmp > 1.0f)
		{
			winrate /= tmp;
			drawrate /= tmp;
		}
		return Value(winrate, drawrate);
	}
}

namespace ag
{
	SearchDataStorage::SearchDataStorage(const SerializedObject &binary_data, size_t &offset)
	{
		minimax_score = binary_data.load<Score>(offset);
		offset += sizeof(Score);

		move_number = binary_data.load<uint16_t>(offset);
		offset += sizeof(uint16_t);

		unserializeVector(storage, binary_data, offset);
	}
	int SearchDataStorage::getMoveNumber() const noexcept
	{
		return move_number;
	}
	void SearchDataStorage::loadFrom(const SearchDataPack &pack)
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
					storage[entries_count].policy_prior = pack.policy_prior.at(row, col);
					storage[entries_count].score = score;
					storage[entries_count].win_rate = value.win_rate;
					storage[entries_count].draw_rate = value.draw_rate;
					entries_count++;
				}
	}
	void SearchDataStorage::storeTo(SearchDataPack &pack) const
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
			const Value q = get_valid_value(storage[i].win_rate, storage[i].draw_rate);
			pack.action_values.at(row, col) = q;
			const Score s = storage[i].score;
			pack.action_scores.at(row, col) = s;
			pack.policy_prior.at(row, col) = storage[i].policy_prior;

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
			pack.minimax_value = get_valid_value(win_rate / sum_visits, draw_rate / sum_visits);
	}
	void SearchDataStorage::serialize(SerializedObject &binary_data) const
	{
		binary_data.save(minimax_score);
		binary_data.save(move_number);
		serializeVector(storage, binary_data);
	}
	void SearchDataStorage::print() const
	{
		std::cout << "move number = " << move_number << '\n';
		for (size_t i = 0; i < storage.size(); i++)
			std::cout << sfill(i, 3, false) << "  " << storage[i].location.text() << "  " << sfill(storage[i].visit_count, 3, false) << "  S="
					<< storage[i].score.toFormattedString() << "  P=" << (float) storage[i].policy_prior << "  Q=" << (float) storage[i].win_rate
					<< ", " << (float) storage[i].draw_rate << '\n';
		std::cout << "minimax score = " << minimax_score.toFormattedString() << '\n';
		std::cout << '\n';
	}

	using visit_format = LowFP<0, 3, 5, -8>;
	using policy_format = LowFP<0, 4, 4, -16>;
	using value_format = LowFP<0, 4, 4, -16>;
	using fp16_format = LowFP<0, 5, 11, -16>;

	SearchDataStorage_v2::SearchDataStorage_v2(const SerializedObject &binary_data, size_t &offset)
	{
		value_scale = fp16_format::to_fp32(binary_data.load<uint16_t>(offset));
		offset += sizeof(uint16_t);
		policy_scale = fp16_format::to_fp32(binary_data.load<uint16_t>(offset));
		offset += sizeof(uint16_t);
		visit_scale = fp16_format::to_fp32(binary_data.load<uint16_t>(offset));
		offset += sizeof(uint16_t);

		minimax_score = binary_data.load<Score>(offset);
		offset += sizeof(Score);

		move_number = binary_data.load<uint16_t>(offset);
		offset += sizeof(uint16_t);

		unserializeVector(storage, binary_data, offset);
	}
	int SearchDataStorage_v2::numberOfEntries() const noexcept
	{
		return storage.size();
	}
	int SearchDataStorage_v2::getMoveNumber() const noexcept
	{
		return move_number;
	}
	void SearchDataStorage_v2::loadFrom(const SearchDataPack &pack)
	{
		move_number = 0;
		size_t entries_count = 0;
		policy_scale = 0.0f;
		value_scale = 0.0f;
		visit_scale = 1.0f;
		int last_idx = 0;
		for (int i = 0; i < pack.board.size(); i++)
		{
			if (pack.visit_count[i] > 0 or pack.action_scores[i].isProven() or (i - last_idx) >= 255)
			{
				entries_count++;
				last_idx = i;
			}
			move_number += static_cast<int>(pack.board[i] != Sign::NONE);
			policy_scale = std::max(policy_scale, pack.policy_prior[i]);
			value_scale = std::max(value_scale, std::max(pack.action_values[i].win_rate, pack.action_values[i].draw_rate));
			visit_scale = std::max(visit_scale, (float) pack.visit_count[i]);
		}
		storage.resize(entries_count);

		policy_scale = (policy_scale == 0.0f) ? 1.0f : (policy_scale / policy_format::max());
		value_scale = (value_scale == 0.0f) ? 1.0f : (value_scale / policy_format::max());
		visit_scale /= visit_format::max();

		minimax_score = pack.minimax_score;
		entries_count = 0;

		last_idx = 0;
		for (int i = 0; i < pack.board.size(); i++)
			if (pack.visit_count[i] > 0 or pack.action_scores[i].isProven() or (i - last_idx) >= 255)
			{
				const int visits = pack.visit_count[i];
				const Value value = pack.action_values[i];
				const Score score = pack.action_scores[i];

				assert(entries_count < storage.size());
				storage[entries_count].location_delta = i - last_idx;
				storage[entries_count].visit_count = visit_format::to_lowp(visits / visit_scale);
				storage[entries_count].policy_prior = policy_format::to_lowp(pack.policy_prior[i] / policy_scale);
				storage[entries_count].score = score_to_int8(score);
				storage[entries_count].win_rate = value_format::to_lowp(value.win_rate / value_scale);
				storage[entries_count].draw_rate = value_format::to_lowp(value.draw_rate / value_scale);
				entries_count++;
				last_idx = i;
			}
	}
	void SearchDataStorage_v2::storeTo(SearchDataPack &pack) const
	{
		int current_idx = 0;
		float win_rate = 0.0f, draw_rate = 0.0f;
		int sum_visits = 0;
		for (size_t i = 0; i < storage.size(); i++)
		{
			const int loc_delta = storage[i].location_delta;
			current_idx += loc_delta;
			assert(pack.board[i] == Sign::NONE);
			const float visits = visit_format::to_fp32(storage[i].visit_count) * visit_scale + 0.5f;
			pack.visit_count[current_idx] = visits;

			const Value q = get_valid_value(value_format::to_fp32(storage[i].win_rate) * value_scale,
					value_format::to_fp32(storage[i].draw_rate) * value_scale);
			pack.action_values[current_idx] = q;
			pack.action_scores[current_idx] = int8_to_score(storage[i].score);
			pack.policy_prior[current_idx] = policy_format::to_fp32(storage[i].policy_prior) * policy_scale;

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
			pack.minimax_value = get_valid_value(win_rate / sum_visits, draw_rate / sum_visits);
	}
	void SearchDataStorage_v2::serialize(SerializedObject &binary_data) const
	{
		binary_data.save(static_cast<uint16_t>(fp16_format::to_lowp(value_scale)));
		binary_data.save(static_cast<uint16_t>(fp16_format::to_lowp(policy_scale)));
		binary_data.save(static_cast<uint16_t>(fp16_format::to_lowp(visit_scale)));
		binary_data.save(minimax_score);
		binary_data.save(move_number);
		serializeVector(storage, binary_data);
	}
	void SearchDataStorage_v2::print() const
	{
		std::cout << "move number = " << move_number << '\n';
		std::cout << "value scale = " << value_scale << '\n';
		std::cout << "policy scale = " << policy_scale << '\n';
		std::cout << "visit scale = " << visit_scale << '\n';
		std::cout << "minimax score = " << minimax_score.toFormattedString() << '\n';
		for (size_t i = 0; i < storage.size(); i++)
			std::cout << sfill(i, 3, false) << " " << sfill(storage[i].location_delta, 3, false) << "  "
					<< sfill(visit_format::to_fp32(storage[i].visit_count) * visit_scale + 0.5f, 3, false) << "  S="
					<< int8_to_score(storage[i].score).toFormattedString() << "  P="
					<< (policy_format::to_fp32(storage[i].policy_prior) * policy_scale) << "  Q="
					<< (value_format::to_fp32(storage[i].win_rate) * value_scale) << ", "
					<< (value_format::to_fp32(storage[i].draw_rate) * value_scale) << '\n';
		std::cout << '\n';
	}

} /* namespace ag */
