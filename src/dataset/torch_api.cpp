/*
 * torch_api.cpp
 *
 *  Created on: Nov 14, 2024
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/torch_api.h>
#include <alphagomoku/dataset/Dataset.hpp>
#include <alphagomoku/dataset/Sampler.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>

#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/networks/NNInputFeatures.hpp>

#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/utils/augmentations.hpp>

#include <cassert>

namespace
{
	using namespace ag;

	Dataset& get_dataset()
	{
		static Dataset dataset;
		return dataset;
	}

	std::vector<std::pair<int, int>> list_all_games(const Dataset &dataset)
	{
		std::vector<std::pair<int, int>> result;

		const std::vector<int> list_of_buffers = dataset.getListOfBuffers();
		for (size_t i = 0; i < list_of_buffers.size(); i++)
		{
			const GameDataBuffer &buffer = dataset.getBuffer(list_of_buffers[i]);
			for (int j = 0; j < buffer.numberOfGames(); j++)
				result.push_back( { list_of_buffers[i], j });
		}
		return result;
	}

	class BaseSampler
	{
			const Dataset *dataset = nullptr;
			std::vector<std::pair<int, int>> buffer_and_game_ordering;
			size_t counter = 0;
			std::mutex sampler_mutex;
		public:
			void init(const Dataset &dataset)
			{
				std::lock_guard<std::mutex> lock(sampler_mutex);
				this->dataset = &dataset;
				buffer_and_game_ordering = list_all_games(dataset);
				std::random_shuffle(buffer_and_game_ordering.begin(), buffer_and_game_ordering.end());
			}
			Sample_t pick_sample()
			{
				std::lock_guard<std::mutex> lock(sampler_mutex);
				if (dataset == nullptr)
					throw std::logic_error("Dataset is null. Most likely this Sampler was not initialized");

				const int buffer_index = buffer_and_game_ordering.at(counter).first;
				const int game_index = buffer_and_game_ordering.at(counter).second;
				const int sample_index = randInt(dataset->getBuffer(buffer_index).getGameData(game_index).numberOfSamples());

				counter++;
				if (counter >= buffer_and_game_ordering.size())
				{
					std::random_shuffle(buffer_and_game_ordering.begin(), buffer_and_game_ordering.end());
					counter = 0;
				}
				return Sample_t { buffer_index, game_index, sample_index };
			}
			GameConfig get_config(const Sample_t &sample) const
			{
				return dataset->getBuffer(sample.buffer_index).getConfig();
			}
			SearchDataPack load_data(const Sample_t &sample) const
			{
				const GameConfig cfg = dataset->getBuffer(sample.buffer_index).getConfig();
				SearchDataPack result(cfg.rows, cfg.cols);
				dataset->getBuffer(sample.buffer_index).getGameData(sample.game_index).getSample(result, sample.sample_index);

				const int r = randInt(available_symmetries(result.board));
				ag::augment(result.board, r);
				ag::augment(result.visit_count, r);
				ag::augment(result.policy_prior, r);
				ag::augment(result.action_values, r);
				ag::augment(result.action_scores, r);
				return result;
			}
	};

	BaseSampler& get_sampler()
	{
		static BaseSampler sampler;
		return sampler;
	}

	void fill_tensor_size(TensorSize_t *ts, std::initializer_list<int> dimensions) noexcept
	{
		assert(ts != nullptr);
		ts->rank = dimensions.size();
		for (int i = 0; i < ts->rank; i++)
			ts->dim[i] = dimensions.begin()[i];
	}

	bool is_same_config(const GameConfig &lhs, const GameConfig &rhs) noexcept
	{
		return lhs.rules == rhs.rules and lhs.rows == rhs.rows and lhs.cols == rhs.cols;
	}
	std::string to_string(const GameConfig &cfg)
	{
		return toString(cfg.rules) + ":" + std::to_string(cfg.rows) + "x" + std::to_string(cfg.cols);
	}
}

namespace ag
{
	void load_dataset_fragment(int i, const char *path)
	{
		assert(path != nullptr);
		get_dataset().load(i, path);
	}
	void unload_dataset_fragment(int i)
	{
		get_dataset().unload(i);
	}
	void get_dataset_size(TensorSize_t *shape, int *size)
	{
		if (shape != nullptr)
		{
			assert(size == nullptr);
			fill_tensor_size(shape, { get_dataset().numberOfGames(), 4 });
			return;
		}
		assert(shape == nullptr);

		size_t idx = 0;
		const std::vector<int> list_of_buffers = get_dataset().getListOfBuffers();
		for (size_t i = 0; i < list_of_buffers.size(); i++)
		{
			const GameDataBuffer &buffer = get_dataset().getBuffer(list_of_buffers[i]);
			for (int j = 0; j < buffer.numberOfGames(); j++)
			{
				size[idx + 0] = list_of_buffers[i];
				size[idx + 1] = j;
				size[idx + 2] = buffer.getGameData(j).numberOfSamples();
				size[idx + 3] = available_symmetries(buffer.getConfig().rows, buffer.getConfig().cols);
				idx += 4;
			}
		}
	}
	void print_dataset_info()
	{
		std::cout << get_dataset().getStats().toString() << '\n';
	}

	void get_tensor_sizes(Sample_t sample, TensorSize_t *input, TensorSize_t *visits, TensorSize_t *policy_prior, TensorSize_t *value_target,
			TensorSize_t *minimax_value, TensorSize_t *minimax_score, TensorSize_t *moves_left, TensorSize_t *action_values,
			TensorSize_t *action_scores)
	{
		const GameConfig cfg = get_dataset().getBuffer(sample.buffer_index).getConfig();
		const int rows = cfg.rows;
		const int columns = cfg.cols;

		fill_tensor_size(input, { rows, columns, 32 });
		fill_tensor_size(visits, { rows, columns, 1 });
		fill_tensor_size(policy_prior, { rows, columns, 1 });
		fill_tensor_size(value_target, { 3 });
		fill_tensor_size(minimax_value, { 3 });
		fill_tensor_size(minimax_score, { 1 });
		fill_tensor_size(moves_left, { 1 });
		fill_tensor_size(action_values, { rows, columns, 3 });
		fill_tensor_size(action_scores, { rows, columns, 1 });
	}
	void load_data(Sample_t sample, float *input, float *visits, float *policy_prior, float *value_target, float *minimax_value, int *minimax_score,
			int *moves_left, float *action_values, int *action_scores)
	{
		assert(input != nullptr);
		assert(visits != nullptr);
		assert(policy_prior != nullptr);
		assert(value_target != nullptr);
		assert(minimax_value != nullptr);
		assert(minimax_score != nullptr);
		assert(moves_left != nullptr);
		assert(action_values != nullptr);
		assert(action_scores != nullptr);

		const GameConfig cfg = get_dataset().getBuffer(sample.buffer_index).getConfig();

		SearchDataPack data(cfg.rows, cfg.cols);
		get_dataset().getBuffer(sample.buffer_index).getGameData(sample.game_index).getSample(data, sample.sample_index);

		assert(sample.augmentation < available_symmetries(data.board));
		ag::augment(data.board, sample.augmentation);
		ag::augment(data.visit_count, sample.augmentation);
		ag::augment(data.policy_prior, sample.augmentation);
		ag::augment(data.action_values, sample.augmentation);
		ag::augment(data.action_scores, sample.augmentation);

		const int rows = cfg.rows;
		const int columns = cfg.cols;

		PatternCalculator calc(cfg);
		calc.setBoard(data.board, data.played_move.sign);
		NNInputFeatures features(rows, columns);
		features.encode(calc);

		const Value qt = convertOutcome(data.game_outcome, data.played_move.sign);
		value_target[0] = qt.win_rate;
		value_target[1] = qt.draw_rate;
		value_target[2] = qt.loss_rate();

		minimax_value[0] = data.minimax_value.win_rate;
		minimax_value[1] = data.minimax_value.draw_rate;
		minimax_value[2] = data.minimax_value.loss_rate();

		minimax_score[0] = Score::to_short(data.minimax_score);

		moves_left[0] = data.moves_left;

		for (int i = 0; i < rows * columns; i++)
		{
			uint32_t f = features[i];
			for (int j = 0; j < 32; j++)
			{
				input[i * 32 + j] = (f & 1u) ? 1.0f : 0.0f;
				f = (f >> 1u);
			}
			visits[i] = static_cast<float>(data.visit_count[i]);
			policy_prior[i] = data.policy_prior[i];

			const Value q = data.action_values[i];
			action_values[i * 3 + 0] = q.win_rate;
			action_values[i * 3 + 1] = q.draw_rate;
			action_values[i * 3 + 2] = q.loss_rate();

			action_scores[i] = Score::to_short(data.action_scores[i]);
		}
	}

	void get_tensor_shapes(int batch_size, const Sample_t *samples, TensorSize_t *input, TensorSize_t *policy_target, TensorSize_t *value_target,
			TensorSize_t *moves_left_target, TensorSize_t *action_values_target)
	{
		if (batch_size <= 0)
			return;
		const GameConfig cfg = get_dataset().getBuffer(samples[0].buffer_index).getConfig();
		for (int b = 1; b < batch_size; b++)
		{
			const GameConfig c = get_dataset().getBuffer(samples[b].buffer_index).getConfig();
			if (not is_same_config(c, cfg))
				throw std::runtime_error("GameConfig mismatch at " + std::to_string(b) + ", expected " + to_string(cfg) + ", got " + to_string(c));
		}

		const int rows = cfg.rows;
		const int columns = cfg.cols;

		fill_tensor_size(input, { batch_size, rows, columns, 32 });
		fill_tensor_size(policy_target, { batch_size, rows, columns, 1 });
		fill_tensor_size(value_target, { batch_size, 3 });
		fill_tensor_size(moves_left_target, { batch_size, 1 });
		fill_tensor_size(action_values_target, { batch_size, rows, columns, 3 });
	}
	void load_multiple_samples(int batch_size, const Sample_t *samples, float *input, float *policy_target, float *value_target,
			float *moves_left_target, float *action_values_target)
	{
		assert(samples != nullptr);
		assert(input != nullptr);
		assert(policy_target != nullptr);
		assert(value_target != nullptr);
		assert(moves_left_target != nullptr);
		assert(action_values_target != nullptr);

		if (batch_size <= 0)
			return;
		const GameConfig cfg = get_dataset().getBuffer(samples[0].buffer_index).getConfig();
		for (int b = 1; b < batch_size; b++)
		{
			const GameConfig c = get_dataset().getBuffer(samples[b].buffer_index).getConfig();
			if (not is_same_config(c, cfg))
				throw std::runtime_error("GameConfig mismatch at " + std::to_string(b) + ", expected " + to_string(cfg) + ", got " + to_string(c));
		}

		const int rows = cfg.rows;
		const int columns = cfg.cols;
		SearchDataPack pack(rows, columns);
		PatternCalculator calc(cfg);
		NNInputFeatures features(rows, columns);

		for (int b = 0; b < batch_size; b++)
		{
			const Sample_t sample = samples[b];

			get_dataset().getBuffer(sample.buffer_index).getGameData(sample.game_index).getSample(pack, sample.sample_index);

			assert(sample.augmentation < available_symmetries(pack.board));
			ag::augment(pack.board, sample.augmentation);
			ag::augment(pack.visit_count, sample.augmentation);
			ag::augment(pack.policy_prior, sample.augmentation);
			ag::augment(pack.action_values, sample.augmentation);
			ag::augment(pack.action_scores, sample.augmentation);

			calc.setBoard(pack.board, pack.played_move.sign);
			features.encode(calc);

			const Value qt = convertOutcome(pack.game_outcome, pack.played_move.sign);
			value_target[0] = qt.win_rate;
			value_target[1] = qt.draw_rate;
			value_target[2] = qt.loss_rate();

			moves_left_target[0] = pack.moves_left;

			float policy_sum = 0.0f;
			for (int i = 0; i < rows * columns; i++)
			{
				uint32_t f = features[i];
				for (int j = 0; j < 32; j++)
				{
					input[i * 32 + j] = (f & 1u) ? 1.0f : 0.0f;
					f = (f >> 1u);
				}

				const Score score = pack.action_scores[i];
				const Value value = score.isProven() ? score.convertToValue() : pack.action_values[i];

				action_values_target[i * 3 + 0] = value.win_rate;
				action_values_target[i * 3 + 1] = value.draw_rate;
				action_values_target[i * 3 + 2] = value.loss_rate();

				switch (score.getProvenValue())
				{
					case ProvenValue::LOSS:
						policy_target[i] = 1.0e-6f;
						break;
					case ProvenValue::DRAW:
						policy_target[i] = std::max(1, pack.visit_count[i]);
						break;
					default:
					case ProvenValue::UNKNOWN:
						policy_target[i] = pack.visit_count[i];
						break;
					case ProvenValue::WIN:
						policy_target[i] = 1.0e+6f;
						break;
				}
				policy_sum += policy_target[i];
			}
			const float tmp = 1.0f / policy_sum;
			for (int i = 0; i < rows * columns; i++)
				policy_target[i] *= tmp;

			input += 32 * rows * columns;
			policy_target += rows * columns;
			value_target += 3;
			moves_left_target += 1;
		}
	}

} /* namespace ag */
