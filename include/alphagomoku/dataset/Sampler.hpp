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
#include <memory>
#include <cstddef>

namespace ag
{
	class Dataset;
	class GameDataStorage;
	struct GameConfig;
}

namespace ag
{
	class Sampler
	{
			const Dataset *dataset = nullptr;
			SearchDataPack search_data_pack;
			std::vector<std::pair<int, int>> buffer_and_game_ordering;
			size_t counter = 0;
			int batch_size = 0;
		public:
			Sampler() noexcept = default;
			Sampler(const Sampler &other) = delete;
			Sampler(Sampler &&other) = delete;
			Sampler& operator=(const Sampler &other) = delete;
			Sampler& operator=(Sampler &&other) = delete;
			virtual ~Sampler() = default;
			virtual void init(const Dataset &dataset, int batchSize);
			virtual void get(TrainingDataPack &result);
		private:
			virtual void prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample) = 0;
	};

	class SamplerVisits: public Sampler
	{
		public:
			SamplerVisits() = default;
		private:
			void prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample);
	};

	class SamplerValues: public Sampler
	{
		public:
			SamplerValues() = default;
		private:
			void prepare_training_data(TrainingDataPack &result, const SearchDataPack &sample);
	};

	std::unique_ptr<Sampler> createSampler(const std::string &name);

} /* namespace ag */

#endif /* ALPHAGOMOKU_DATASET_SAMPLER_HPP_ */
