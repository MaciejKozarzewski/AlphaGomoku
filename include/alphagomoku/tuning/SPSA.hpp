/*
 * SPSA.hpp
 *
 *  Created on: Apr 5, 2026
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TUNING_SPSA_HPP_
#define ALPHAGOMOKU_TUNING_SPSA_HPP_

#include <vector>
#include <functional>
#include <limits>

class Json;

namespace ag
{

	class SPSA
	{
			using opt_func = std::function<double(const std::vector<double>&)>;

			opt_func function_to_optimize;
			std::vector<double> theta;
			double a = 1.1;
			double c = 0.1;
			double alpha = 0.602;
			double gamma = 0.101;
			double step = 0; // k
			double best_value = std::numeric_limits<double>::lowest();
		public:
			SPSA(opt_func f, int dim) noexcept :
					function_to_optimize(f),
					theta(dim, 0.5)
			{
			}

			void set_a(double value) noexcept
			{
				a = value;
			}
			void set_c(double value) noexcept
			{
				c = value;
			}
			void set_alpha(double value) noexcept
			{
				alpha = value;
			}
			void set_gamma(double value) noexcept
			{
				gamma = value;
			}
			void set_initial_theta(const std::vector<double> &t)
			{
				theta = t;
			}

			double do_one_step(int max_iterations);

			void load_progress(const Json &json);
			Json save_progress() const;
		private:
			std::vector<double> generate_delta() const;

	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_TUNING_SPSA_HPP_ */
