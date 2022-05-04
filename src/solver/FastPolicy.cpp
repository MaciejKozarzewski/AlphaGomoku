/*
 * FastPolicy.cpp
 *
 *  Created on: Apr 16, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/solver/FastPolicy.hpp>
#include <alphagomoku/utils/misc.hpp>

namespace
{
	using namespace ag;

	matrix<Sign> invert_board(const matrix<Sign> &board)
	{
		matrix<Sign> result(board);
		for (int i = 0; i < result.size(); i++)
			result[i] = invertSign(result[i]);
		return result;
	}
	int number_of_features(GameRules rules) noexcept
	{
		switch (rules)
		{
			case GameRules::FREESTYLE:
				return 1 << 18;
			case GameRules::STANDARD:
			case GameRules::RENJU:
			case GameRules::CARO:
				return 1 << 22;
			default:
				return 0;
		}
	}
}

namespace ag::experimental
{
	FastPolicy::FastPolicy(GameConfig gameConfig) :
			pattern_extractor(gameConfig),
			feature_weights(number_of_features(gameConfig.rules)),
			threat_weights(9),
			bias(gameConfig.rows, gameConfig.cols)
	{
		for (size_t i = 0; i < feature_weights.size(); i++)
			feature_weights[i] = randFloat();
		for (size_t i = 0; i < threat_weights.size(); i++)
			threat_weights[i] = randFloat();
		for (int i = 0; i < bias.size(); i++)
			bias[i] = randFloat();
	}
	std::vector<float> FastPolicy::getAccuracy(int top_k) const
	{
		return std::vector<float>();
	}
	void FastPolicy::learn(const matrix<Sign> &board, Sign signToMove, const matrix<float> &target)
	{
//		if (signToMove == Sign::CROSS)
//			pattern_extractor.setBoard(board, signToMove);
//		else
//			pattern_extractor.setBoard(invert_board(board), invertSign(signToMove));
	}
} /* namespace ag */

