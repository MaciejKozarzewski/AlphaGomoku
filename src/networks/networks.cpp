/*
 * networks.cpp
 *
 *  Created on: Feb 28, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/networks.hpp>
#include <alphagomoku/networks/blocks.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/utils/math_utils.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/training/Optimizer.hpp>
#include <minml/training/Regularizer.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/math.hpp>

#include <minml/layers/Conv2D.hpp>
#include <minml/layers/MultiHeadAttention.hpp>
#include <minml/layers/GlobalPooling.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/DepthToSpace.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/Multiply.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/LayerNormalization.hpp>
#include <minml/layers/RMSNormalization.hpp>
#include <minml/layers/SpaceToDepth.hpp>
#include <minml/layers/Softmax.hpp>

#include <string>

namespace
{
	void* get_pointer(ml::Tensor &src, std::initializer_list<int> idx)
	{
		assert(src.device().isCPU());
		return reinterpret_cast<uint8_t*>(src.data()) + ml::sizeOf(src.dtype()) * src.getIndexOf(idx);
	}
	const void* get_pointer(const ml::Tensor &src, std::initializer_list<int> idx)
	{
		assert(src.device().isCPU());
		return reinterpret_cast<const uint8_t*>(src.data()) + ml::sizeOf(src.dtype()) * src.getIndexOf(idx);
	}

	ag::float3 to_float3(const ag::Value &value) noexcept
	{
		return ag::float3 { value.win_rate, value.draw_rate, value.loss_rate() };
	}
	ag::Value from_float3(const ag::float3 &f) noexcept
	{
		return ag::Value(f.x, f.y);
	}

	template<typename T>
	void space_to_depth(void *dst, const ag::matrix<T> &src, int patch_size)
	{
		if (patch_size == 1)
		{
			std::memcpy(dst, src.data(), src.sizeInBytes());
			return;
		}
		int counter = 0;
		for (int i = 0; i < src.rows(); i += patch_size)
			for (int j = 0; j < src.cols(); j += patch_size)
				for (int h = 0; h < patch_size; h++)
					for (int w = 0; w < patch_size; w++)
					{
						if ((i + h) < src.rows() and (j + w) < src.cols())
							reinterpret_cast<T*>(dst)[counter] = src.at(i + h, j + w);
						else
							reinterpret_cast<T*>(dst)[counter] = T { };
						counter++;
					}
	}
	template<typename T>
	void depth_to_space(ag::matrix<T> &dst, const void *src, int patch_size)
	{
		if (patch_size == 1)
		{
			std::memcpy(dst.data(), src, dst.sizeInBytes());
			return;
		}
		int counter = 0;
		for (int i = 0; i < dst.rows(); i += patch_size)
			for (int j = 0; j < dst.cols(); j += patch_size)
				for (int h = 0; h < patch_size; h++)
					for (int w = 0; w < patch_size; w++)
					{
						if ((i + h) < dst.rows() and (j + w) < dst.cols())
							dst.at(i + h, j + w) = reinterpret_cast<const T*>(src)[counter];
						counter++;
					}
	}
}

namespace ag
{
	ResnetPV::ResnetPV() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPV::name() const
	{
		return "ResnetPV";
	}
	void ResnetPV::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw::ResnetPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw::name() const
	{
		return "ResnetPVraw";
	}
	void ResnetPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVQ::ResnetPVQ() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVQ::name() const
	{
		return "ResnetPVQ";
	}
	void ResnetPVQ::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		auto q = createActionValuesHead(graph, x, filters);
		graph.addOutput(q, 0.05f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPV::BottleneckPV() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPV::name() const
	{
		return "BottleneckPV";
	}
	void BottleneckPV::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVraw::BottleneckPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVraw::name() const
	{
		return "BottleneckPVraw";
	}
	void BottleneckPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckBroadcastPVraw::BottleneckBroadcastPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckBroadcastPVraw::name() const
	{
		return "BottleneckBroadcastPVraw";
	}
	void BottleneckBroadcastPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);
//		x = createBroadcastingBlock(graph, x);

		for (int i = 0; i < blocks; i++)
		{
			x = createBottleneckBlock_v3(graph, x, filters);
			if ((i + 1) % 4 == 0)
				x = createBroadcastingBlock(graph, x);
		}

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPoolingPVraw::BottleneckPoolingPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPoolingPVraw::name() const
	{
		return "BottleneckPoolingPVraw";
	}
	void BottleneckPoolingPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);
		x = createPoolingBlock(graph, x);

		for (int i = 0; i < blocks; i++)
		{
			x = createBottleneckBlock_v3(graph, x, filters);
			if ((i + 1) % 4 == 0)
				x = createPoolingBlock(graph, x);
		}

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVQ::BottleneckPVQ() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVQ::name() const
	{
		return "BottleneckPVQ";
	}
	void BottleneckPVQ::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		auto q = createActionValuesHead(graph, x, filters);
		graph.addOutput(q, 0.05f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVQraw::ResnetPVQraw() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVQraw::name() const
	{
		return "ResnetPVQraw";
	}
	void ResnetPVQraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		auto q = createActionValuesHead(graph, x, filters);
		graph.addOutput(q, 0.2f);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
		{
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
			graph.getNode(graph.numberOfNodes() - 2).getLayer().setRegularizer(ml::Regularizer(0.1f * trainingOptions.l2_regularization)); // action values head
		}
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetOld::ResnetOld() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetOld::name() const
	{
		return "ResnetOld";
	}
	ml::Graph& ResnetOld::getGraph()
	{
		return graph;
	}
	void ResnetOld::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 4 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = graph.addInput(input_shape);

		x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);

			y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
			y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);

			x = graph.add(ml::Add("relu"), { x, y });
		}

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);

		p = graph.add(ml::Conv2D(1, 1, "linear").useBias(false), p);
		p = graph.add(ml::Dense(game_config.rows * game_config.cols, "linear").useWeights(false), p);
		p = graph.add(ml::Softmax( { 1 }), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(2, 1, "linear").useBias(false), x);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);

		v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), v);
		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		v = graph.add(ml::Dense(3, "linear"), v);
		v = graph.add(ml::Softmax( { 1 }), v);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v0::ResnetPVraw_v0() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v0::name() const
	{
		return "ResnetPVraw_v0";
	}
	void ResnetPVraw_v0::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v1::ResnetPVraw_v1() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v1::name() const
	{
		return "ResnetPVraw_v1";
	}
	void ResnetPVraw_v1::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 4 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v2::ResnetPVraw_v2() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v2::name() const
	{
		return "ResnetPVraw_v2";
	}
	void ResnetPVraw_v2::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 4 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createResidualBlock(graph, x, filters);

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);

		p = graph.add(ml::Conv2D(1, 1, "linear").useBias(false), p);
		p = graph.add(ml::Dense(game_config.rows * game_config.cols, "linear").useWeights(false), p);
		p = graph.add(ml::Softmax( { 1 }), p);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	Transformer::Transformer() noexcept :
			AGNetwork()
	{
	}
	std::string Transformer::name() const
	{
		return "Transformer";
	}
	void Transformer::packInputData(int index, const NNInputFeatures &features)
	{
		assert(index >= 0 && index < getBatchSize());
		space_to_depth(get_pointer(*input_on_cpu, { index, 0, 0, 0 }), features, patch_size);

//		pattern_calculator->print();
//		for (int i = 0; i < features.rows(); i++)
//			for (int j = 0; j < features.cols(); j++)
//				std::cout << i << "," << j << " " << std::bitset<32>(features.at(i, j)).to_string() << '\n';
//
//		int counter = 0;
//		for (int i = 0; i < input_on_cpu->dim(1); i++)
//			for (int j = 0; j < input_on_cpu->dim(2); j++)
//			{
//				for (int k = 0; k < input_on_cpu->dim(3); k++, counter++)
//					std::cout << i << "," << j << "," << k << " "
//							<< std::bitset<32>(reinterpret_cast<uint32_t*>(input_on_cpu->data())[counter]).to_string() << '\n';
//				std::cout << '\n';
//			}
//		exit(0);
	}
	void Transformer::packTargetData(int index, const matrix<float> &policy, const matrix<Value> &actionValues, Value value)
	{
		assert(index >= 0 && index < getBatchSize());

		space_to_depth(get_pointer(targets_on_cpu.at(POLICY_OUTPUT_INDEX), { index, 0 }), policy, patch_size);

		const float3 tmp = to_float3(value);
		std::memcpy(get_pointer(targets_on_cpu.at(VALUE_OUTPUT_INDEX), { index, 0 }), &tmp, sizeof(tmp));

//		if (targets_on_cpu.size() == 3)
//		{
//			workspace.resize(game_config.rows * game_config.cols);
//			for (int i = 0; i < actionValues.size(); i++)
//				workspace[i] = to_float3(actionValues[i]);
//			space_to_depth(get_pointer(targets_on_cpu.at(ACTION_VALUES_OUTPUT_INDEX), { index, 0, 0, 0 }), workspace, patch_size);
//		}
	}
	void Transformer::unpackOutput(int index, matrix<float> &policy, matrix<Value> &actionValues, Value &value) const
	{
		assert(index >= 0 && index < getBatchSize());

		workspace.resize(outputs_on_cpu.at(POLICY_OUTPUT_INDEX).lastDim());
		ml::convertType(context_on_cpu, workspace.data(), ml::DataType::FLOAT32, get_pointer(outputs_on_cpu.at(POLICY_OUTPUT_INDEX), { index, 0 }),
				outputs_on_cpu.at(POLICY_OUTPUT_INDEX).dtype(), policy.size());
		depth_to_space(policy, workspace.data(), patch_size);

		float3 tmp;
		ml::convertType(context_on_cpu, &tmp, ml::DataType::FLOAT32, get_pointer(outputs_on_cpu.at(VALUE_OUTPUT_INDEX), { index, 0 }),
				outputs_on_cpu.at(VALUE_OUTPUT_INDEX).dtype(), 3);
		value = from_float3(tmp);

//		if (outputs_on_cpu.size() == 3)
//		{
//			workspace.resize(game_config.rows * game_config.cols);
//			ml::convertType(context_on_cpu, workspace.data(), ml::DataType::FLOAT32, get_pointer(outputs_on_cpu.at(ACTION_VALUES_OUTPUT_INDEX), {
//					index, 0, 0, 0 }), outputs_on_cpu.at(ACTION_VALUES_OUTPUT_INDEX).dtype(), 3 * workspace.size());
//			for (int i = 0; i < actionValues.size(); i++)
//				actionValues[i] = from_float3(workspace[i]);
//		}
	}
	/*
	 * private
	 */
	ml::Shape Transformer::get_input_encoding_shape() const
	{
		return ml::Shape( { graph.getInputShape()[0], graph.getInputShape()[1], graph.getInputShape()[2], square(patch_size) });
	}
	void Transformer::create_network(const TrainingConfig &trainingOptions)
	{
		const int height = (game_config.rows + patch_size - 1) / patch_size;
		const int width = (game_config.cols + patch_size - 1) / patch_size;
		const int in_channels = 32 * square(patch_size);
		const int out_channels = square(patch_size);

		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, height, width, in_channels });
		const int blocks = trainingOptions.blocks;
		const int embedding = trainingOptions.filters;
		const int head_dim = 32;
		const int pos_encoding_range = std::max(height, width);

		auto x = graph.addInput(input_shape);
		x = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::RMSNormalization(), x);
			y = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), y);
			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), y);
			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
			x = graph.add(ml::Add(), { x, y });

			y = graph.add(ml::RMSNormalization(), x);
			y = graph.add(ml::Conv2D(embedding, 1, "relu"), y);
