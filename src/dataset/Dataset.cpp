/*
 * Dataset.cpp
 *
 *  Created on: May 29, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/Dataset.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>

#include <iostream>

namespace ag
{

	GameDataBufferStats Dataset::getStats() const noexcept
	{
		GameDataBufferStats stats;
		for (auto iter = m_list_of_buffers.begin(); iter != m_list_of_buffers.end(); iter++)
			stats += iter->second.getStats();
		return stats;
	}
	int Dataset::numberOfBuffers() const noexcept
	{
		return m_list_of_buffers.size();
	}
	int Dataset::numberOfGames() const noexcept
	{
		int result = 0;
		for (auto iter = m_list_of_buffers.begin(); iter != m_list_of_buffers.end(); iter++)
			result += iter->second.numberOfGames();
		return result;
	}
	int Dataset::numberOfSamples() const noexcept
	{
		int result = 0;
		for (auto iter = m_list_of_buffers.begin(); iter != m_list_of_buffers.end(); iter++)
			result += iter->second.numberOfSamples();
		return result;
	}
	bool Dataset::isLoaded(int i) const noexcept
	{
		return m_list_of_buffers.find(i) != m_list_of_buffers.end();
	}
	void Dataset::load(int i, const std::string &path)
	{
		if (not isLoaded(i))
			m_list_of_buffers.insert( { i, GameDataBuffer(path) });
	}
	void Dataset::unload(int i)
	{
		if (isLoaded(i))
			m_list_of_buffers.erase(i);
	}
	void Dataset::clear()
	{
		m_list_of_buffers.clear();
	}
	const GameDataBuffer& Dataset::getBuffer(int i) const
	{
		auto iter = m_list_of_buffers.find(i);
		if (iter != m_list_of_buffers.end())
			return iter->second;
		else
			throw std::runtime_error("No buffer with index " + std::to_string(i));
	}
	std::vector<int> Dataset::getListOfBuffers() const
	{
		std::vector<int> result;
		for (auto iter = m_list_of_buffers.begin(); iter != m_list_of_buffers.end(); iter++)
			result.push_back(iter->first);
		return result;
	}

} /* namespace ag */

