/*
 * Sampler.hpp
 *
 *  Created on: Feb 24, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_DATASET_SAMPLER_HPP_
#define ALPHAGOMOKU_DATASET_SAMPLER_HPP_

#include <alphagomoku/dataset/data_packs.hpp>

#include <vector>
#include <cstddef>

namespace ag
{
	class GameDataBuffer;
	class GameDataStorage;
}

namespace ag
{

	class Sampler
	{
			struct SamplePosition
			{
					int game_index = 0;
					int sample_index = 0;
					SamplePosition() noexcept = default;
					SamplePosition(int gi, int si) noexcept :
							game_index(gi),
							sample_index(si)
					{
					}
			};

			const GameDataBuffer &buffer;
			SearchDataPack search_data_pack;
			std::vector<int> game_ordering;
			size_t counter = 0;
			int batch_size;
		public:
			Sampler(const GameDataBuffer &buffer, int batchSize);
			void get(TrainingDataPack &result);
		private:
			void reset();
			std::vector<SamplePosition> shuffle_dataset(size_t batch_size);
			void prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample);
			void prepare_training_data_old(TrainingDataPack &result, const SearchDataPack &sample);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_SAMPLER_HPP_ */
