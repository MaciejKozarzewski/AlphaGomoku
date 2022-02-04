/*
 * EdgeGenerator.hpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_

#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <memory>
#include <vector>

namespace ag
{
	class Edge;
	class SearchTask;
}

namespace ag
{
	enum class ExclusionMode
	{
		NONE,
		SYMMETRIC,
		INNER_SQUARE,
		OUTSIDE_SQUARE
	};

	struct ExclusionConfig
	{
			ExclusionMode mode = ExclusionMode::NONE;
			int param = 0;
	};

	class EdgeGenerator
	{
		private:
			GameRules game_rules;
			float policy_threshold;
			int max_edges;
		public:
			EdgeGenerator(GameRules rules, float policyThreshold, int maxEdges, ExclusionConfig cfg = ExclusionConfig());

			EdgeGenerator* clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_ */
