/*
 * EvaluationGame.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/mcts/NNEvaluator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <libml/utils/json.hpp>

namespace ag
{

	Player::Player(const GameConfig &gameOptions, const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name) :
			nn_evaluator(evaluator),
			game_config(gameOptions),
			tree(options.tree_config),
			search(gameOptions, options.search_config),
			name(name),
			simulations(options.simulations_min)
	{
	}
	void Player::setSign(Sign s) noexcept
	{
		sign = s;
	}
	Sign Player::getSign() const noexcept
	{
		return sign;
	}
	std::string Player::getName() const
	{
		return name;
	}
	void Player::setBoard(const matrix<Sign> &board, Sign signToMove)
	{
		search.cleanup(tree);
		tree.setBoard(board, signToMove);
		tree.setEdgeSelector(PuctSelector(search.getConfig().exploration_constant, 0.5f));
		tree.setEdgeGenerator(SolverGenerator(search.getConfig().expansion_prior_treshold, search.getConfig().max_children));
	}
	void Player::selectSolveEvaluate()
	{
		search.select(tree);
		search.solve();
		search.scheduleToNN(nn_evaluator);
	}
	void Player::expandBackup()
	{
		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
	}
	bool Player::isSearchOver()
	{
		if (tree.getSimulationCount() >= simulations or tree.isProven())
		{
			search.cleanup(tree);
			return true;
		}
		else
			return false;
	}
	void Player::scheduleSingleTask(SearchTask &task)
	{
		nn_evaluator.addToQueue(task);
	}
	Move Player::getMove() const noexcept
	{
		BestEdgeSelector selector;
		Node root_node = tree.getInfo( { });
		Edge *edge = selector.select(&root_node);

//		std::cout << Board::toString(tree.getBoard()) << '\n';
//		std::cout << root_node.toString() << '\n';
//		for (int i = 0; i < root_node.numberOfEdges(); i++)
//			std::cout << "    " << root_node.getEdge(i).toString() << '\n';

		return edge->getMove();
	}
	SearchData Player::getSearchData() const
	{
		const Node root_node = tree.getInfo( { });
		matrix<float> policy(game_config.rows, game_config.cols);
		matrix<ProvenValue> proven_values(game_config.rows, game_config.cols);
		matrix<Value> action_values(game_config.rows, game_config.cols);
		for (int i = 0; i < root_node.numberOfEdges(); i++)
		{
			Move m = root_node.getEdge(i).getMove();
			policy.at(m.row, m.col) = root_node.getEdge(i).getVisits();
			proven_values.at(m.row, m.col) = root_node.getEdge(i).getProvenValue();
			action_values.at(m.row, m.col) = root_node.getEdge(i).getValue();
		}
		normalize(policy);

		BestEdgeSelector selector;
		Edge *edge = selector.select(&root_node);
		Move move = edge->getMove();

		SearchData result(policy.rows(), policy.cols());
		result.setBoard(tree.getBoard());
		result.setActionProvenValues(proven_values);
		result.setPolicy(policy);
		result.setActionValues(action_values);
		result.setMinimaxValue(root_node.getValue());
		result.setProvenValue(root_node.getProvenValue());
		result.setMove(move);
		return result;
	}

	EvaluationGame::EvaluationGame(GameConfig gameConfig, GameBuffer &gameBuffer, bool useOpening, bool saveData) :
			game_buffer(gameBuffer),
			game(gameConfig),
			request(gameConfig.rules),
			use_opening(useOpening),
			save_data(saveData)
	{
	}
	void EvaluationGame::clear()
	{
		first_player.reset();
		second_player.reset();
		state = GAME_NOT_STARTED;
		is_request_scheduled = false;
		opening_trials = 0;
		has_stored_opening = false;
	}
	void EvaluationGame::setFirstPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name)
	{
		first_player = std::make_unique<Player>(game.getConfig(), options, evaluator, name);
	}
	void EvaluationGame::setSecondPlayer(const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name)
	{
		second_player = std::make_unique<Player>(game.getConfig(), options, evaluator, name);
	}
	bool EvaluationGame::prepareOpening()
	{
		assert(first_player != nullptr);

		if (use_opening)
		{
			if (has_stored_opening)
			{
				has_stored_opening = false;
				first_player->setSign(Sign::CIRCLE);
				second_player->setSign(Sign::CROSS);
				game.setPlayers(second_player->getName(), first_player->getName());
				game.loadOpening(opening);
				return true;
			}

			if (is_request_scheduled == true)
			{
				if (request.isReadyNetwork() == false)
					return false;
				is_request_scheduled = false;
				if (fabsf(request.getValue().win - request.getValue().loss) < (0.05f + opening_trials * 0.01f))
				{
					opening_trials = 0;
					has_stored_opening = true;
					first_player->setSign(Sign::CROSS);
					second_player->setSign(Sign::CIRCLE);
					game.setPlayers(first_player->getName(), second_player->getName());
					return true;
				}
				else
					opening_trials++;
			}

			opening = ag::prepareOpening(game.getConfig(), 2);
			game.loadOpening(opening);
			request.set(game.getBoard(), game.getSignToMove());
			first_player->scheduleSingleTask(request);
			is_request_scheduled = true;
			return false;
		}
		else
		{
			if (has_stored_opening == false)
			{
				has_stored_opening = true;
				first_player->setSign(Sign::CROSS);
				second_player->setSign(Sign::CIRCLE);
				game.setPlayers(first_player->getName(), second_player->getName());
			}
			else
			{
				has_stored_opening = false;
				first_player->setSign(Sign::CIRCLE);
				second_player->setSign(Sign::CROSS);
				game.setPlayers(second_player->getName(), first_player->getName());
			}
			return true;
		}
	}
	void EvaluationGame::generate()
	{
		if (state == GAME_NOT_STARTED)
		{
			if (use_opening)
				opening_trials = 0;
			state = PREPARE_OPENING;
		}

		if (state == PREPARE_OPENING)
		{
			bool isReady = prepareOpening();
			if (isReady == false)
				return;
			else
			{
				state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
				get_player().setBoard(game.getBoard(), game.getSignToMove());
			}
		}

		if (state == GAMEPLAY_SELECT_SOLVE_EVALUATE)
		{
			get_player().selectSolveEvaluate();
			state = GAMEPLAY_EXPAND_AND_BACKUP;
			return;
		}

		if (state == GAMEPLAY_EXPAND_AND_BACKUP)
		{
			get_player().expandBackup();
			if (get_player().isSearchOver())
			{
				if (save_data)
					game.addSearchData(get_player().getSearchData());
				game.makeMove(get_player().getMove());
				if (game.isOver())
				{
					game.resolveOutcome();
					game_buffer.addToBuffer(game);
					state = GAME_NOT_STARTED;
					return;
				}
				else
					get_player().setBoard(game.getBoard(), game.getSignToMove());
			}
			state = GAMEPLAY_SELECT_SOLVE_EVALUATE;
		}
	}
	/*
	 * private
	 */
	Player& EvaluationGame::get_player() const noexcept
	{
		assert(first_player != nullptr && first_player->getSign() != Sign::NONE);
		assert(second_player != nullptr && second_player->getSign() != Sign::NONE);
		assert(first_player->getSign() == invertSign(second_player->getSign()));

		if (game.getSignToMove() == first_player->getSign())
			return *first_player;
		else
			return *second_player;
	}
} /* namespace ag */