//			auto lhs = graph.add(ml::Conv2D(embedding, 1, "relu"), y);
//			auto rhs = graph.add(ml::Conv2D(embedding, 1), y);
//			y = graph.add(ml::Multiply(), {lhs, rhs});
			y = graph.add(ml::Conv2D(embedding, 1), y);

//			x = graph.add(ml::Add(), { x, y });

//			auto y = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), x);
//			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), y);
//			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
//			y = graph.add(ml::RMSNormalization(), y);
//			x = graph.add(ml::Add(), { x, y });
//
//			y = graph.add(ml::Conv2D(embedding, 1, "relu"), x);
//			y = graph.add(ml::Conv2D(embedding, 1), y);
//			y = graph.add(ml::RMSNormalization(), y);
//			x = graph.add(ml::Add(), { x, y });
		}
//		x = graph.add(ml::RMSNormalization(), x);

		// policy head
//		auto p = graph.add(ml::Conv2D(256, 3, "linear").useBias(false), x);
//		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);
//		p = graph.add(ml::Conv2D(out_channels, 1, "linear"), p);
//		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);

//		auto p = graph.add(ml::Conv2D(3 * 256, 1).useBias(false), x);
//		p = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), p);
		auto p = graph.add(ml::Conv2D(256, 1, "relu"), x);
		p = graph.add(ml::Conv2D(out_channels, 1), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p);

		// value head
