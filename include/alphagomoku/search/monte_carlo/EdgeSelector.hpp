/*
 * EdgeSelector.hpp
 *
 *  Created on: Sep 9, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_
#define ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_

#include <memory>
#include <vector>
#include <random>

namespace ag
{
	class Move;
	class Node;
	class Edge;
}

namespace ag
{
	/**
	 * @brief Base interface responsible for selecting edge within the tree.
	 */
	class EdgeSelector
	{
		public:
			EdgeSelector() = default;
			EdgeSelector(const EdgeSelector &other) = delete;
			EdgeSelector(EdgeSelector &&other) = delete;
			EdgeSelector& operator=(const EdgeSelector &other) = delete;
			EdgeSelector& operator=(EdgeSelector &&other) = delete;
			virtual ~EdgeSelector() = default;

			virtual std::unique_ptr<EdgeSelector> clone() const = 0;
			virtual Edge* select(const Node *node) noexcept = 0;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class PUCTSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			PUCTSelector(float exploration, float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief UCT edge selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class UCTSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			UCTSelector(float exploration, float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + styleFactor * P(draw) but add some noise at root.
	 */
	class NoisyPUCTSelector: public EdgeSelector
	{
		private:
			std::default_random_engine generator;
			std::extreme_value_distribution<float> noise;
			std::vector<float> noisy_policy;
			const float exploration_constant; /**< controls the level of exploration */
			const float style_factor; /**< used to determine what to optimize during search */
			bool is_initialized = false;
		public:
			NoisyPUCTSelector(float exploration, float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector used to find few balanced moves, then continues with the baseSelector.
	 */
	class BalancedSelector: public EdgeSelector
	{
		private:
			const int balance_depth;
			std::unique_ptr<EdgeSelector> base_selector;
		public:
			BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the largest Q-value = P(win) + styleFactor * P(draw).
	 */
	class MaxValueSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			MaxValueSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the most visits.
	 */
	class MaxVisitSelector: public EdgeSelector
	{
		public:
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge that is considered to be the best.
	 */
	class BestEdgeSelector: public EdgeSelector
	{
		private:
			const float style_factor; /**< used to determine what to optimize during search */
		public:
			BestEdgeSelector(float styleFactor = 0.5f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Sequential Halving selector that optimizes P(win) + styleFactor * P(draw).
	 */
	class SequentialHalvingSelector: public EdgeSelector
	{
		private:
			struct Data
			{
					Edge *edge = nullptr;
					float noise = 0.0f;
					float logit = 0.0f;
			};
			std::default_random_engine generator;
			std::extreme_value_distribution<float> noise;

			std::vector<Data> action_list;
			std::vector<float> workspace;

			const int max_edges;
			const int max_simulations;
			const float C_visit;
			const float C_scale;

			int simulations_left = 0;
			int expected_visit_count = 0;
			bool is_initialized = false;
		public:
			SequentialHalvingSelector(int maxEdges, int maxSimulations, float cVisit = 50.0f, float cScale = 1.0f);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
		private:
			void initialize(const Node *node);
			Edge* select_at_root(const Node *node) noexcept;
			Edge* select_below_root(const Node *node) noexcept;
			void sort_workspace(int topN) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_ */
