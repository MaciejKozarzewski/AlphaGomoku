/*
 * networks.hpp
 *
 *  Created on: Feb 28, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_NETWORKS_NETWORKS_HPP_
#define ALPHAGOMOKU_NETWORKS_NETWORKS_HPP_

#include <alphagomoku/networks/AGNetwork.hpp>

namespace ag
{

	class ResnetPV: public AGNetwork
	{
		public:
			ResnetPV() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

	class ResnetPVQ: public AGNetwork
	{
		public:
			ResnetPVQ() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

	class ResnetPVQraw: public AGNetwork
	{
		public:
			ResnetPVQraw() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NETWORKS_HPP_ */
