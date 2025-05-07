/*
 * NetworkDataPack.hpp
 *
 *  Created on: Feb 21, 2025
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_NETWORKDATAPACK_HPP_
#define ALPHAGOMOKU_NETWORKS_NETWORKDATAPACK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/networks/NNInputFeatures.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/configs.hpp>

#include <minml/graph/Graph.hpp>

#include <memory>
#include <vector>

namespace ml
{
	class Tensor;
	enum class DataType
	;
} /* namespace ml */

namespace ag
{
	struct Value;
	class PatternCalculator;
} /* namespace ag */

namespace ag
{
	class NetworkDataPack
	{
			GameConfig game_config;
			ml::Context context_on_cpu;

			ml::Tensor input_on_cpu;
			std::vector<ml::Tensor> outputs_on_cpu;
			std::vector<ml::Tensor> targets_on_cpu;
			std::vector<ml::Tensor> masks_on_cpu;

			std::unique_ptr<PatternCalculator> pattern_calculator; // lazily allocated on first use
			NNInputFeatures input_features; // same as above
			mutable std::vector<float3> workspace;
		public:
			NetworkDataPack() = default;
			NetworkDataPack(const GameConfig &cfg, int batchSize, ml::DataType dtype);

			void packInputData(int index, const matrix<Sign> &board, Sign signToMove);
			/*
			 * \brief Can be used to pack the data if the features were already calculated.
			 */
			void packInputData(int index, const NNInputFeatures &features);

			void packPolicyTarget(int index, const matrix<float> &target);
			void packValueTarget(int index, Value target);
			void packActionValuesTarget(int index, const matrix<Value> &target, const matrix<float> &mask);
			void packMovesLeftTarget(int index, int target);

			void unpackPolicy(int index, matrix<float> &policy) const;
			void unpackValue(int index, Value &value) const;
			void unpackActionValues(int index, matrix<Value> &actionValues) const;
			void unpackMovesLeft(int index, float &movesLeft) const;

			void pinMemory();

			ml::Tensor& getInput();
			const ml::Tensor& getInput() const;

			ml::Tensor& getOutput(char c);
			const ml::Tensor& getOutput(char c) const;
			ml::Tensor& getTarget(char c);
			const ml::Tensor& getTarget(char c) const;
			ml::Tensor& getMask(char c);
			const ml::Tensor& getMask(char c) const;

			GameConfig getGameConfig() const noexcept;
			int getBatchSize() const noexcept;
		private:
			PatternCalculator& get_pattern_calculator();
			void allocate_target_tensors();
			void allocate_mask_tensors();
	};

	std::vector<float> getAccuracy(int batchSize, const NetworkDataPack &pack, int top_k);

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NETWORKDATAPACK_HPP_ */
