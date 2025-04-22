/*
 * GameDataStorage.hpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_GAMEDATASTORAGE_HPP_
#define ALPHAGOMOKU_DATASET_GAMEDATASTORAGE_HPP_

#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>

#include <vector>

class SerializedObject;

namespace ag
{
	struct SearchDataPack;
}

namespace ag
{
	class GameDataStorage
	{
		private:
			std::vector<SearchDataStorage> search_data;
			std::vector<SearchDataStorage_v2> search_data_v2;
			std::vector<Move> played_moves;
			std::vector<uint16_t> played_moves_v2;
			GameOutcome game_outcome = GameOutcome::UNKNOWN;
			int format = 200;
			int rows = 0;
			int columns = 0;
		public:
			GameDataStorage() noexcept = default;
			GameDataStorage(int rows, int cols, int format) noexcept;
			GameDataStorage(const SerializedObject &binary_data, size_t &offset, int format);

			void clear();

			int numberOfMoves() const;
			int numberOfSamples() const;

			void getSample(SearchDataPack &result, int index) const;
			Move getMove(int index) const;
			GameOutcome getOutcome() const noexcept;

			void addSample(const SearchDataPack &sample);
			void addMove(Move move);
			void addMoves(const std::vector<Move> &moves);
			void setOutcome(GameOutcome outcome) noexcept;

			void serialize(SerializedObject &binary_data) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_GAMEDATASTORAGE_HPP_ */
