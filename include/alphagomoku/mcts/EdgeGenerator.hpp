/*
 * EdgeGenerator.hpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_
#define ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_

#include <alphagomoku/utils/matrix.hpp>

#include <memory>
#include <limits>

namespace ag
{
	class SearchTask;
}

namespace ag
{

	class EdgeGenerator
	{
		public:
			EdgeGenerator() = default;
			EdgeGenerator(const EdgeGenerator &other) = delete;
			EdgeGenerator(EdgeGenerator &&other) = delete;
			EdgeGenerator& operator=(const EdgeGenerator &other) = delete;
			EdgeGenerator& operator=(EdgeGenerator &&other) = delete;
			virtual ~EdgeGenerator() = default;

			virtual std::unique_ptr<EdgeGenerator> clone() const = 0;
			virtual void generate(SearchTask &task) const = 0;
	};

	class BaseGenerator: public EdgeGenerator
	{
		private:
			float policy_threshold = 0.0f;
			int max_edges = std::numeric_limits<int>::max();
		public:
			BaseGenerator() = default;
			BaseGenerator(float policyThreshold, int maxEdges);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

	/**
	 * \brief Generator that assumes that VCF solver was run on the given task.
	 * It is because VCF solver has the ability to check for game end conditions, eliminating the need to do it again during edge generation.
	 */
	class SolverGenerator: public EdgeGenerator
	{
		private:
			float policy_threshold = 0.0f;
			int max_edges = std::numeric_limits<int>::max();
		public:
			SolverGenerator() = default;
			SolverGenerator(float policyThreshold, int maxEdges);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

	/*
	 * \brief
	 */
	class SequentialHalvingGenerator: public EdgeGenerator
	{
		private:
			int max_edges;
		public:
			SequentialHalvingGenerator(int maxEdges);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
		private:
			void generate_at_root(SearchTask &task) const;
			void generate_below_root(SearchTask &task) const;
	};

	/**
	 * \brief Generator that adds all edges and applies some noise to the policy.
	 */
	class NoisyGenerator: public EdgeGenerator
	{
		private:
			matrix<float> noise_matrix;
			float noise_weight;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			NoisyGenerator(const matrix<float> &noiseMatrix, float noiseWeight, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

	/**
	 * \brief Generator that adds all edges.
	 */
	class BalancedGenerator: public EdgeGenerator
	{
		private:
			const int balance_depth;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			BalancedGenerator(int balanceDepth, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

	/**
	 * \brief Generator that generates balanced moves at depth 0 excluding central NxN square
	 */
	class CenterExcludingGenerator: public EdgeGenerator
	{
		private:
			const int square_size;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			CenterExcludingGenerator(int squareSize, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

	/**
	 * \brief Generator that generates balanced moves at depth 0 including only central NxN square
	 */
	class CenterOnlyGenerator: public EdgeGenerator
	{
		private:
			const int square_size;
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			CenterOnlyGenerator(int squareSize, const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

	/**
	 * \brief Generator that generates balanced moves at depth 0 excluding symmetrical moves (if they exist)
	 */
	class SymmetricalExcludingGenerator: public EdgeGenerator
	{
		private:
			std::unique_ptr<EdgeGenerator> base_generator;
		public:
			SymmetricalExcludingGenerator(const EdgeGenerator &baseGenerator);
			std::unique_ptr<EdgeGenerator> clone() const;
			void generate(SearchTask &task) const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGEGENERATOR_HPP_ */
