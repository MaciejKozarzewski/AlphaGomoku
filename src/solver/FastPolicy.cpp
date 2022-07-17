/*
 * FastPolicy.cpp
 *
 *  Created on: Apr 16, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/solver/FastPolicy.hpp>
#include <alphagomoku/solver/Pattern.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <filesystem>

namespace
{
	using namespace ag;
	using namespace ag::experimental;

	std::vector<Weight> load_file(const std::string &path)
	{
		try
		{
			const size_t size_in_bytes = std::filesystem::file_size(path);
			std::vector<Weight> result(size_in_bytes / sizeof(Weight));
			std::fstream file(path, std::ios::in | std::ios::binary);
			file.read(reinterpret_cast<char*>(result.data()), size_in_bytes);
			return result;
		} catch (std::exception &e)
		{
			std::cerr << e.what() << '\n';
			Logger::write(std::string(e.what()) + " - not using fast policy");
		}
		return std::vector<Weight>();
	}
}

namespace ag::experimental
{

	WeightTable::WeightTable(const std::string &path)
	{
		const std::vector<Weight> loaded = load_file(path);
		size_t offset = load(loaded, 0, GameRules::FREESTYLE);
		offset = load(loaded, offset, GameRules::STANDARD);
		offset = load(loaded, offset, GameRules::RENJU);
		offset = load(loaded, offset, GameRules::CARO);
	}
	size_t WeightTable::load(const std::vector<Weight> &loaded, size_t offset, GameRules rules)
	{
		std::vector<Weight> &dst = internal_get(rules);
		dst = std::vector<Weight>(power(4, Pattern::length(rules)));
		if (loaded.size() > 0)
			for (size_t i = 0; i < dst.size(); i++)
			{
				Pattern p(Pattern::length(rules), i);
				if (p.isValid())
				{
					dst[i] = loaded[offset];
					offset++;
				}
			}
		return offset;
	}
	std::vector<Weight>& WeightTable::internal_get(GameRules rules)
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
				return freestyle_weights;
			case GameRules::STANDARD:
				return standard_weights;
			case GameRules::RENJU:
				return renju_weights;
			case GameRules::CARO:
				return caro_weights;
			default:
				throw std::logic_error("Unknown rule");
		}
	}
	const std::vector<Weight>& WeightTable::get(GameRules rules)
	{
		static WeightTable table("/home/maciek/Desktop/fast_policy.bin"); // FIXME add proper path relative to the executable
		return table.internal_get(rules);
	}
	void WeightTable::combineAndStore(const std::string &path)
	{
		std::vector<Weight> freestyle = load_file(path + "/freestyle.bin");
		std::vector<Weight> standard = load_file(path + "/standard.bin");
		std::vector<Weight> renju = load_file(path + "/renju.bin");
		std::vector<Weight> caro = load_file(path + "/caro.bin");

		std::vector<Weight> combined;
		combined.insert(combined.end(), freestyle.begin(), freestyle.end());
		combined.insert(combined.end(), standard.begin(), standard.end());
		combined.insert(combined.end(), renju.begin(), renju.end());
		combined.insert(combined.end(), caro.begin(), caro.end());

		std::fstream file(path + "/fast_policy.bin", std::ios::out | std::ios::binary);
		file.write(reinterpret_cast<char*>(combined.data()), sizeof(Weight) * combined.size());
	}

	FastPolicy::FastPolicy(GameConfig gameConfig) :
			game_config(gameConfig),
			pattern_extractor(gameConfig),
			feature_weights_cross(power(4, Pattern::length(gameConfig.rules))),
			feature_weights_circle(feature_weights_cross.size())
	{
		for (size_t i = 0; i < feature_weights_cross.size(); i++)
		{
			feature_weights_cross[i] = randFloat();
			feature_weights_circle[i] = randFloat();
		}
	}
	std::vector<float> FastPolicy::learn(const matrix<Sign> &board, Sign signToMove, const matrix<float> &target)
	{
		std::vector<float> &feature_weights = (signToMove == Sign::CROSS) ? feature_weights_cross : feature_weights_circle;

		matrix<float> policy = forward(board, signToMove);

		// backward and update pass
		for (int r = 0; r < policy.rows(); r++)
			for (int c = 0; c < policy.cols(); c++)
			{
				const float gradient = target.at(r, c) - policy.at(r, c);
				for (int dir = 0; dir < 4; dir++)
				{
					const uint32_t raw_feature = pattern_extractor.getRawFeatureAt(r, c, dir);
					feature_weights[raw_feature] += learning_rate * gradient;
				}
			}

		// loss and metrics
		std::vector<float> result(1 + 5);

		result[0] = cross_entropy(target.begin(), target.end(), policy.begin()) - cross_entropy(target.begin(), target.end(), target.begin());
		Move correct = pickMove(target);
		for (int l = 0; l < 5; l++)
		{
			Move best = pickMove(policy);
			if (correct == best)
				for (int m = l; m < 5; m++)
					result.at(1 + m) += 1;
			policy.at(best.row, best.col) = 0.0f;
		}
		return result;
	}
	matrix<float> FastPolicy::forward(const matrix<Sign> &board, Sign signToMove)
	{
		const std::vector<float> &feature_weights = (signToMove == Sign::CROSS) ? feature_weights_cross : feature_weights_circle;
		pattern_extractor.setBoard(board);

		matrix<float> policy(board.rows(), board.cols());

		// forward pass
		for (int r = 0; r < policy.rows(); r++)
			for (int c = 0; c < policy.cols(); c++)
			{
				float tmp = 0.0f;
				for (int dir = 0; dir < 4; dir++)
					tmp += feature_weights[pattern_extractor.getRawFeatureAt(r, c, dir)];
				policy.at(r, c) = tmp;
			}

		// softmax
		float max_value = std::numeric_limits<float>::lowest();
		for (int i = 0; i < policy.size(); i++)
			max_value = std::max(max_value, policy[i]);

		float sum = 0.0f;
		for (int i = 0; i < policy.size(); i++)
		{
			policy[i] = std::exp(policy[i] - max_value);
			sum += policy[i];
		}
		if (sum == 0.0f)
			policy.fill(1.0f / policy.size());
		else
		{
			sum = 1.0f / sum;
			for (int i = 0; i < policy.size(); i++)
				policy[i] *= sum;
		}
		return policy;
	}
	void FastPolicy::setLearningRate(float lr) noexcept
	{
		learning_rate = lr;
	}
	void FastPolicy::dump(const std::string &path)
	{
		float min_value = std::numeric_limits<float>::max();
		float max_value = std::numeric_limits<float>::lowest();

		for (size_t i = 0; i < feature_weights_circle.size(); i++)
		{
			min_value = std::min(min_value, std::min(feature_weights_cross[i], feature_weights_circle[i]));
			max_value = std::max(max_value, std::max(feature_weights_cross[i], feature_weights_circle[i]));
		}
		const float scale = 255.0f / (max_value - min_value);
		std::cout << min_value << " " << max_value << '\n';

		std::vector<Weight> weights;
		for (size_t i = 0; i < feature_weights_circle.size(); i++)
		{
			Pattern p(Pattern::length(game_config.rules), i);
			if (p.isValid())
			{
				p.flip(); // ensure that weights for symmetrical features are exactly the same
				const size_t flipped_index = p.encode();
				const float cross = (feature_weights_cross[i] + feature_weights_cross[flipped_index]) / 2;
				const float circle = (feature_weights_circle[i] + feature_weights_circle[flipped_index]) / 2;

				Weight w { static_cast<uint8_t>((cross + min_value) * scale), static_cast<uint8_t>((circle + min_value) * scale) };
				weights.push_back(w);
			}
		}

		std::fstream file(path, std::ios::out | std::ios::binary);
		file.write(reinterpret_cast<char*>(weights.data()), sizeof(Weight) * weights.size());
	}

	FastOrderingPolicy::FastOrderingPolicy(PatternCalculator &calculator, GameConfig cfg) :
			pattern_extractor(calculator),
			feature_weights(WeightTable::get(cfg.rules)),
			config(cfg),
			pad(Pattern::length(cfg.rules) / 2),
			spot_values(2 * pad + cfg.rows, 2 * pad + cfg.cols)
	{
	}
	void FastOrderingPolicy::init()
	{
		for (int r = 0; r < config.rows; r++)
			for (int c = 0; c < config.cols; c++)
				for (int dir = 0; dir < 4; dir++)
					internal_update(r, c, dir);
	}
	void FastOrderingPolicy::update(Move move)
	{
		assert(move.row >= 0 && move.row < config.rows);
		assert(move.col >= 0 && move.col < config.cols);
		for (int i = -pad; i <= pad; i++)
		{
			internal_update(move.row, move.col + i, HORIZONTAL);
			internal_update(move.row + i, move.col, VERTICAL);
			internal_update(move.row + i, move.col + i, DIAGONAL);
			internal_update(move.row + i, move.col - i, ANTIDIAGONAL);
		}
	}
	/*
	 * private
	 */
	void FastOrderingPolicy::internal_update(int row, int col, Direction dir) noexcept
	{
		assert(dir >= HORIZONTAL && dir <= ANTIDIAGONAL);
		const uint32_t pattern = pattern_extractor.getRawFeatureAt(row, col, dir);
		spot_values.at(pad + row, pad + col).for_cross[dir] = feature_weights[pattern].for_cross;
		spot_values.at(pad + row, pad + col).for_circle[dir] = feature_weights[pattern].for_circle;
	}

} /* namespace ag */

