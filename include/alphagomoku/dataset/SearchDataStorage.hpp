/*
 * SearchDataStorage.hpp
 *
 *  Created on: Mar 12, 2023
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
			struct entry
			{
					Location location;
					uint16_t visit_count;
					CompressedFloat policy_prior;
					Score score;
					CompressedFloat win_rate;
					CompressedFloat draw_rate;
			};

			std::vector<entry> storage;
			Score minimax_score;
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
