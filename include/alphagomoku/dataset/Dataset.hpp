/*
 * Dataset.hpp
 *
 *  Created on: May 29, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_DATASET_HPP_
#define ALPHAGOMOKU_DATASET_DATASET_HPP_

#include <alphagomoku/dataset/GameDataBuffer.hpp>

#include <map>

namespace ag
{

	class Dataset
	{
			std::map<int, GameDataBuffer> m_list_of_buffers;
		public:
			GameDataBufferStats getStats() const noexcept;
			int numberOfBuffers() const noexcept;
			int numberOfGames() const noexcept;
			int numberOfSamples() const noexcept;
			bool isLoaded(int i) const noexcept;
			void load(int i, const std::string &path);
			void unload(int i);
			void clear();
			const GameDataBuffer& getBuffer(int i) const;
			std::vector<int> getListOfBuffers() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_DATASET_HPP_ */
