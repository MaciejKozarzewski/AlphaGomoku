/*
 * EvaluationGame.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <minml/utils/json.hpp>

namespace ag
{

	Player::Player(const GameConfig &gameOptions, const SelfplayConfig &options, NNEvaluator &evaluator, const std::string &name) :
			nn_evaluator(evaluator),
			game_config(gameOptions),
			tree(options.tree_config),
			search(gameOptions, options.search_config),
			name(name),
			simulations(options.simulations)
	{
		// TODO temporary hack to initialize shared hash table in the TSS
		setBoard(matrix<Sign>(gameOptions.rows, gameOptions.cols), Sign::CROSS);
		search.setBatchSize(0);
		search.select(tree);
		search.setBatchSize(options.search_config.max_batch_size);
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

		tree.setEdgeSelector(PUCTSelector(search.getConfig().exploration_constant, 0.5f));
		tree.setEdgeGenerator(BaseGenerator(search.getConfig().max_children, true));
	}
	void Player::selectSolveEvaluate()
	{
		search.select(tree, simulations);
		search.solve();
		const int tmp = nn_evaluator.getQueueSize();
		search.scheduleToNN(nn_evaluator);
		nn_evals += nn_evaluator.getQueueSize() - tmp;
	}
	void Player::expandBackup()
	{
		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);
	}
	bool Player::isSearchOver()
	{
		if (tree.getSimulationCount() >= simulations or tree.isRootProven())
		{
			search.cleanup(tree);
			nn_evals = 0;
			return true;
		}
		else
			return false;
	}
	ThreatSpaceSearch& Player::getSolver() noexcept
	{
		return search.getSolver();
	}
	NNEvaluator& Player::getNNEvaluator() noexcept
	{
		return nn_evaluator;
	}
	void Player::scheduleSingleTask(SearchTask &task)
	{
		nn_evaluator.addToQueue(task);
	}
	Move Player::getMove() const noexcept
	{
		BestEdgeSelector selector;
		const Node root_node = tree.getInfo( { });
		Edge *edge = selector.select(&root_node);

//		std::cout << "Player : " << getName() << '\n';
//		std::cout << Board::toString(tree.getBoard()) << '\n';
//		std::cout << root_node.toString() << '\n';
//		for (int i = 0; i < root_node.numberOfEdges(); i++)
//			std::cout << "    " << root_node.getEdge(i).toString() << '\n';

		return edge->getMove();
	}

	EvaluationGame::EvaluationGame(GameConfig gameConfig, EvaluatorThread &manager, bool useOpening) :
			manager_thread(manager),
			game(gameConfig),
			opening_generator(gameConfig, 8),
			use_opening(useOpening)
	{
	}
	void EvaluationGame::clear()
	{
		first_player.reset();
		second_player.reset();
		state = GAME_NOT_STARTED;
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
			{ // use previously created opening, but with switched players
				has_stored_opening = false;
				first_player->setSign(Sign::CIRCLE);
				second_player->setSign(Sign::CROSS);
				game.setPlayers(second_player->getName(), first_player->getName());
				game.loadOpening(opening);
				return true;
			}
			else
			{
				if (opening_generator.isEmpty())
					opening_generator.generate(8, first_player->getNNEvaluator(), first_player->getSolver());

				if (opening_generator.isEmpty())
					return false;
				else
				{
					has_stored_opening = true;
					first_player->setSign(Sign::CROSS);
					second_player->setSign(Sign::CIRCLE);
					game.setPlayers(first_player->getName(), second_player->getName());
					opening = opening_generator.pop();
					game.loadOpening(opening);
					return true;
				}
			}
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
	void EvaluationGame::generate(int p)
	{
		if (state == GAME_NOT_STARTED)
		{
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

		switch (p)
		{
			case 1: // run generation only for the first player
				if (game.getSignToMove() == first_player->getSign())
					break;
				else
					return;
			case 2: // run generation only for the second player
				if (game.getSignToMove() == second_player->getSign())
					break;
				else
					return;
			default:
			case 3: // run generation for both players
				break;
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
				game.makeMove(get_player().getMove());
				if (game.isOver())
				{
					manager_thread.addToBuffer(game);
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

