/*
 * SearchTask.hpp
 *
 *  Created on: Sep 14, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_
#define ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/selfplay/NNInputFeatures.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace ag
{
	/**
	 * \brief Combines Node and Edge selected in each step of select phase.
	 * Node and Edge must always be in pairs.
	 */
	struct NodeEdgePair
	{
			Node *node = nullptr; // non-owning
			Edge *edge = nullptr; // non-owning
	};
	/**
	 *\brief Collects together all information required in a single step of the tree search.
	 */
	class SearchTask
	{
		private:
			std::vector<NodeEdgePair> visited_path;/**< trajectory within the tree visited during select phase */
			std::vector<Edge> edges; /**< edges created by EdgeGenerator */

			matrix<Sign> board; /**< board representation of a state at the end of visited_path */

			NNInputFeatures features; /**< features used as input to the network */
			matrix<float> policy; /**< policy returned from neural network */

			matrix<Value> action_values; /**< matrix of action values returned from neural network */
			Value value; /**< whole position value returned from neural network */

			matrix<Score> action_scores; /**< matrix of action scores returned from solver */
			Score score; /**< whole position score computed by solver */

			GameConfig game_config;
			Sign sign_to_move = Sign::NONE;
			bool was_processed_by_network = false; /**< flag indicating whether the task has been evaluated by neural network and can be used for edge generation */
			bool was_processed_by_solver = false; /**< flag indicating whether the task has been evaluated by alpha-beta search and can be used for edge generation */
			bool must_defend = false; /**< flag indicating whether the player to move must defend a threat from the opponent */
		public:
			SearchTask(GameConfig config);
			void set(const matrix<Sign> &base, Sign signToMove);
			int getAbsoluteDepth() const noexcept
			{
				if (visited_path.size() == 0)
					return 0;
				else
					return visited_path.back().node->getDepth();
			}
			int getRelativeDepth() const noexcept
			{
				return visitedPathLength();
			}

			int visitedPathLength() const noexcept
			{
				return static_cast<int>(visited_path.size());
			}
			NodeEdgePair getPair(int index) const noexcept
			{
				assert(index >= 0 && index < visitedPathLength());
				return visited_path[index];
			}
			NodeEdgePair getLastPair() const noexcept
			{
				assert(visitedPathLength() > 0);
				return visited_path.back();
			}
			Edge* getLastEdge() const noexcept
			{
				if (visited_path.size() == 0)
					return nullptr;
				else
					return visited_path.back().edge;
			}

			const std::vector<Edge>& getEdges() const noexcept
			{
				return edges;
			}
			std::vector<Edge>& getEdges() noexcept
			{
				return edges;
			}
			GameConfig getGameConfig() const noexcept
			{
				return game_config;
			}
			Sign getSignToMove() const noexcept
			{
				return sign_to_move;
			}
			Value getValue() const noexcept
			{
				return value;
			}
			Score getScore() const noexcept
			{
				return score;
			}
			bool wasProcessedByNetwork() const noexcept
			{
				return was_processed_by_network;
			}
			bool wasProcessedBySolver() const noexcept
			{
				return was_processed_by_solver;
			}
			bool isReady() const noexcept
			{
				return score.isProven() or wasProcessedByNetwork();
			}
			bool mustDefend() const noexcept
			{
				return must_defend;
			}
			const NNInputFeatures& getFeatures() const noexcept
			{
				return features;
			}
			NNInputFeatures& getFeatures() noexcept
			{
				return features;
			}
			const matrix<Sign>& getBoard() const noexcept
			{
				return board;
			}
			matrix<Sign>& getBoard() noexcept
			{
				return board;
			}
			const matrix<float>& getPolicy() const noexcept
			{
				return policy;
			}
			matrix<float>& getPolicy() noexcept
			{
				return policy;
			}
			const matrix<Value>& getActionValues() const noexcept
			{
				return action_values;
			}
			matrix<Value>& getActionValues() noexcept
			{
				return action_values;
			}
			const matrix<Score>& getActionScores() const noexcept
			{
				return action_scores;
			}
			matrix<Score>& getActionScores() noexcept
			{
				return action_scores;
			}

			void append(Node *node, Edge *edge);
			void markAsProcessedByNetwork() noexcept
			{
				was_processed_by_network = true;
			}
			void markAsProcessedBySolver() noexcept
			{
				was_processed_by_solver = true;
			}
			void setMustDefendFlag(bool flag) noexcept
			{
				must_defend = flag;
			}

			void setValue(Value v) noexcept
			{
				value = v;
			}
			void setScore(Score s) noexcept
			{
				score = s;
			}

			void addEdge(Move move);
			std::string toString() const;
	};

	class SearchTaskList
	{
			std::vector<SearchTask> tasks;
			GameConfig game_config;
			int stored_elements = 0;
			int max_size = 0;
		public:
			SearchTaskList(GameConfig config, size_t maxSize) noexcept :
					game_config(config)
			{
				resize(maxSize);
			}
			void resize(size_t newMaxSize)
			{
				assert(stored_elements == 0);
				max_size = newMaxSize;
				if (newMaxSize > tasks.size())
					for (int i = tasks.size(); i < max_size; i++)
						tasks.push_back(SearchTask(game_config));
			}
			int maxSize() const noexcept
			{
				return max_size;
			}
			int storedElements() const noexcept
			{
				return stored_elements;
			}
			void clear() noexcept
			{
				stored_elements = 0;
			}
			SearchTask& getNext()
			{
				assert(stored_elements < max_size);
				stored_elements++;
				return tasks.at(stored_elements - 1);
			}
			void removeLast() noexcept
			{
				assert(stored_elements > 0);
				stored_elements--;
			}
			SearchTask& get(int index)
			{
				return tasks.at(index);
			}
			const SearchTask& get(int index) const
			{
				return tasks.at(index);
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_SEARCHTASK_HPP_ */
