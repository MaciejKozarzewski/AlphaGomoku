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
#include <minml/training/LossFunction.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/math.hpp>

#include <minml/layers/ChannelScaling.hpp>
#include <minml/layers/Conv2D.hpp>
#include <minml/layers/MultiHeadAttention.hpp>
#include <minml/layers/GlobalAveragePooling.hpp>
#include <minml/layers/LearnableGlobalPooling.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/DepthToSpace.hpp>
#include <minml/layers/DepthwiseConv2D.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/Multiply.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/LayerNormalization.hpp>
#include <minml/layers/PositionalEncoding.hpp>
#include <minml/layers/RMSNormalization.hpp>
#include <minml/layers/SpaceToDepth.hpp>
#include <minml/layers/Softmax.hpp>
#include <minml/layers/WindowPartitioning.hpp>
#include <minml/layers/WindowMerging.hpp>

#include <string>

namespace ag
{
	ResnetPV::ResnetPV() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPV::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw::ResnetPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVQ::ResnetPVQ() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVQ::getOutputConfig() const
	{
		return "pvq";
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
		graph.addOutput(q, ml::CrossEntropyLoss(), 0.05f);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPV::BottleneckPV() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPV::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVraw::BottleneckPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVraw::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckBroadcastPVraw::BottleneckBroadcastPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckBroadcastPVraw::getOutputConfig() const
	{
		return "pv";
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
//			if ((i + 1) % 4 == 0)
//				x = createBroadcastingBlock(graph, x);
		}

		auto p = createPolicyHead(graph, x, filters);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPoolingPVraw::BottleneckPoolingPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPoolingPVraw::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVQ::BottleneckPVQ() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVQ::getOutputConfig() const
	{
		return "pvq";
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
		graph.addOutput(q, ml::CrossEntropyLoss(), 0.05f);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVQraw::ResnetPVQraw() noexcept :
			AGNetwork()
	{
	}

	std::string ResnetPVQraw::getOutputConfig() const
	{
		return "pvq";
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
		graph.addOutput(q, ml::CrossEntropyLoss(), 0.2f);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetOld::ResnetOld() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetOld::getOutputConfig() const
	{
		return "pv";
	}
	std::string ResnetOld::name() const
	{
		return "ResnetOld";
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
		p = graph.add(ml::Dense(game_config.rows * game_config.cols, "linear"), p);
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v0::ResnetPVraw_v0() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v0::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v1::ResnetPVraw_v1() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v1::getOutputConfig() const
	{
		return "pv";
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
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ResnetPVraw_v2::ResnetPVraw_v2() noexcept :
			AGNetwork()
	{
	}
	std::string ResnetPVraw_v2::getOutputConfig() const
	{
		return "pv";
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
		p = graph.add(ml::Dense(game_config.rows * game_config.cols, "linear"), p);
		p = graph.add(ml::Softmax( { 1 }), p);
		graph.addOutput(p);

		auto v = createValueHead(graph, x, filters);
		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	Transformer_v2::Transformer_v2() noexcept :
			AGNetwork()
	{
	}
	std::string Transformer_v2::getOutputConfig() const
	{
		return "pv";
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
		const int batch_size = trainingOptions.device_config.batch_size;
		const int height = game_config.rows;
		const int width = game_config.cols;

		const ml::Shape input_shape( { batch_size, height, width, 32 });
		const bool symmetric = false;
		const int blocks = trainingOptions.blocks;
		const int embedding = trainingOptions.filters;
		const int head_dim = 32;
		const int patch_size = trainingOptions.patch_size;
		const int pos_encoding_range = std::max((height + patch_size - 1) / patch_size, (width + patch_size - 1) / patch_size);
		const int num_heads = embedding / head_dim;
		assert(embedding % head_dim == 0);

		auto x = graph.addInput(input_shape);

//		auto pos_enc = conv_bn_relu(graph, x, 32, 1);
//		pos_enc = graph.add(ml::Dense(128, "relu"), pos_enc);
//		auto pos_enc = graph.add(ml::Dense(num_heads * square(height * width)), x);

		x = conv_bn_relu(graph, x, embedding, 5);
//		for (int i = 0; i < 4; i++)
//		{
//			auto y = conv_bn_relu(graph, x, embedding / 2, 3);
//			y = conv_bn(graph, y, embedding / 2, 3);
//			x = graph.add(ml::Add("relu"), { x, y });

//			x = mha_pre_norm_block(graph, x, embedding / 2, head_dim, pos_encoding_range, false);
//			x = ffn_pre_norm_block(graph, x, embedding / 2);
//		}

//		x = conv_bn_relu(graph, x, embedding, 1);
		for (int i = 0; i < blocks; i++)
		{
//			auto y = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), x);
//			y = graph.add(ml::BatchNormalization().useGamma(false), y);
////			y = graph.add(ml::PositionalEncoding(), y);
//			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range, false), y);
//			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
//			y = graph.add(ml::BatchNormalization().useGamma(false), y);
//			x = graph.add(ml::Add("relu"), { x, y });
////			x = graph.add(ml::RMSNormalization(), x);
//
//			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);
//			y = graph.add(ml::BatchNormalization("relu").useGamma(false), y);
//			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
//			y = graph.add(ml::BatchNormalization("linear").useGamma(false), y);
//			x = graph.add(ml::Add("relu"), { x, y });
////			x = graph.add(ml::RMSNormalization(), x);
//
//			auto y = graph.add(ml::RMSNormalization(), x);
//			y = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), y);
////			y = graph.add(ml::PositionalEncoding(), y);
//			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range, false), y);
//			y = graph.add(ml::Conv2D(embedding, 1).useBias(false), y);
//			x = graph.add(ml::Add("relu"), { x, y });
//
//			y = graph.add(ml::RMSNormalization(), x);
//			y = graph.add(ml::Conv2D(embedding, 1, "relu"), y);
//			y = graph.add(ml::Conv2D(embedding, 1), y);
//			x = graph.add(ml::Add(), { x, y });

//			auto y = conv_bn_relu(graph, x, embedding / 2, 1);
//			y = conv_bn_relu(graph, y, embedding / 2, 3);
//			y = conv_bn(graph, y, embedding, 3);
//			x = graph.add(ml::Add("relu"), { x, y });

//			auto y = graph.add(ml::PositionalEncoding(), x);
//			y = conv_bn(graph, y, (3 - symmetric) * embedding, 1);
//			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range, symmetric), y);
//			y = conv_bn(graph, y, embedding, 1);
//			x = graph.add(ml::Add(), { x, y });
//
//			auto z = conv_bn_relu(graph, x, embedding, 1);
//			z = conv_bn(graph, z, embedding, 1);
//			x = graph.add(ml::Add(), { x, z });

//			x = mha_pre_norm_block(graph, x, embedding, head_dim, pos_encoding_range, false);

//			const int shift = (i % 2 == 0) ? 0 : 4;

			auto y = graph.add(ml::RMSNormalization(false), x);
			y = graph.add(ml::PositionalEncoding(), y);
			y = graph.add(ml::Conv2D((3 - symmetric) * embedding, 1).useBias(false), y);
//			y = graph.add(ml::WindowPartitioning(8, shift), y);
			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range, symmetric), y);
//			y = graph.add(ml::WindowMerging(ml::Shape( { batch_size, height, width, embedding }), shift), y);
			y = graph.add(ml::Conv2D(embedding, 1), y);
			x = graph.add(ml::Add(), { x, y });

			auto z = graph.add(ml::RMSNormalization(false), x);
			z = graph.add(ml::Conv2D(embedding, 1, "relu"), z);
			z = graph.add(ml::Conv2D(embedding, 1), z);
			x = graph.add(ml::Add(), { x, z });

//			auto y = graph.add(ml::PositionalEncoding(), x);
//			y = graph.add(ml::Conv2D((3 - symmetric) * embedding, 1).useBias(false), y);
//			y = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range, symmetric), y);
//			y = graph.add(ml::Conv2D(embedding, 1), y);
//			y = graph.add(ml::RMSNormalization(false), y);
//			x = graph.add(ml::Add(), { x, y });

//			x = ffn_pre_norm_block(graph, x, embedding);
		}

		auto p = conv_bn_relu(graph, x, embedding, 3);
		p = graph.add(ml::Conv2D(1, 1, "linear"), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p);

//		auto v = graph.add(ml::GlobalPooling(), x);
//		v = graph.add(ml::Dense(256, "linear").useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3, "linear"), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v);

//		auto p = graph.add(ml::RMSNormalization(), x);
//		p = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), p);
//		p = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), p);
//		p = graph.add(ml::Conv2D(1, 1), p);
//		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
//		graph.addOutput(p);
//
//		auto v = graph.add(ml::GlobalPooling(), x);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ConvUnet::ConvUnet() noexcept :
			AGNetwork()
	{
	}
	std::string ConvUnet::getOutputConfig() const
	{
		return "pv";
	}
	std::string ConvUnet::name() const
	{
		return "ConvUnet";
	}
	/*
	 * private
	 */
	void ConvUnet::create_network(const TrainingConfig &trainingOptions)
	{
		const int height = game_config.rows;
		const int width = game_config.cols;

		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, height, width, 32 });
		const int blocks = trainingOptions.blocks;
		const int embedding = trainingOptions.filters;
		const int head_dim = 32;
		const int pos_encoding_range = std::max(height, width);
		assert(embedding % head_dim == 0);

		auto x = createInputBlock(graph, input_shape, embedding);

		const int size_2h = (height + 1) / 2;
		const int size_2w = (width + 1) / 2;

		auto y = conv_bn_relu(graph, x, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		auto level0 = graph.add(ml::Add(), { x, y });
		x = graph.add(ml::SpaceToDepth(2), level0);
		x = graph.add(ml::Conv2D(embedding * 2, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		y = conv_bn_relu(graph, x, embedding * 2, 3);
		y = conv_bn_relu(graph, y, embedding * 2, 3);
		y = conv_bn_relu(graph, y, embedding * 2, 3);
		auto level1 = graph.add(ml::Add(), { x, y });
		x = graph.add(ml::SpaceToDepth(2), level1);
		x = graph.add(ml::Conv2D(embedding * 4, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		y = conv_bn_relu(graph, x, embedding * 4, 3);
		y = conv_bn_relu(graph, y, embedding * 4, 3);
		x = graph.add(ml::Add(), { x, y });

		y = conv_bn_relu(graph, x, embedding * 4, 3);
		y = conv_bn_relu(graph, y, embedding * 4, 3);
		x = graph.add(ml::Add(), { x, y });

		x = graph.add(ml::DepthToSpace(2, { size_2h, size_2w }), x);
		x = graph.add(ml::Conv2D(embedding * 2, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		x = graph.add(ml::Add(), { x, level1 });
		y = conv_bn_relu(graph, x, embedding * 2, 3);
		y = conv_bn_relu(graph, y, embedding * 2, 3);
		y = conv_bn_relu(graph, y, embedding * 2, 3);
		x = graph.add(ml::Add(), { x, y });

		x = graph.add(ml::DepthToSpace(2, { height, width }), x);
		x = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		x = graph.add(ml::Add(), { x, level0 });
		y = conv_bn_relu(graph, x, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		x = graph.add(ml::Add(), { x, y });

		auto p = graph.add(ml::RMSNormalization(), x);
		p = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), p);
		p = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), p);
		p = graph.add(ml::Conv2D(1, 1), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p);

//		auto v = graph.add(ml::GlobalPooling(), x);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	TransformerUnet::TransformerUnet() noexcept :
			AGNetwork()
	{
	}
	std::string TransformerUnet::getOutputConfig() const
	{
		return "pv";
	}
	std::string TransformerUnet::name() const
	{
		return "TransformerUnet";
	}
	/*
	 * private
	 */
	void TransformerUnet::create_network(const TrainingConfig &trainingOptions)
	{
		const int height = game_config.rows;
		const int width = game_config.cols;

		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, height, width, 32 });
		const int blocks = trainingOptions.blocks;
		const int embedding = trainingOptions.filters;
		const int head_dim = 32;
		const int pos_encoding_range = std::max(height, width);
		assert(embedding % head_dim == 0);

		auto x = createInputBlock(graph, input_shape, embedding);

		const int size_2h = (height + 1) / 2;
		const int size_2w = (width + 1) / 2;

		auto y = conv_bn_relu(graph, x, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		auto level0 = graph.add(ml::Add(), { x, y });
		x = graph.add(ml::SpaceToDepth(2), level0);
		x = graph.add(ml::Conv2D(embedding * 2, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = mha_pre_norm_block(graph, x, embedding * 2, head_dim, pos_encoding_range / 2, false);
		auto level1 = mha_pre_norm_block(graph, x, embedding * 2, head_dim, pos_encoding_range / 2, false);
		x = graph.add(ml::SpaceToDepth(2), level1);
		x = graph.add(ml::Conv2D(embedding * 4, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = mha_pre_norm_block(graph, x, embedding * 4, head_dim, pos_encoding_range / 4, false);
		x = mha_pre_norm_block(graph, x, embedding * 4, head_dim, pos_encoding_range / 4, false);
		x = mha_pre_norm_block(graph, x, embedding * 4, head_dim, pos_encoding_range / 4, false);
		x = mha_pre_norm_block(graph, x, embedding * 4, head_dim, pos_encoding_range / 4, false);

		x = graph.add(ml::DepthToSpace(2, { size_2h, size_2w }), x);
		x = graph.add(ml::Conv2D(embedding * 2, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		x = graph.add(ml::Add(), { x, level1 });
		x = mha_pre_norm_block(graph, x, embedding * 2, head_dim, pos_encoding_range / 2, false);
		x = mha_pre_norm_block(graph, x, embedding * 2, head_dim, pos_encoding_range / 2, false);
		x = mha_pre_norm_block(graph, x, embedding * 2, head_dim, pos_encoding_range / 2, false);

		x = graph.add(ml::DepthToSpace(2, { height, width }), x);
		x = graph.add(ml::Conv2D(embedding, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);
		x = graph.add(ml::Add(), { x, level0 });
		y = conv_bn_relu(graph, x, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		y = conv_bn_relu(graph, y, embedding, 3);
		x = graph.add(ml::Add(), { x, y });

		auto p = graph.add(ml::RMSNormalization(), x);
		p = graph.add(ml::Conv2D(3 * embedding, 1).useBias(false), p);
		p = graph.add(ml::MultiHeadAttention(embedding / head_dim, pos_encoding_range), p);
		p = graph.add(ml::Conv2D(1, 1), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p);

//		auto v = graph.add(ml::GlobalPooling(), x);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(embedding).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v);

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	BottleneckPVUM::BottleneckPVUM() noexcept :
			AGNetwork()
	{
	}
	std::string BottleneckPVUM::getOutputConfig() const
	{
		return "pvm";
	}
	std::string BottleneckPVUM::name() const
	{
		return "BottleneckPVUM";
	}
	void BottleneckPVUM::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 32 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = createInputBlock(graph, input_shape, filters);

		for (int i = 0; i < blocks; i++)
			x = createBottleneckBlock_v3(graph, x, filters);

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);
		p = graph.add(ml::Conv2D(1, 1, "linear"), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }), p);
		graph.addOutput(p, ml::CrossEntropyLoss());

//		auto common = graph.add(ml::GlobalPooling(), x);
//		common = graph.add(ml::Dense(256).useBias(false), common);
//		common = graph.add(ml::BatchNormalization("relu").useGamma(false), common);
//
//		// value head
//		auto v = graph.add(ml::Dense(256).useBias(false), common);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v, ml::CrossEntropyLoss());
//
//		// uncertainty head
//		auto unc = graph.add(ml::Dense(128).useBias(false), common);
//		unc = graph.add(ml::BatchNormalization("relu").useGamma(false), unc);
//		unc = graph.add(ml::Dense(1, "sigmoid"), unc);
//		graph.addOutput(unc, ml::MeanSquaredLoss(1.0f));
//
//		// moves left head
//		auto mlh = graph.add(ml::Dense(128).useBias(false), common);
//		mlh = graph.add(ml::BatchNormalization("relu").useGamma(false), mlh);
//		mlh = graph.add(ml::Dense(50), mlh);
//		mlh = graph.add(ml::Softmax( { 1 }), mlh);
//		graph.addOutput(mlh, ml::CrossEntropyLoss(0.1f));

//		auto global_pooling = graph.add(ml::GlobalPooling(), x);
//
//		//value head
//		auto v = graph.add(ml::Dense(std::min(256, 2 * filters)).useBias(false), global_pooling);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(std::min(256, 2 * filters)).useBias(false), v);
//		v = graph.add(ml::BatchNormalization("relu").useGamma(false), v);
//		v = graph.add(ml::Dense(3), v);
//		v = graph.add(ml::Softmax( { 1 }), v);
//		graph.addOutput(v, ml::CrossEntropyLoss());
//
//		// minimax estimate with uncertainty
//		auto unc = graph.add(ml::Dense(128).useBias(false), global_pooling);
//		unc = graph.add(ml::BatchNormalization("relu").useGamma(false), unc);
//		unc = graph.add(ml::Dense(128).useBias(false), unc);
//		unc = graph.add(ml::BatchNormalization("relu").useGamma(false), unc);
//		unc = graph.add(ml::Dense(1, "sigmoid"), unc);
//		graph.addOutput(unc, ml::MeanSquaredLoss(1.0f));
//
//		// moves left head
//		auto mlh = graph.add(ml::Dense(64).useBias(false), global_pooling);
//		mlh = graph.add(ml::BatchNormalization("relu").useGamma(false), mlh);
//		mlh = graph.add(ml::Dense(64).useBias(false), mlh);
//		mlh = graph.add(ml::BatchNormalization("relu").useGamma(false), mlh);
//		mlh = graph.add(ml::Dense(50), mlh);
//		mlh = graph.add(ml::Softmax( { 1 }), mlh);
//		graph.addOutput(mlh, ml::CrossEntropyLoss(0.1f));

		graph.init();
		graph.setOptimizer(ml::RAdam());
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ConvNextPVraw::ConvNextPVraw() noexcept :
			AGNetwork()
	{
	}
	std::string ConvNextPVraw::getOutputConfig() const
	{
		return "pv";
	}
	std::string ConvNextPVraw::name() const
	{
		return "ConvNextPVraw";
	}
	void ConvNextPVraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = graph.addInput(input_shape);
		x = graph.add(ml::Conv2D(filters, 5).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = squeeze_and_excitation_block(graph, x, filters);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::DepthwiseConv2D(filters, 7).useBias(false), x);
			y = graph.add(ml::BatchNormalization().useGamma(false), y);
			y = graph.add(ml::Conv2D(filters, 1, "relu"), y);
			x = graph.add(ml::Conv2D(filters, 1), { y, x });

			x = squeeze_and_excitation_block(graph, x, filters);
		}

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 1).useBias(false).quantizable(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);
		p = squeeze_and_excitation_block(graph, p, filters);
		p = graph.add(ml::Conv2D(1, 1).quantizable(false), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }).quantizable(false), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(filters, 1, "relu").quantizable(false), x);
		v = graph.add(ml::GlobalAveragePooling().quantizable(false), v);
		v = graph.add(ml::Dense(256).useBias(false).quantizable(false), v);
		v = graph.add(ml::BatchNormalization("relu"), v);
		v = graph.add(ml::Dense(3).quantizable(false), v);
		v = graph.add(ml::Softmax( { 1 }).quantizable(false), v);
		graph.addOutput(v);

		// moves left head
//		auto mlh = graph.add(ml::Conv2D(32, 1, "relu"), x);
//		mlh = graph.add(ml::GlobalPooling(), mlh);
//		mlh = graph.add(ml::Dense(128).useBias(false), mlh);
//		mlh = graph.add(ml::BatchNormalization("relu"), mlh);
//		mlh = graph.add(ml::Dense(50), mlh);
//		mlh = graph.add(ml::Softmax( { 1 }), mlh);
//		graph.addOutput(mlh, ml::CrossEntropyLoss(0.1f));

		graph.init();
		graph.setOptimizer(ml::RAdam(0.001f, 0.9f, 0.999f));
//		if (trainingOptions.l2_regularization != 0.0)
//			graph.setRegularizer(ml::RegularizerL2(trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ConvNextPVQraw::ConvNextPVQraw() noexcept :
			AGNetwork()
	{
	}
	std::string ConvNextPVQraw::getOutputConfig() const
	{
		return "pvq";
	}
	std::string ConvNextPVQraw::name() const
	{
		return "ConvNextPVQraw";
	}
	void ConvNextPVQraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = graph.addInput(input_shape);
		x = graph.add(ml::Conv2D(filters, 5).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = squeeze_and_excitation_block(graph, x, filters);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::DepthwiseConv2D(filters, 7).useBias(false), x);
			y = graph.add(ml::BatchNormalization().useGamma(false), y);
			y = graph.add(ml::Conv2D(filters, 1, "relu"), y);
			x = graph.add(ml::Conv2D(filters, 1), { y, x });

			x = squeeze_and_excitation_block(graph, x, filters);
		}

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 1).useBias(false).quantizable(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);
		p = squeeze_and_excitation_block(graph, p, filters);
		p = graph.add(ml::Conv2D(1, 1).quantizable(false), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }).quantizable(false), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(filters, 1, "relu").quantizable(false), x);
		v = graph.add(ml::GlobalAveragePooling().quantizable(false), v);
		v = graph.add(ml::Dense(256).useBias(false).quantizable(false), v);
		v = graph.add(ml::BatchNormalization("leaky_relu"), v);
		v = graph.add(ml::Dense(3).quantizable(false), v);
		v = graph.add(ml::Softmax( { 1 }).quantizable(false), v);
		graph.addOutput(v);

		auto q = graph.add(ml::Conv2D(filters, 1, "linear").useBias(false).quantizable(false), x);
		q = graph.add(ml::BatchNormalization("relu").useGamma(false), q);
		q = squeeze_and_excitation_block(graph, q, filters);
		q = graph.add(ml::Conv2D(3, 1, "linear").quantizable(false), q);
		q = graph.add(ml::Softmax( { 3 }).quantizable(false), q);
		graph.addOutput(q);

		graph.init();
		graph.setOptimizer(ml::RAdam(0.001f, 0.9f, 0.999f, trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

	ConvNextPVQMraw::ConvNextPVQMraw() noexcept :
			AGNetwork()
	{
	}
	std::string ConvNextPVQMraw::getOutputConfig() const
	{
		return "pvqm";
	}
	std::string ConvNextPVQMraw::name() const
	{
		return "ConvNextPVQMraw";
	}
	void ConvNextPVQMraw::create_network(const TrainingConfig &trainingOptions)
	{
		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, game_config.rows, game_config.cols, 8 });
		const int blocks = trainingOptions.blocks;
		const int filters = trainingOptions.filters;

		auto x = graph.addInput(input_shape);
		x = graph.add(ml::Conv2D(filters, 5).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		x = squeeze_and_excitation_block(graph, x, filters);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::DepthwiseConv2D(filters, 7).useBias(false), x);
			y = graph.add(ml::BatchNormalization().useGamma(false), y);
			y = graph.add(ml::Conv2D(filters, 1, "relu"), y);
			x = graph.add(ml::Conv2D(filters, 1), { y, x });

			x = squeeze_and_excitation_block(graph, x, filters);
		}

		// policy head
		auto p = graph.add(ml::Conv2D(filters, 1).useBias(false).quantizable(false), x);
		p = graph.add(ml::BatchNormalization("relu").useGamma(false), p);
		p = squeeze_and_excitation_block(graph, p, filters);
		p = graph.add(ml::Conv2D(1, 1).quantizable(false), p);
		p = graph.add(ml::Softmax( { 1, 2, 3 }).quantizable(false), p);
		graph.addOutput(p);

		// value head
		auto v = graph.add(ml::Conv2D(filters, 1, "relu").quantizable(false), x);
		v = graph.add(ml::GlobalAveragePooling().quantizable(false), v);
		v = graph.add(ml::Dense(256).useBias(false).quantizable(false), v);
		v = graph.add(ml::BatchNormalization("leaky_relu"), v);
		v = graph.add(ml::Dense(3).quantizable(false), v);
		v = graph.add(ml::Softmax( { 1 }).quantizable(false), v);
		graph.addOutput(v);

		auto q = graph.add(ml::Conv2D(filters, 1, "linear").useBias(false).quantizable(false), x);
		q = graph.add(ml::BatchNormalization("relu").useGamma(false), q);
		q = squeeze_and_excitation_block(graph, q, filters);
		q = graph.add(ml::Conv2D(3, 1, "linear").quantizable(false), q);
		q = graph.add(ml::Softmax( { 3 }).quantizable(false), q);
		graph.addOutput(q);

		// moves left head
		auto mlh = graph.add(ml::Conv2D(32, 1, "relu"), x);
		mlh = graph.add(ml::GlobalAveragePooling(), mlh);
		mlh = graph.add(ml::Dense(128).useBias(false), mlh);
		mlh = graph.add(ml::BatchNormalization("leaky_relu"), mlh);
		mlh = graph.add(ml::Dense(50), mlh);
		mlh = graph.add(ml::Softmax( { 1 }), mlh);
		graph.addOutput(mlh, ml::CrossEntropyLoss(), 0.25f);

		graph.init();
		graph.setOptimizer(ml::RAdam(0.001f, 0.9f, 0.999f, trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

} /* namespace ag */

