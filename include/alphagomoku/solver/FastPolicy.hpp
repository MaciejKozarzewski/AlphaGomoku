/*
 * FastPolicy.hpp
 *
 *  Created on: Apr 16, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FASTPOLICY_HPP_
#define INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FASTPOLICY_HPP_

#include <alphagomoku/solver/PatternCalculator.hpp>

#include <fstream>
#include <cassert>

namespace ag::experimental
{
	struct Weight
	{
			uint8_t for_cross;
			uint8_t for_circle;
	};

	class WeightTable
	{
		private:
			std::vector<Weight> freestyle_weights;
			std::vector<Weight> standard_weights;
			std::vector<Weight> renju_weights;
			std::vector<Weight> caro_weights;

			WeightTable(const std::string &path);
			size_t load(const std::vector<Weight> &loaded, size_t offset, GameRules rules);
			std::vector<Weight>& internal_get(GameRules rules);
		public:
			static const std::vector<Weight>& get(GameRules rules);
			static void combineAndStore(const std::string &path);
	};

	class FastPolicy
	{
		private:
			GameConfig game_config;
			PatternCalculator pattern_extractor;
			std::vector<float> feature_weights_cross;
			std::vector<float> feature_weights_circle;

			float learning_rate = 0.01f;
		public:
			FastPolicy(GameConfig gameConfig);

			std::vector<float> learn(const matrix<Sign> &board, Sign signToMove, const matrix<float> &target);
			matrix<float> forward(const matrix<Sign> &board, Sign signToMove);

			void setLearningRate(float lr) noexcept;

			void dump(const std::string &path);
	};

	class FastOrderingPolicy
	{
		private:
			struct SpotValue
			{
					uint8_t for_cross[4];
					uint8_t for_circle[4];
			};
			PatternCalculator &pattern_extractor;
			const std::vector<Weight> &feature_weights;
			GameConfig config;
			int pad;
			matrix<SpotValue> spot_values;

		public:
			FastOrderingPolicy(PatternCalculator &calculator, GameConfig cfg);
			void init();
			void update(Move move);
			uint32_t getValue(Move m) const noexcept
			{
				assert(m.sign == Sign::CROSS || m.sign == Sign::CIRCLE);
				assert(m.row >= 0 && m.row < config.rows);
				assert(m.col >= 0 && m.col < config.cols);

				const SpotValue sv = spot_values.at(pad + m.row, pad + m.col);
				const uint8_t *tmp = (m.sign == Sign::CROSS) ? sv.for_cross : sv.for_circle;
				return static_cast<uint32_t>(tmp[0]) + static_cast<uint32_t>(tmp[1]) + static_cast<uint32_t>(tmp[2]) + static_cast<uint32_t>(tmp[3]);
			}
		private:
			void internal_update(int row, int col, Direction dir) noexcept;
	};

} /* namespace ag */

#endif /* INCLUDE_ALPHAGOMOKU_VCF_SOLVER_FASTPOLICY_HPP_ */
