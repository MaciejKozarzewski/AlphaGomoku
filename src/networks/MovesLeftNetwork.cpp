/*
 * MovesLeftNetwork.cpp
 *
 *  Created on: Oct 9, 2025
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/networks/MovesLeftNetwork.hpp>
#include <alphagomoku/networks/NNInputFeatures.hpp>
#include <alphagomoku/networks/blocks.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>

#include <alphagomoku/networks/networks.hpp>

#include <minml/graph/graph_optimizers.hpp>
#include <minml/core/Device.hpp>
#include <minml/core/Context.hpp>
#include <minml/core/Event.hpp>
#include <minml/core/Shape.hpp>
#include <minml/core/Tensor.hpp>
#include <minml/core/math.hpp>
#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>
#include <minml/utils/testing_util.hpp>
#include <minml/layers/Conv2D.hpp>
#include <minml/layers/DepthwiseConv2D.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/SpaceToDepth.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/GlobalAveragePooling.hpp>
#include <minml/layers/Softmax.hpp>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <bitset>
#include <memory>

namespace
{
	float one_if(bool cond) noexcept
	{
		return cond ? 1.0f : 0.0f;
	}
	int arg_max(const ag::matrix<float> &m, int row) noexcept
	{
		float max_value = m.at(row, 0);
		int idx = 0;
		for (int c = 1; c < m.cols(); c++)
			if (m.at(row, c) > max_value)
			{
				max_value = m.at(row, c);
				idx = c;
			}
		return idx;
	}
}

namespace ag
{

	MovesLeftNetwork::MovesLeftNetwork() noexcept :
			pattern_calculator(GameConfig())
	{
	}
	MovesLeftNetwork::MovesLeftNetwork(const GameConfig &gameOptions, const TrainingConfig &trainingOptions) noexcept :
			pattern_calculator(gameOptions)
	{
		game_config = gameOptions;
		create_network(trainingOptions);
		allocate_tensors();
	}
	std::vector<float> MovesLeftNetwork::getAccuracy(int batchSize, int top_k) const
	{
		std::vector<float> result(1 + top_k);
		matrix<float> output(batchSize, 1 + game_config.rows * game_config.cols);
		matrix<float> target(batchSize, 1 + game_config.rows * game_config.cols);

		ml::convertType(ml::Context(), output.data(), ml::DataType::FLOAT32, output_on_cpu.data(), output_on_cpu.dtype(), output_on_cpu.volume());
		ml::convertType(ml::Context(), target.data(), ml::DataType::FLOAT32, target_on_cpu.data(), target_on_cpu.dtype(), target_on_cpu.volume());

		for (int b = 0; b < batchSize; b++)
		{
			const int correct = arg_max(target, b);
			for (int l = 0; l < top_k; l++)
			{
				const int best = arg_max(output, b);
				if (correct == best)
					for (int m = l; m < top_k; m++)
						result[1 + m] += 1;
				output.at(b, best) = 0.0f;
			}
			result[0] += 1;
		}
		return result;
	}
	void MovesLeftNetwork::packInputData(int index, const matrix<Sign> &board, Sign signToMove, GameRules rules)
	{
		/*
		 * Input features are:
		 *  Channel	Description
		 *  0		legal move
		 *  1		own stone
		 *  2		opponent stone
		 *  3 		ones (constant channel)
		 *  4 		black (cross) to move
		 *  5 		white (circle) to move
		 *  6 		forbidden move
		 *  7		freestyle rule (five or more)
		 *  8		standard rule (exactly five)
		 *  9		renju rule (exactly five with extra rules for black)
		 *  10		caro5 rule	(exactly five and unblocked)
		 *  11		caro6 rule (five or more and unblocked)
		 */

		pattern_calculator.setBoard(board, signToMove);

		for (int row = 0; row < board.rows(); row++)
			for (int col = 0; col < board.cols(); col++)
			{
				input_on_cpu.at( { index, row, col, 0 }) = one_if(board.at(row, col) == Sign::NONE);
				input_on_cpu.at( { index, row, col, 1 }) = one_if(board.at(row, col) == signToMove);
				input_on_cpu.at( { index, row, col, 2 }) = one_if(board.at(row, col) == invertSign(signToMove));
				input_on_cpu.at( { index, row, col, 3 }) = 1.0f;
				input_on_cpu.at( { index, row, col, 4 }) = one_if(signToMove == Sign::CROSS);
				input_on_cpu.at( { index, row, col, 5 }) = one_if(signToMove == Sign::CIRCLE);
				input_on_cpu.at( { index, row, col, 6 }) = one_if(pattern_calculator.isForbidden(signToMove, row, col));
				input_on_cpu.at( { index, row, col, 7 }) = one_if(rules == GameRules::FREESTYLE);
				input_on_cpu.at( { index, row, col, 8 }) = one_if(rules == GameRules::STANDARD);
				input_on_cpu.at( { index, row, col, 9 }) = one_if(rules == GameRules::RENJU);
				input_on_cpu.at( { index, row, col, 10 }) = one_if(rules == GameRules::CARO5);
				input_on_cpu.at( { index, row, col, 11 }) = one_if(rules == GameRules::CARO6);
			}
	}
	void MovesLeftNetwork::packTarget(int index, int movesLeft)
	{
		for (int i = 0; i < target_on_cpu.lastDim(); i++)
			target_on_cpu.at( { index, i }) = 0.0f; // clear all entries
		target_on_cpu.at( { index, movesLeft }) = 1.0f;
	}
	void MovesLeftNetwork::unpackOutput(int index, float &movesLeft) const
	{
		float result = 0.0f;
		for (int i = 0; i < output_on_cpu.lastDim(); i++)
		{
			std::cout << i << " " << (float) output_on_cpu.at( { index, i }) << '\n';
			result += (float) output_on_cpu.at( { index, i }) * (float) i;
		}
		movesLeft = result;
	}

	void MovesLeftNetwork::forward(int batch_size)
	{
		graph.getInput().copyFrom(graph.context(), input_on_cpu);
		graph.predict(batch_size);
		output_on_cpu.copyFrom(graph.context(), graph.getOutput());
		graph.context().synchronize();
	}
	std::vector<float> MovesLeftNetwork::train(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getInput().copyFrom(graph.context(), input_on_cpu);
		graph.getTarget().copyFrom(graph.context(), target_on_cpu);

		const bool success = graph.train(batch_size);
		output_on_cpu.copyFrom(graph.context(), graph.getOutput());
		graph.context().synchronize();
		if (success)
			return graph.getLoss(batch_size);
		else
			return std::vector<float>();
	}
	std::vector<float> MovesLeftNetwork::getLoss(int batch_size)
	{
		assert(graph.isTrainable());
		graph.getTarget().copyFrom(graph.context(), target_on_cpu);
		return graph.getLoss(batch_size);
	}

	void MovesLeftNetwork::changeLearningRate(float lr)
	{
		assert(graph.isTrainable());
		graph.getOptimizer().setLearningRate(lr);
	}
	void MovesLeftNetwork::optimize(int level)
	{
		graph.makeTrainable(false);
		if (level >= 1)
		{
			ml::FoldBatchNorm().optimize(graph);
			ml::FoldAdd().optimize(graph);
		}
		if (level >= 2)
		{
			ml::FuseConvBlock().optimize(graph);
			ml::FuseSEBlock().optimize(graph);
		}
	}
	void MovesLeftNetwork::convertToHalfFloats()
	{
		ml::DataType new_type = ml::DataType::FLOAT32;
		if (graph.device().supportsType(ml::DataType::FLOAT16))
			new_type = ml::DataType::FLOAT16;
		if (new_type != ml::DataType::FLOAT32)
		{
			graph.convertTo(new_type);
			allocate_tensors();
		}
	}
	void MovesLeftNetwork::saveToFile(const std::string &path) const
	{
		graph.context().synchronize();
		SerializedObject so;
		Json json;
		json["architecture"] = "MovesLeftNetwork";
		json["config"] = game_config.toJson();
		json["model"] = graph.save(so);
		FileSaver fs(path);
		fs.save(json, so, 2);
	}
	void MovesLeftNetwork::loadFromFile(const std::string &path)
	{
		FileLoader fl(path);
		loadFrom(fl.getJson(), fl.getBinaryData());
	}
	void MovesLeftNetwork::loadFrom(const Json &json, const SerializedObject &so)
	{
		if (json["architecture"].getString() != "MovesLeftNetwork")
			throw std::runtime_error(
					"MovesLeftNetwork::loadFromFile() : saved model has architecture '" + json["architecture"].getString()
							+ "' and cannot be loaded into model 'MovesLeftNetwork'");

		unloadGraph();
		game_config = GameConfig(json["config"]);
		graph.load(json["model"], so);
	}
	void MovesLeftNetwork::unloadGraph()
	{
		synchronize();
		graph.clear();
	}
	bool MovesLeftNetwork::isLoaded() const noexcept
	{
		return graph.numberOfNodes() > 0;
	}
	void MovesLeftNetwork::synchronize()
	{
		graph.context().synchronize();
	}
	void MovesLeftNetwork::moveTo(ml::Device device)
	{
		graph.moveTo(device);
		allocate_tensors();
	}

	int MovesLeftNetwork::getBatchSize() const noexcept
	{
		return graph.getInputShape().firstDim();
	}
	void MovesLeftNetwork::setBatchSize(int batchSize)
	{
		if (batchSize != graph.getInputShape().firstDim())
		{ // get here only if the batch size was actually changed
			ml::Shape shape = graph.getInputShape();
			shape[0] = batchSize;
			graph.setInputShape(shape);
			allocate_tensors();
		}
	}
	GameConfig MovesLeftNetwork::getGameConfig() const noexcept
	{
		return game_config;
	}
	ml::Event MovesLeftNetwork::addEvent() const
	{
		return graph.context().createEvent();
	}
	/*
	 * private
	 */
	ml::Shape MovesLeftNetwork::get_input_shape() const noexcept
	{
		return ml::Shape( { getBatchSize(), game_config.rows, game_config.cols, 12 });
	}
	void MovesLeftNetwork::allocate_tensors()
	{
		input_on_cpu = ml::Tensor( { getBatchSize(), game_config.rows, game_config.cols, 12 });
		output_on_cpu = ml::Tensor( { getBatchSize(), 1 + game_config.rows * game_config.cols });
		target_on_cpu = ml::zeros_like(output_on_cpu);
		if (graph.device().isCUDA())
		{
			input_on_cpu.pageLock();
			output_on_cpu.pageLock();
			target_on_cpu.pageLock();
		}
	}
	void MovesLeftNetwork::create_network(const TrainingConfig &trainingOptions)
	{
		const int height = game_config.rows;
		const int width = game_config.cols;

		const ml::Shape input_shape( { trainingOptions.device_config.batch_size, height, width, 12 });
		const int blocks = trainingOptions.blocks;
		const int channels = trainingOptions.filters;
		const int channels_x2 = channels * 2;
		const int channels_x4 = channels * 4;

		auto x = createInputBlock(graph, input_shape, channels);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::DepthwiseConv2D(channels, 7).useBias(false), x);
			y = graph.add(ml::BatchNormalization().useGamma(false), y);
			y = graph.add(ml::Conv2D(channels, 1, "relu"), y);
			x = graph.add(ml::Conv2D(channels, 1), { y, x });

			x = squeeze_and_excitation_block(graph, x, channels);
		}

		x = graph.add(ml::SpaceToDepth(2), x);
		x = graph.add(ml::Conv2D(channels_x2, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::DepthwiseConv2D(channels_x2, 7).useBias(false), x);
			y = graph.add(ml::BatchNormalization().useGamma(false), y);
			y = graph.add(ml::Conv2D(channels_x2, 1, "relu"), y);
			x = graph.add(ml::Conv2D(channels_x2, 1), { y, x });

			x = squeeze_and_excitation_block(graph, x, channels_x2);
		}

		x = graph.add(ml::SpaceToDepth(2), x);
		x = graph.add(ml::Conv2D(channels_x4, 1).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu").useGamma(false), x);

		for (int i = 0; i < blocks; i++)
		{
			auto y = graph.add(ml::DepthwiseConv2D(channels_x4, 7).useBias(false), x);
			y = graph.add(ml::BatchNormalization().useGamma(false), y);
			y = graph.add(ml::Conv2D(channels_x4, 1, "relu"), y);
			x = graph.add(ml::Conv2D(channels_x4, 1), { y, x });

			x = squeeze_and_excitation_block(graph, x, channels_x4);
		}

		x = conv_bn_relu(graph, x, channels_x4, 3);
		x = graph.add(ml::GlobalAveragePooling(), x);
		x = graph.add(ml::Dense(channels_x4).useBias(false), x);
		x = graph.add(ml::BatchNormalization("relu"), x);
		x = graph.add(ml::Dense(1 + height * width), x);
		x = graph.add(ml::Softmax( { 1 }), x);
		graph.addOutput(x);

		graph.init();
		graph.setOptimizer(ml::RAdam(0.001f, 0.9f, 0.999f, trainingOptions.l2_regularization));
		graph.moveTo(trainingOptions.device_config.device);
	}

} /* namespace ag */

