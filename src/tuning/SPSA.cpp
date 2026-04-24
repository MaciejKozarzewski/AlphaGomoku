/*
 * SPSA.cpp
 *
 *  Created on: Apr 8, 2026
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/tuning/SPSA.hpp>
#include <alphagomoku/utils/random.hpp>

#include <minml/utils/json.hpp>

#include <iostream>
#include <cmath>

namespace
{
	std::vector<double> constrain_to_01(const std::vector<double> &x)
	{
		std::vector<double> result(x.size());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = std::max(0.0, std::min(1.0, x[i]));
		return result;
	}
	std::vector<double> linear_combination(double a, const std::vector<double> &x, double b, const std::vector<double> &y)
	{
		assert(x.size() == y.size());
		std::vector<double> result(x.size());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = a * x[i] + b * y[i];
		return result;
	}

	std::string to_string(const std::vector<double> &x)
	{
		std::string result;
		for (size_t i = 0; i < x.size(); i++)
			result += " " + std::to_string(x[i]);
		return result;
	}
}

namespace ag
{

	double SPSA::do_one_step(int max_iterations)
	{
		const double A = max_iterations / 10.0;
		step++;

		std::cout << "step = " << step << '\n';
		std::cout << "theta =" << to_string(theta) << '\n';

		const double c_k = c / std::pow(step, gamma);
		const double a_k = a / std::pow((step + A), alpha);

		std::cout << "c_k = " << c_k << ", a_k = " << a_k << '\n';

		const std::vector<double> delta = generate_delta();
		std::cout << "delta =" << to_string(delta) << '\n';

		const std::vector<double> theta_plus_delta = constrain_to_01(linear_combination(1.0, theta, c_k, delta));
		const std::vector<double> theta_minus_delta = constrain_to_01(linear_combination(1.0, theta, -c_k, delta));
		std::cout << "theta_plus_delta =" << to_string(theta_plus_delta) << '\n';
		std::cout << "theta_minus_delta =" << to_string(theta_minus_delta) << '\n';

		double grad;
		if (gradient_function)
			grad = gradient_function(theta_plus_delta, theta_minus_delta);
		else
		{
			const double fm = function_to_optimize(theta_minus_delta);
			std::cout << "fm = " << fm << '\n';
			const double fp = function_to_optimize(theta_plus_delta);
			std::cout << "fp = " << fp << '\n';
			best_value = std::max(best_value, std::max(fm, fp));
			grad = fp - fm;
		}

		std::vector<double> gradient(theta.size());
		for (size_t i = 0; i < gradient.size(); i++)
			gradient[i] = grad / (2.0 * c_k * delta[i]);
		std::cout << "gradient =" << to_string(gradient) << '\n';

		theta = constrain_to_01(linear_combination(1.0, theta, a_k, gradient));
		std::cout << "new theta =" << to_string(theta) << '\n';
		std::cout << "diff = " << grad << '\n';
		std::cout << "best = " << best_value << '\n';
		std::cout << '\n';

		return grad;
	}
	void SPSA::load_progress(const Json &json)
	{
	}
	Json SPSA::save_progress() const
	{
	}
	/*
	 * private
	 */
	std::vector<double> SPSA::generate_delta() const
	{
		std::vector<double> result(theta.size());
		for (size_t i = 0; i < result.size(); i++)
			result[i] = randBool() ? 1.0 : -1.0;
		return result;
	}

} /* namespace ag */

