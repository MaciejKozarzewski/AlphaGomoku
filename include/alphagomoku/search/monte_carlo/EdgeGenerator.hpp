/*
 * EdgeGenerator.hpp
 *
 *  Created on: Sep 15, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGEGENERATOR_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGEGENERATOR_HPP_

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
			const int max_edges;
			const float expansion_threshold;
			const bool force_expand_root;
		public:
			BaseGenerator(int maxEdges, float expansionThreshold, bool forceExpandRoot = false);
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

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGEGENERATOR_HPP_ */
