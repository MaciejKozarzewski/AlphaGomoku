/*
 * Edge.cpp
 *
 *  Created on: Sep 7, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/random.hpp>

#include <iostream>

namespace ag
{
	std::string Edge::toString() const
	{
		std::string result = getMove().text();
		if (getMove().row < 10)
			result += " ";
		result += " : S=" + getScore().toFormattedString();
		result += " : Q=" + getValue().toString();
//		result += " (" + std::to_string(getExpectation()) + " +/- " + std::to_string(getVisits() >= 2 ? getVariance() : 0.0f) + ")";
		result += " : Visits=" + std::to_string(getVisits());
#ifndef NDEBUG
		result += " (" + std::to_string(getVirtualLoss()) + ")";
		result += " : flags=" + std::to_string(static_cast<int>(isBeingExpanded()));
#endif
		result += " : P=" + std::to_string(getPolicyPrior());
		return result;
	}

	float Edge::sampleFromDistribution() const noexcept
	{
		switch (getProvenValue())
		{
			case ProvenValue::LOSS:
				return -1000.0f + getScore().getDistance();
			case ProvenValue::DRAW:
				return Value::draw().getExpectation();
			default:
			case ProvenValue::UNKNOWN:
				break;
			case ProvenValue::WIN:
				return +1000.0f - getScore().getDistance();
		}

		if (getVisits() < 2)
		{
			const float P = getPolicyPrior() / (1.0f + getVisits());
//			return P;
			return (randFloat() <= P) ? (100.0f + P) : P;
		}
		return randGaussian(getExpectation(), getVariance());

//		if(getVisits() == 0)
//			return 0.0f;

//		const float P = getPolicyPrior() / (1.0f + getVisits());
//		if (randFloat() <= P or getVisits() == 0)
//			return 100.0f + P;
//
//		if (getVisits() == 0)
//		{
//			const float P = getPolicyPrior();
//			return (randFloat() <= P) ? (100.0f + P) : P;
//		}
//		const int N = getVisits();
//		const float alpha = 1.0f + getExpectation() * N;
//		const float beta = 1.0f + (1.0f - getExpectation()) * N;
//		return randBeta(alpha, beta);

//		int tmp = randInt(getVisits());
////		const int orig_tmp = tmp;
//
//		int idx = 0;
//		for (; idx < nb_bins; idx++)
//		{
//			if (tmp < histogram[idx])
//				break;
//			tmp -= histogram[idx];
//		}
//		const float prev = static_cast<float>(idx) / nb_bins;
//		const float next = static_cast<float>(idx + 1) / nb_bins;
//		const float fraction = tmp / (1.0e-8f + static_cast<float>(histogram[idx]));
//
////		if (getVisits() == 1000)
////		{
////			std::cout << "rand = " << orig_tmp << " (" << tmp << ")\n";
////			for (int i = 0; i < nb_bins; i++)
////				std::cout << "bin " << i << "  = " << histogram[i] << '\n';
////			std::cout << "range " << prev << " to " << next << ", fraction = " << fraction << '\n';
////			exit(0);
////		}
//
//		return prev + (next - prev) * fraction;
	}
	void Edge::updateDistribution(Value eval) noexcept
	{
//		const int idx = eval.getExpectation() * nb_bins;
//		histogram[std::max(0, std::min(nb_bins - 1, idx))]++;
	}
} /* namespace ag */

