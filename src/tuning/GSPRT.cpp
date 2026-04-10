/*
 * GSPRT.cpp
 *
 *  Created on: Apr 2, 2026
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/tuning/GSPRT.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/evaluation/TwoMatch.hpp>

#include <cmath>

namespace
{

	static constexpr double nelo_divided_by_nt = 800.0 / std::log(10.0); // 347.43558552260146

	double count_games(const std::array<double, 5> &results)
	{
		double games = 0.0;
		for (size_t i = 0; i < results.size(); i++)
			games += results[i];
		return games;
	}

	std::array<double, 5> results_to_pdf(const std::array<double, 5> &results, double epsilon = 1.0e-3)
	{
		const double games = count_games(results);
		std::array<double, 5> pdf;
		for (size_t i = 0; i < results.size(); i++)
			pdf[i] = std::max(epsilon, results[i]) / games;
		return pdf;
	}

	double get_mean(const std::array<double, 5> &pdf)
	{
		double result = 0.0;
		for (size_t i = 0; i < pdf.size(); i++)
			result += static_cast<double>(i) / static_cast<double>(pdf.size()) * pdf[i];
		return result;
	}
	double get_variance(const std::array<double, 5> &pdf)
	{
		const double mean = get_mean(pdf);
		double result = 0.0;
		for (size_t i = 0; i < pdf.size(); i++)
			result += static_cast<double>(i) / static_cast<double>(pdf.size()) * ag::square(pdf[i] - mean);
		return result;
	}

	double LLR_normalized(double nelo0, double nelo1, const std::array<double, 5> &results)
	{
		const double count = count_games(results);
		const std::array<double, 5> pdf = results_to_pdf(results);

		const double mean = get_mean(pdf);
		const double variance = get_variance(pdf);

		const double nt0 = nelo0 / nelo_divided_by_nt;
		const double nt1 = nelo1 / nelo_divided_by_nt;
		const double nt = (mean - 0.5) / std::sqrt(2.0 * variance);

		return count * std::log((1 + (nt - nt0) * (nt - nt0)) / (1 + (nt - nt1) * (nt - nt1)));
	}

	int get_points(ag::Sign player_sign, ag::GameOutcome outcome)
	{
		switch (outcome)
		{
			default:
			case ag::GameOutcome::UNKNOWN:
				throw std::logic_error("unknown game outcome");
			case ag::GameOutcome::DRAW:
				return 1;
			case ag::GameOutcome::CROSS_WIN:
				return (player_sign == ag::Sign::CROSS) ? 2 : 0;
			case ag::GameOutcome::CIRCLE_WIN:
				return (player_sign == ag::Sign::CIRCLE) ? 2 : 0;
		}
	}

}

namespace ag
{

	GSPRT::GSPRT(double alpha, double beta, double elo0, double elo1) :
			elo0(elo0),
			elo1(elo1),
			LA(std::log(beta / (1.0 - alpha))),
			LB(std::log((1.0 - beta) / alpha))
	{
		std::fill(results.begin(), results.end(), 0);
	}
	void GSPRT::add_result(int result)
	{
		results.at(result) += 1;
		LLR = LLR_normalized(elo0, elo1, results);
		if (LLR > max_LLR)
		{
			sq1 += square(LLR - max_LLR);
			max_LLR = LLR;
			o1 = sq1 / (2 * LLR);
		}
		if (LLR < min_LLR)
		{
			sq0 += square(LLR - min_LLR);
			min_LLR = LLR;
			o0 = -sq0 / (2 * LLR);
		}

		if (LLR > LB - o1)
			status = 1;
		else
		{
			if (LLR < LA + o0)
				status = 0;
		}
	}

	std::vector<int> convert_match_results(const std::vector<TwoMatch> &buffer)
	{
		std::vector<int> result(buffer.size(), 0);
		for (size_t i = 0; i < buffer.size(); i++)
		{
			const int game0 = get_points(Sign::CROSS, buffer[i].getGame(0).getOutcome());
			const int game1 = get_points(Sign::CIRCLE, buffer[i].getGame(1).getOutcome());
			result[i] = game0 + game1;
		}
		return result;
	}

} /* namespace ag */

