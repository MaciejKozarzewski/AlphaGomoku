/*
 * GameDataStorage.cpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <minml/utils/serialization.hpp>

namespace ag
{
	GameDataStorage::GameDataStorage(int rows, int cols) noexcept :
			rows(rows),
			columns(cols)
	{
	}
	GameDataStorage::GameDataStorage(const SerializedObject &binary_data, size_t &offset)
	{
		const int loaded_version = binary_data.load<int>(offset);
		offset += sizeof(loaded_version);

		if (loaded_version != 0)
			throw std::logic_error("GameDataStorage() : loaded unsupported version " + std::to_string(loaded_version));

		const uint32_t number_of_states = binary_data.load<uint32_t>(offset);
		offset += sizeof(number_of_states);

		search_data.resize(number_of_states);
		for (size_t i = 0; i < number_of_states; i++)
			search_data[i] = SearchDataStorage(binary_data, offset);

		unserializeVector(played_moves, binary_data, offset);

		game_outcome = static_cast<GameOutcome>(binary_data.load<int>(offset));
		offset += sizeof(int);

		rows = binary_data.load<int>(offset);
		offset += sizeof(rows);
		columns = binary_data.load<int>(offset);
		offset += sizeof(columns);
	}

	void GameDataStorage::clear()
	{
		search_data.clear();
		played_moves.clear();
		game_outcome = GameOutcome::UNKNOWN;
	}

	int GameDataStorage::numberOfMoves() const noexcept
	{
		return played_moves.size();
	}
	int GameDataStorage::numberOfSamples() const noexcept
	{
		return search_data.size();
	}
	const SearchDataStorage& GameDataStorage::operator[](int index) const
	{
		return search_data.at(index);
	}

	void GameDataStorage::getSample(SearchDataPack &result, int index) const
	{
		result.clear();
		const int move_number = search_data.at(index).getMoveNumber();
		search_data.at(index).storeTo(result);
		for (int i = 0; i < move_number; i++)
			Board::putMove(result.board, played_moves.at(i));
		result.played_move = played_moves.at(move_number);
		result.game_outcome = game_outcome;
	}
	Move GameDataStorage::getMove(int index) const
	{
		return played_moves.at(index);
	}
	GameOutcome GameDataStorage::getOutcome() const noexcept
	{
		return game_outcome;
	}

	void GameDataStorage::addSample(const SearchDataPack &sample)
	{
		search_data.push_back(SearchDataStorage());
		search_data.back().loadFrom(sample);
	}
	void GameDataStorage::addMove(Move move)
	{
		assert(move.sign == Sign::CROSS || move.sign == Sign::CIRCLE);
		assert(std::find(played_moves.begin(), played_moves.end(), move) == played_moves.end()); // move must not be added twice

		played_moves.push_back(move);
	}
	void GameDataStorage::addMoves(const std::vector<Move> &moves)
	{
		for (size_t i = 0; i < moves.size(); i++)
			addMove(moves[i]);
	}
	void GameDataStorage::setOutcome(GameOutcome outcome) noexcept
	{
		game_outcome = outcome;
	}

	void GameDataStorage::serialize(SerializedObject &binary_data) const
	{
		binary_data.save<int>(version);
		binary_data.save<uint32_t>(static_cast<uint32_t>(search_data.size()));
		for (size_t i = 0; i < search_data.size(); i++)
			search_data[i].serialize(binary_data);
		serializeVector(played_moves, binary_data);
		binary_data.save<int>(static_cast<int>(game_outcome));
		binary_data.save<int>(rows);
		binary_data.save<int>(columns);
	}

} /* namespace ag */

