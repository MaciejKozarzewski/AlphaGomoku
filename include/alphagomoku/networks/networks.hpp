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

	class BottleneckPV: public AGNetwork
	{
		public:
			BottleneckPV() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};
	class BottleneckPVraw: public AGNetwork
	{
		public:
			BottleneckPVraw() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};
	class BottleneckBroadcastPVraw: public AGNetwork
	{
		public:
			BottleneckBroadcastPVraw() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};
	class BottleneckPVQ: public AGNetwork
	{
		public:
			BottleneckPVQ() noexcept;
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

	class ResnetOld: public AGNetwork
	{
		public:
			ResnetOld() noexcept;
			std::string name() const;
			ml::Graph& getGraph();
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

	class ResnetPVraw_v0: public AGNetwork
	{
		public:
			ResnetPVraw_v0() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

	class ResnetPVraw_v1: public AGNetwork
	{
		public:
			ResnetPVraw_v1() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

	class ResnetPVraw_v2: public AGNetwork
	{
		public:
			ResnetPVraw_v2() noexcept;
			std::string name() const;
		protected:
			void create_network(const TrainingConfig &trainingOptions);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_NETWORKS_NETWORKS_HPP_ */
