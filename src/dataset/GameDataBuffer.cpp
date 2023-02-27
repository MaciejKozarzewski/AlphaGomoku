/*
 * GameDataBuffer.cpp
 *
 *  Created on: Feb 25, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <cstddef>
#include <stdexcept>

namespace ag
{
	GameDataBufferStats& GameDataBufferStats::operator+=(const GameDataBufferStats &other) noexcept
	{
		this->games += other.games;
		this->samples += other.samples;
		this->cross_win += other.cross_win;
		this->draws += other.draws;
		this->circle_win += other.circle_win;
		this->game_length += other.game_length;
		return *this;
	}
	std::string GameDataBufferStats::toString() const
	{
		std::string result;
		result += "----GameBufferStats----\n";
		result += "games   = " + std::to_string(games) + '\n';
		result += "samples = " + std::to_string(samples) + '\n';
		result += "cross   = " + std::to_string(cross_win) + '\n';
		result += "draws   = " + std::to_string(draws) + '\n';
		result += "circle  = " + std::to_string(circle_win) + '\n';
		result += "avg len = " + std::to_string((float) game_length / games) + '\n';
		return result;
	}

	GameDataBuffer::GameDataBuffer(GameConfig cfg) noexcept :
			game_config(cfg)
	{
	}
	GameDataBuffer::GameDataBuffer(const std::string &path)
	{
		load(path);
	}
	const GameConfig& GameDataBuffer::getConfig() const noexcept
	{
		return game_config;
	}
	void GameDataBuffer::clear() noexcept
	{
		buffer_data.clear();
	}
	int GameDataBuffer::size() const noexcept
	{
		return buffer_data.size();
	}
	int GameDataBuffer::numberOfSamples() const noexcept
	{
		int result = 0;
		for (size_t i = 0; i < buffer_data.size(); i++)
			result += buffer_data[i].numberOfSamples();
		return result;
	}
	void GameDataBuffer::append(const GameDataBuffer &other)
	{
		this->buffer_data.insert(this->buffer_data.end(), other.buffer_data.begin(), other.buffer_data.end());
	}
	void GameDataBuffer::addGameData(const GameDataStorage &game)
	{
		buffer_data.push_back(game);
	}
	const GameDataStorage& GameDataBuffer::getGameData(int index) const
	{
		return buffer_data.at(index);
	}
	GameDataStorage& GameDataBuffer::getGameData(int index)
	{
		return buffer_data.at(index);
	}
	void GameDataBuffer::removeFromBuffer(int index)
	{
		buffer_data.erase(buffer_data.begin() + index);
	}
	void GameDataBuffer::removeRange(int from, int to)
	{
		buffer_data.erase(buffer_data.begin() + from, buffer_data.begin() + to);
	}
	void GameDataBuffer::save(const std::string &path) const
	{
		Json json(JsonType::Object);
		json["config"] = game_config.toJson();
		json["offsets"] = Json(JsonType::Array);
		SerializedObject so;
		Json &list_of_offsets = json["offsets"];
		for (size_t i = 0; i < buffer_data.size(); i++)
		{
			list_of_offsets[i] = so.size(); // saving offset
			buffer_data[i].serialize(so);
		}

		FileSaver fs(path);
		fs.save(json, so, -1, true);
		fs.close();
	}
	void GameDataBuffer::load(const std::string &path)
	{
		FileLoader fl(path, true);

		game_config = GameConfig(fl.getJson()["config"]);

		const Json &list_of_offsets = fl.getJson()["offsets"];
		const size_t size = list_of_offsets.size();

		for (size_t i = 0; i < size; i++)
		{
			size_t offset = list_of_offsets[i].getLong();
			buffer_data.push_back(GameDataStorage(fl.getBinaryData(), offset));
		}
	}
	GameDataBufferStats GameDataBuffer::getStats() const noexcept
	{
		GameDataBufferStats stats;
		stats.games = buffer_data.size();
		for (size_t i = 0; i < buffer_data.size(); i++)
		{
			stats.samples += buffer_data[i].numberOfSamples();
			stats.game_length += buffer_data[i].numberOfMoves();
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

} /* namespace ag */

