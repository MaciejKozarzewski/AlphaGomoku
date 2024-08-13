/*
 * GameDataStorage.cpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/file_util.hpp>

#include <minml/utils/serialization.hpp>

#include <iostream>

namespace ag
{
	GameDataStorage::GameDataStorage(int rows, int cols, int format) noexcept :
			format(format),
			rows(rows),
			columns(cols)
	{
	}
	GameDataStorage::GameDataStorage(const SerializedObject &binary_data, size_t &offset, int format) :
			format(format)
	{
		const uint32_t number_of_states = binary_data.load<uint32_t>(offset);
		offset += sizeof(uint32_t);

		switch (format)
		{
			case 100:
			{
				search_data.resize(number_of_states);
				for (size_t i = 0; i < number_of_states; i++)
					search_data[i] = SearchDataStorage(binary_data, offset);
				unserializeVector(played_moves, binary_data, offset);
				break;
			}
			case 200:
			{
				search_data_v2.resize(number_of_states);
				for (size_t i = 0; i < number_of_states; i++)
					search_data_v2[i] = SearchDataStorage_v2(binary_data, offset);
				unserializeVector(played_moves_v2, binary_data, offset);
				break;
			}
			default:
				throw std::logic_error("GameDataStorage() unsupported format " + std::to_string(format));
		}

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
		search_data_v2.clear();
		played_moves.clear();
		played_moves_v2.clear();
		game_outcome = GameOutcome::UNKNOWN;
	}

	int GameDataStorage::numberOfMoves() const
	{
		switch (format)
		{
			case 100:
				return played_moves.size();
			case 200:
				return played_moves_v2.size();
			default:
				throw std::logic_error("GameDataStorage::numberOfMoves() unsupported format " + std::to_string(format));
		}
	}
	int GameDataStorage::numberOfSamples() const
	{
		switch (format)
		{
			case 100:
				return search_data.size();
			case 200:
				return search_data_v2.size();
			default:
				throw std::logic_error("GameDataStorage::numberOfSamples() unsupported format " + std::to_string(format));
		}
	}

	void GameDataStorage::getSample(SearchDataPack &result, int index) const
	{
		result.clear();
		switch (format)
		{
			case 100:
			{
				search_data.at(index).storeTo(result);
				const int move_number = search_data.at(index).getMoveNumber();
				result.played_move = played_moves.at(move_number);
				for (int i = 0; i < move_number; i++)
					Board::putMove(result.board, played_moves.at(i));
				break;
			}
			case 200:
			{
				search_data_v2.at(index).storeTo(result);
				const int move_number = search_data_v2.at(index).getMoveNumber();
				result.played_move = Move(played_moves_v2.at(move_number));
				for (int i = 0; i < move_number; i++)
					Board::putMove(result.board, Move(played_moves_v2.at(i)));
				break;
			}
			default:
				throw std::logic_error("GameDataStorage::getSample() unsupported format " + std::to_string(format));
		}
		result.game_outcome = game_outcome;
	}
	Move GameDataStorage::getMove(int index) const
	{
		switch (format)
		{
			case 100:
				return played_moves.at(index);
			case 200:
				return Move(played_moves_v2.at(index));
			default:
				throw std::logic_error("GameDataStorage::getMove() unsupported format " + std::to_string(format));
		}
	}
	GameOutcome GameDataStorage::getOutcome() const noexcept
	{
		return game_outcome;
	}

	void GameDataStorage::addSample(const SearchDataPack &sample)
	{
		switch (format)
		{
			case 100:
				search_data.push_back(SearchDataStorage());
				search_data.back().loadFrom(sample);
				break;
			case 200:
				search_data_v2.push_back(SearchDataStorage_v2());
				search_data_v2.back().loadFrom(sample);
				break;
			default:
				throw std::logic_error("GameDataStorage::addSample() unsupported format " + std::to_string(format));
		}

	}
	void GameDataStorage::addMove(Move move)
	{
		assert(move.sign == Sign::CROSS || move.sign == Sign::CIRCLE);

		switch (format)
		{
			case 100:
				assert(std::find(played_moves.begin(), played_moves.end(), move) == played_moves.end()); // move must not be added twice
				played_moves.push_back(move);
				break;
			case 200:
				assert(std::find(played_moves_v2.begin(), played_moves_v2.end(), move.toShort()) == played_moves_v2.end()); // move must not be added twice
				played_moves_v2.push_back(move.toShort());
				break;
			default:
				throw std::logic_error("GameDataStorage::addMove() unsupported format " + std::to_string(format));
		}
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
		switch (format)
		{
			case 100:
			{
				binary_data.save<uint32_t>(static_cast<uint32_t>(search_data.size()));
				for (size_t i = 0; i < search_data.size(); i++)
					search_data[i].serialize(binary_data);
				serializeVector(played_moves, binary_data);
				break;
			}
			case 200:
			{
				binary_data.save<uint32_t>(static_cast<uint32_t>(search_data_v2.size()));
				for (size_t i = 0; i < search_data_v2.size(); i++)
					search_data_v2[i].serialize(binary_data);
				serializeVector(played_moves_v2, binary_data);
				break;
			}
			default:
				throw std::logic_error("GameDataStorage::serialize() unsupported format " + std::to_string(format));
		}
		binary_data.save<int>(static_cast<int>(game_outcome));
		binary_data.save<int>(rows);
		binary_data.save<int>(columns);
	}

} /* namespace ag */

