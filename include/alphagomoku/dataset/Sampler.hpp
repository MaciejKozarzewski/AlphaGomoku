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
			const GameDataBuffer &buffer;
			SearchDataPack search_data_pack;
			std::vector<int> sample_order;
			std::vector<int> sampling_stats;
			int sample_counter = 0;
		public:
			Sampler(const GameDataBuffer &buffer);
			void get(TrainingDataPack &result);
		private:
			void reset();
			int pick_move_number(const GameDataStorage &game);
			void prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample);
			void prepare_training_data_old(TrainingDataPack &result, const SearchDataPack &sample);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_SAMPLER_HPP_ */
