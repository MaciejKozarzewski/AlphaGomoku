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
	struct EdgeSelectorConfig;
}

namespace ag
{
	/**
	 * @brief Base interface responsible for selecting edge within the tree.
	 */
	class EdgeSelector
	{
		public:
			EdgeSelector() noexcept = default;
			EdgeSelector(const EdgeSelector &other) = delete;
			EdgeSelector(EdgeSelector &&other) = delete;
			EdgeSelector& operator=(const EdgeSelector &other) = delete;
			EdgeSelector& operator=(EdgeSelector &&other) = delete;
			virtual ~EdgeSelector() = default;

			virtual std::unique_ptr<EdgeSelector> clone() const = 0;
			virtual Edge* select(const Node *node) noexcept = 0;

			static std::unique_ptr<EdgeSelector> create(const EdgeSelectorConfig &config);
	};

	/**
	 * @brief Edge selector based on Thompson sampling.
	 */
	class ThompsonSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const std::string init_to;
			const std::string noise_type;
			const float noise_weight;
			const float exploration_constant; /**< controls the level of exploration */
		public:
			ThompsonSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector based on KL-UCB sampling.
	 */
	class KLUCBSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const std::string init_to;
			const std::string noise_type;
			const float noise_weight;
			const float exploration_constant; /**< controls the level of exploration */
		public:
			KLUCBSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector based on Bayes-UCB sampling.
	 */
	class BayesUCBSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const std::string init_to;
			const std::string noise_type;
			const float noise_weight;
			const float exploration_constant; /**< controls the level of exploration */
		public:
			BayesUCBSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + 0.5 * P(draw).
	 */
	class PUCTSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const std::string init_to;
			const std::string noise_type;
			const float noise_weight;
			const float exploration_constant; /**< controls the level of exploration */
		public:
			PUCTSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + 0.5 * P(draw).
	 */
	class PUCTfpuSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const std::string init_to;
			const std::string noise_type;
			const float noise_weight;
			const float exploration_constant; /**< controls the level of exploration */
		public:
			PUCTfpuSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief PUCT edge selector that optimizes P(win) + 0.5 * P(draw).
	 */
	class PUCTvarianceSelector: public EdgeSelector
	{
		private:
			std::vector<float> noisy_policy;
			const std::string noise_type;
			const float noise_weight;
			const float exploration_constant; /**< controls the level of exploration */
		public:
			PUCTvarianceSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Upper Confidence Bound edge selector.
	 */
	class UCBSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
		public:
			UCBSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Lower Confidence Bound edge selector.
	 */
	class LCBSelector: public EdgeSelector
	{
		private:
			const float exploration_constant; /**< controls the level of exploration */
		public:
			LCBSelector(const EdgeSelectorConfig &config);
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
			BalancedSelector();
			BalancedSelector(int balanceDepth, const EdgeSelector &baseSelector);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

	/**
	 * @brief Edge selector that chooses edge with the largest Q-value = P(win) + styleFactor * P(draw).
	 */
	class MaxValueSelector: public EdgeSelector
	{
		public:
			MaxValueSelector() noexcept;
			MaxValueSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};
	class MaxPolicySelector: public EdgeSelector
	{
		public:
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
	class MinVisitSelector: public EdgeSelector
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
		public:
			BestEdgeSelector();
			BestEdgeSelector(const EdgeSelectorConfig &config);
			std::unique_ptr<EdgeSelector> clone() const;
			Edge* select(const Node *node) noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_MONTE_CARLO_EDGESELECTOR_HPP_ */
