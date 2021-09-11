/*
 * rules.hpp
 *
 *  Created on: Sep 10, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_GAME_RULES_HPP_
#define ALPHAGOMOKU_GAME_RULES_HPP_

#include <string>

namespace ag
{
	enum class GameRules
	{
		FREESTYLE,
		STANDARD,
		RENJU,
		CARO
	};
	std::string toString(GameRules rules);
	GameRules rulesFromString(const std::string &str);

	enum class GameOutcome
	{
		UNKNOWN,
		DRAW,
		CROSS_WIN, // or black win
		CIRCLE_WIN // or white win
	};
	std::string toString(GameOutcome outcome);
	GameOutcome outcomeFromString(const std::string &str);

} /* namespace ag */

#endif /* ALPHAGOMOKU_GAME_RULES_HPP_ */
