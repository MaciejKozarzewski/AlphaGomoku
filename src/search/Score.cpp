/*
 * Score.cpp
 *
 *  Created on: Jan 26, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/game/rules.hpp>

namespace
{
	std::string spaces(int x)
	{
		if (x < 10)
			return "  ";
		if (x < 100)
			return " ";
		return "";
	}
}

namespace ag
{
	std::string toString(ProvenValue pv)
	{
		switch (pv)
		{
			default:
			case ProvenValue::UNKNOWN:
				return "UNKNOWN";
			case ProvenValue::LOSS:
				return "LOSS";
			case ProvenValue::DRAW:
				return "DRAW";
			case ProvenValue::WIN:
				return "WIN";
		}
	}
	ProvenValue convertProvenValue(GameOutcome outcome, Sign signToMove)
	{
		switch (outcome)
		{
			default:
			case GameOutcome::UNKNOWN:
				return ProvenValue::UNKNOWN;
			case GameOutcome::DRAW:
				return ProvenValue::DRAW;
			case GameOutcome::CROSS_WIN:
				return (signToMove == Sign::CROSS) ? ProvenValue::WIN : ProvenValue::LOSS;
			case GameOutcome::CIRCLE_WIN:
				return (signToMove == Sign::CIRCLE) ? ProvenValue::WIN : ProvenValue::LOSS;
		}
	}

	std::string toString(Bound b)
	{
		switch (b)
		{
			default:
			case Bound::NONE:
				return "NONE";
			case Bound::LOWER:
				return "LOWER";
			case Bound::UPPER:
				return "UPPER";
			case Bound::EXACT:
				return "EXACT";
		}
	}

	std::string Score::toString() const
	{
		switch (getProvenValue())
		{
			case ProvenValue::LOSS:
				if (isInfinite())
					return "-INF";
				else
					return "LOSS in " + std::to_string(getEval());
			case ProvenValue::DRAW:
				return "DRAW in " + std::to_string(getEval());
			default:
			case ProvenValue::UNKNOWN:
				if (getEval() > 0)
					return "+" + std::to_string(getEval());
				else
					return std::to_string(getEval());
			case ProvenValue::WIN:
				if (isInfinite())
					return "+INF";
				else
					return "WIN in " + std::to_string(-getEval());
		}
	}
	std::string Score::toFormattedString() const
	{
		std::string result;
		if (isProven())
		{
			if (isInfinite())
				result = this->toString();
			else
			{
				const int dist = std::min(getDistance(), 999);
				result = spaces(dist);
				switch (getProvenValue())
				{
					default:
						break;
					case ProvenValue::LOSS:
						result += 'L';
						break;
					case ProvenValue::DRAW:
						result += 'D';
						break;
					case ProvenValue::WIN:
						result += 'W';
						break;
				}
				result += std::to_string(dist);
			}
		}
		else
		{
			const int eval = std::min(std::abs(getEval()), 999);
			result = spaces(eval);
			if (getEval() == 0)
				result += ' ';
			else
				result += (getEval() > 0) ? '+' : '-';
			result += std::to_string(eval);
		}
		assert(result.size() == 4);
		return result;
	}

} /* namespace ag */

