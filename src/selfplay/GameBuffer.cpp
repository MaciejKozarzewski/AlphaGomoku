/*
 * GameBuffer.cpp
 *
 *  Created on: Mar 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>

#include <stddef.h>
#include <iostream>
#include <stdexcept>

namespace ag
{
	GameBufferStats& GameBufferStats::operator+=(const GameBufferStats &other) noexcept
	{
		this->played_games += other.played_games;
		this->positions += other.positions;
		this->cross_win += other.cross_win;
		this->draws += other.draws;
		this->circle_win += other.circle_win;
		this->game_length += other.game_length;
		return *this;
	}
	std::string GameBufferStats::toString() const
	{
		std::string result;
		result += "----GameBufferStats----\n";
		result += "played games  = " + std::to_string(played_games) + '\n';
		result += "positions     = " + std::to_string(positions) + '\n';
		result += "cross  = " + std::to_string(cross_win) + '\n';
		result += "draws  = " + std::to_string(draws) + '\n';
		result += "circle = " + std::to_string(circle_win) + '\n';
		result += "avg length = " + std::to_string((float) game_length / played_games) + '\n';
		return result;
	}

	GameBuffer::GameBuffer(const std::string &path)
	{
		load(path);
	}
	void GameBuffer::clear() noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.clear();
	}
	int GameBuffer::size() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.size();
	}
	void GameBuffer::addToBuffer(const Game &game)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.push_back(game);
	}
	const Game& GameBuffer::getFromBuffer(int index) const
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.at(index);
	}
	Game& GameBuffer::getFromBuffer(int index)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.at(index);
	}
	void GameBuffer::removeFromBuffer(int index)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.erase(buffer_data.begin() + index);
	}
	void GameBuffer::removeRange(int from, int to)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.erase(buffer_data.begin() + from, buffer_data.begin() + to);
	}
	void GameBuffer::save(const std::string &path) const
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		Json json(JsonType::Array);
		SerializedObject so;
		for (size_t i = 0; i < buffer_data.size(); i++)
			json[i] = buffer_data[i].serialize(so);

		FileSaver fs(path);
		fs.save(json, so, -1, true);
		fs.close();
	}
	void GameBuffer::load(const std::string &path)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		FileLoader fl(path, true);
		size_t size = fl.getJson().size();
		for (size_t i = 0; i < size; i++)
			buffer_data.push_back(Game(fl.getJson()[i], fl.getBinaryData()));
	}
	GameBufferStats GameBuffer::getStats() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		GameBufferStats stats;
		stats.played_games = buffer_data.size();
		for (size_t i = 0; i < buffer_data.size(); i++)
		{
			stats.positions += buffer_data[i].getNumberOfSamples();
			stats.game_length += buffer_data[i].length();
			switch (buffer_data[i].getOutcome())
			{
				default:
					break;
				case GameOutcome::CROSS_WIN:
					stats.cross_win++;
					break;
				case GameOutcome::DRAW:
					stats.draws++;
					break;
				case GameOutcome::CIRCLE_WIN:
					stats.circle_win++;
					break;
			}
		}
		return stats;
	}
	bool GameBuffer::isCorrect() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		for (size_t i = 0; i < buffer_data.size(); i++)
			if (buffer_data[i].isCorrect() == false)
				return false;
		return true;
	}
	std::string GameBuffer::generatePGN(bool fullGameHistory)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		std::string result;
		for (size_t i = 0; i < buffer_data.size(); i++)
			result += buffer_data[i].generatePGN(fullGameHistory);
		return result;
	}

	PositionBuffer::PositionBuffer(const std::string &path)
	{
		load(path);
	}
	void PositionBuffer::clear() noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.clear();
	}
	int PositionBuffer::size() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.size();
	}
	void PositionBuffer::addToBuffer(const SearchData &position)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.push_back(position);
	}
	const SearchData& PositionBuffer::getFromBuffer(int index) const
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.at(index);
	}
	SearchData& PositionBuffer::getFromBuffer(int index)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return buffer_data.at(index);
	}
	void PositionBuffer::removeFromBuffer(int index)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.erase(buffer_data.begin() + index);
	}
	void PositionBuffer::removeRange(int from, int to)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer_data.erase(buffer_data.begin() + from, buffer_data.begin() + to);
	}
	void PositionBuffer::save(const std::string &path) const
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		SerializedObject so;
		so.save<int64_t>(buffer_data.size());
		for (size_t i = 0; i < buffer_data.size(); i++)
			buffer_data[i].serialize(so);

		FileSaver fs(path);
		fs.save(Json(), so, -1, true);
		fs.close();
	}
	void PositionBuffer::load(const std::string &path)
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		FileLoader fl(path, true);
		int64_t number_of_positions = fl.getBinaryData().load<int64_t>(0);

		size_t offset = sizeof(number_of_positions);
		for (int64_t i = 0; i < number_of_positions; i++)
			buffer_data.push_back(SearchData(fl.getBinaryData(), offset));
	}
	GameBufferStats PositionBuffer::getStats() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		GameBufferStats stats;
		stats.played_games = buffer_data.size();
		stats.positions = buffer_data.size();
		for (size_t i = 0; i < buffer_data.size(); i++)
		{
			switch (buffer_data[i].getOutcome())
			{
				default:
					break;
				case GameOutcome::CROSS_WIN:
					stats.cross_win++;
					break;
				case GameOutcome::DRAW:
					stats.draws++;
					break;
				case GameOutcome::CIRCLE_WIN:
					stats.circle_win++;
					break;
			}
		}
		return stats;
	}
	bool PositionBuffer::isCorrect() const noexcept
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		for (size_t i = 0; i < buffer_data.size(); i++)
			if (buffer_data[i].isCorrect() == false)
				return false;
		return true;
	}

} /* namespace ag */

