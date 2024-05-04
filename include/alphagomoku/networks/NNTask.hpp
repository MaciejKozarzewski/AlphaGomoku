/*
 * NNTask.hpp
 *
 *  Created on: Mar 17, 2024
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_NNTASK_HPP_
#define ALPHAGOMOKU_NETWORKS_NNTASK_HPP_

#include <minml/core/Tensor.hpp>

#include <vector>

namespace ag
{

	class NNTask
	{
			ml::Tensor input_on_cpu;
			ml::Tensor input_on_device;
			std::vector<ml::Tensor> outputs_on_cpu;
			std::vector<ml::Tensor> targets_on_cpu;

		public:
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NNTASK_HPP_ */
