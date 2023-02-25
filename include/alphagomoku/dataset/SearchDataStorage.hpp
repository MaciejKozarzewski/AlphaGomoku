/*
 * SearchDataStorage.hpp
 *
 *  Created on: Feb 25, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_SEARCHDATASTORAGE_HPP_
#define ALPHAGOMOKU_DATASET_SEARCHDATASTORAGE_HPP_

#include <alphagomoku/dataset/CompressedFloat.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/Score.hpp>

#include <vector>

class SerializedObject;

namespace ag
{
	struct SearchDataPack;
}

namespace ag
{
	class SearchDataStorage
	{
			struct mcts_entry
			{
					Location location;
					uint16_t visit_count;
					CompressedFloat policy_prior;
					CompressedFloat win_rate;
					CompressedFloat draw_rate;
			};
			struct score_entry
			{
					Score score;
					Location location;
			};

			std::vector<mcts_entry> mcts_storage;
			std::vector<score_entry> score_storage;
			float base_policy_prior = 0.0f;
			Score base_score;
			uint16_t move_number = 0;

		public:
			SearchDataStorage() noexcept = default;
			SearchDataStorage(const SerializedObject &binary_data, size_t &offset);

			int getMoveNumber() const noexcept;
			void loadFrom(const SearchDataPack &pack);
			void storeTo(SearchDataPack &pack) const;
			void serialize(SerializedObject &binary_data) const;
			void print() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_SEARCHDATASTORAGE_HPP_ */