//		auto v = graph.add(ml::Conv2D(4, 1, "linear").useBias(false), x);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(256, "linear").useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3, "linear"), v);
//		v = graph.add(ml::Softmax( { 1 }), v);

		auto v = graph.add(ml::GlobalPooling(), x);
		v = graph.add(ml::Dense(256, "relu"), v);
		v = graph.add(ml::Dense(3), v);
		v = graph.add(ml::Softmax( { 1 }), v);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	Transformer_v2::Transformer_v2() noexcept :
			AGNetwork()
	{
	}
	std::string Transformer_v2::name() const
	{
		return "Transformer_v2";
	}
	/*
	 * private
	 */
	void Transformer_v2::create_network(const TrainingConfig &trainingOptions)
	{
		const int height = game_config.rows;
		const int width = game_config.cols;

		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, height, width, 32 });
		const int blocks = trainingOptions.blocks;
		const int embedding = trainingOptions.filters;
		const int head_dim = 32;
		const int patch_size = trainingOptions.patch_size;
		const int pos_encoding_range = std::max((height + patch_size - 1) / patch_size, (width + patch_size - 1) / patch_size);
		assert(embedding % head_dim == 0);

//		auto x = graph.addInput(input_shape);
//		x = graph.add(ml::SpaceToDepth(patch_size), x);
//		x = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);
//
//		for (int i = 0; i < blocks; i++)
//		{
//			auto y = graph.add(ml::RMSNormalization(), x);
//			y = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), y);
//			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), y);
//			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
//			x = graph.add(ml::Add(), { x, y });
//
////			auto y = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), x);
////			y = graph.add(ml::BatchNormalization().useGamma(false), y);
////			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), y);
////			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
////			y = graph.add(ml::BatchNormalization().useGamma(false), y);
////			x = graph.add(ml::Add(), { x, y });
//
//			y = graph.add(ml::RMSNormalization(), x);
//			y = graph.add(ml::Conv2D(embedding, 1, "relu"), y);
//			y = graph.add(ml::Conv2D(embedding, 1), y);
//			x = graph.add(ml::Add(), { x, y });
//
////			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);
////			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
////			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
////			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
////			x = graph.add(ml::Add(), { x, y });
//		}
//
//		auto p = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);
//		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);
//		p = graph.add(ml::DepthToSpace(patch_size, { game_config.rows, game_config.cols }), p);
//		p = graph.add(ml::Conv2D(1, 1), p);
//		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
//		graph.addOutput(p);
//
//		auto v = graph.add(ml::GlobalPooling(), x);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v);

		auto x = graph.addInput(input_shape);
//		x = graph.add(ml::Conv2D(embedding, 5, "linear").useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

//		for (int i = 0; i < blocks; i++)
//		{
//			auto y = graph.add(ml::Conv2D(embedding / 2, 1, "linear").useBias(false), x);
//			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
//
//			y = graph.add(ml::Conv2D(embedding / 2, 3, "linear").useBias(false), y);
//			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
//
//			y = graph.add(ml::Conv2D(embedding, 3, "linear").useBias(false), y);
//			y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);
//
//			x = graph.add(ml::Add("relu"), { x, y });
//		}

//		auto p = graph.add(ml::RMSNormalization(), x);
//		p = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), p);
//		p = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), p);
		auto p = graph.add(ml::Conv2D(1, 1), x);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p);

//		auto v = graph.add(ml::GlobalPooling(), x);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
		auto v = graph.add(ml::Conv2D(1, 1), x);
		v = graph.add(ml::Dense(3), v);
		v = graph.add(ml::Softmax( { 1 }), v);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::Optimizer());
		if (trainingOptions.l2_regularization != 0.0)
			graph.setRegularizer(ml::Regularizer(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

} /* namespace ag */

