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
			std::vector<Move> played_moves;
			GameOutcome game_outcome = GameOutcome::UNKNOWN;
			int rows = 0;
			int columns = 0;
		public:
			GameDataStorage() noexcept = default;
			GameDataStorage(int rows, int cols) noexcept;
			GameDataStorage(const SerializedObject &binary_data, size_t &offset);

			void clear();

			int numberOfMoves() const noexcept;
			int numberOfSamples() const noexcept;

			const SearchDataStorage& operator[](int index) const;

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
