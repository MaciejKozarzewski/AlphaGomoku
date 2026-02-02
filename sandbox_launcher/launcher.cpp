/*
 * launcher.cpp
 *
 *  Created on: Feb 26, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/dataset/data_packs.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/dataset/GameDataStorage.hpp>
#include <alphagomoku/dataset/SearchDataStorage.hpp>
#include <alphagomoku/dataset/Sampler.hpp>
#include <alphagomoku/dataset/CompressedFloat.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/networks/networks.hpp>
#include <alphagomoku/networks/MovesLeftNetwork.hpp>
#include <alphagomoku/patterns/DefensiveMoveTable.hpp>
#include <alphagomoku/patterns/ThreatTable.hpp>
#include <alphagomoku/protocols/Protocol.hpp>
#include <alphagomoku/search/monte_carlo/Edge.hpp>
#include <alphagomoku/search/monte_carlo/EdgeSelector.hpp>
#include <alphagomoku/search/monte_carlo/NNEvaluator.hpp>
#include <alphagomoku/search/monte_carlo/Node.hpp>
#include <alphagomoku/search/monte_carlo/NodeCache.hpp>
#include <alphagomoku/search/monte_carlo/Search.hpp>
#include <alphagomoku/search/monte_carlo/SearchTask.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/selfplay/OpeningGenerator.hpp>
#include <alphagomoku/selfplay/SupervisedLearning.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/os_utils.hpp>
#include <alphagomoku/utils/Logger.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/augmentations.hpp>
#include <alphagomoku/utils/SpinLock.hpp>
#include <alphagomoku/utils/random.hpp>
#include <alphagomoku/utils/selfcheck.hpp>
#include <alphagomoku/protocols/GomocupProtocol.hpp>
#include <alphagomoku/networks/networks.hpp>
#include <alphagomoku/version.hpp>
#include <alphagomoku/search/monte_carlo/EdgeGenerator.hpp>
#include <alphagomoku/search/monte_carlo/Tree.hpp>
#include <alphagomoku/search/alpha_beta/MoveGenerator.hpp>
#include <alphagomoku/search/alpha_beta/MinimaxSearch.hpp>
#include <alphagomoku/search/alpha_beta/AlphaBetaSearch.hpp>
#include <alphagomoku/search/alpha_beta/VCFSolver.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/GeneratorManager.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/selfplay/EvaluationManager.hpp>
#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/selfplay/NetworkLoader.hpp>
#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/player/SearchThread.hpp>
#include <alphagomoku/player/EngineSettings.hpp>
#include <alphagomoku/player/EngineController.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/patterns/PatternTable.hpp>
#include <alphagomoku/patterns/PatternClassifier.hpp>
#include <alphagomoku/patterns/Pattern.hpp>
#include <alphagomoku/networks/NNUE.hpp>
#include <alphagomoku/networks/MovesLeftNetwork.hpp>
#include <alphagomoku/utils/bit_utils.hpp>
#include <minml/utils/ZipWrapper.hpp>
#include <alphagomoku/search/alpha_beta/ThreatSpaceSearch.hpp>
#include <alphagomoku/utils/low_precision.hpp>

#include <alphagomoku/search/pab/Search.hpp>

#include "minml/core/Device.hpp"
#include "minml/utils/json.hpp"
#include "minml/utils/serialization.hpp"
#include "minml/layers/Dense.hpp"
#include "minml/layers/BatchNormalization.hpp"
#include <minml/graph/graph_optimizers.hpp>
#include <minml/graph/CalibrationTable.hpp>
#include <minml/graph/swa_utils.hpp>
#include <minml/utils/testing_util.hpp>

#include <cstddef>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <numeric>
#include <memory>
#include <functional>
#include <x86intrin.h>

using namespace ag;

std::vector<std::vector<Move>> generate_openings(size_t number, GameConfig game_config, const std::string pathToNetwork)
{
	DeviceConfig device_config;
	device_config.batch_size = 256;
	device_config.device = ml::Device::cuda(0);

	NNEvaluator evaluator(device_config);
	evaluator.loadGraph(pathToNetwork);

	TSSConfig tss_config;
	AlphaBetaSearch solver(game_config);

	std::vector<std::vector<Move>> result;

	OpeningGenerator generator(game_config, 10);
	while (result.size() < number)
	{
		generator.generate(device_config.batch_size, evaluator, solver);
		evaluator.evaluateGraph();

		while (not generator.isEmpty() and result.size() < number)
			result.push_back(generator.pop());
	}

	return result;
}

void generate_games()
{
	setupSignalHandler(SignalType::INT, SignalHandlerMode::CUSTOM_HANDLER);
	const std::string working_directory = "/home/maciek/alphagomoku/new_runs_2025/standard_dataset/";
	const std::string path_to_network = "/home/maciek/alphagomoku/new_runs_2025/standard_pvqm_8x128/network_swa_opt.bin";

	if (not pathExists(working_directory))
		createDirectory(working_directory);
	if (not pathExists(working_directory + "buffers/"))
		createDirectory(working_directory + "buffers/");

	const MasterLearningConfig config(FileLoader(working_directory + "config.json").getJson());
	Json metadata;
	if (pathExists(working_directory + "metadata.json"))
		metadata = FileLoader(working_directory + "metadata.json").getJson();
	else
	{
		metadata["current_part"] = 0;
		metadata["total_parts"] = 20;
	}

	for (int i = metadata["current_part"].getInt(); i < metadata["total_parts"].getInt(); i++)
	{
		GeneratorManager generator_manager(config.game_config, config.generation_config);
		generator_manager.setWorkingDirectory(working_directory);
		generator_manager.generate(path_to_network, config.generation_config.games_per_iteration);
		if (hasCapturedSignal(SignalType::INT))
			break;
		metadata["current_part"] = i + 1;
		std::cout << "Finished generating games\n";

		std::cout << "Saving buffer\n";
		generator_manager.getGameBuffer().save(working_directory + "/buffers/buffer_" + std::to_string(i) + ".bin");
	}
	std::ofstream output;
	output.open(working_directory + "metadata.json", std::ios::out);
	output << metadata.dump(2) << '\n';
}

void run_training()
{
	ml::Device::setNumberOfThreads(4);
	const std::string path = "/home/maciek/alphagomoku/runs_2026/testy_bn/ffn_x4/";

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.network_arch = "ConvNextPVQMraw";
	training_config.augment_training_data = true;
	training_config.blocks = 8;
	training_config.filters = 128;
	training_config.patch_size = 1;
	training_config.device_config.batch_size = 128;
	training_config.l2_regularization = 2.0e-5f;
	training_config.sampler_type = "values";

	SupervisedLearning sl(training_config);

	std::unique_ptr<AGNetwork> network = createAGNetwork(training_config.network_arch);
	network->init(game_config, training_config);
//	network->convertToHalfFloats();
//	network->get_graph().setGradientScaler(ml::GradientScaler(1024.0f, 10000));

	int initial_epoch = 0;
	int max_epochs = 500;
	int swa_num_epochs = 20;
	int steps_per_epoch = 1000;
// @formatter:off
	Parameter<float> learning_rate( { std::pair<int, float> {   0, 1.0e-4f },
									  std::pair<int, float> {  25, 1.0e-3f },
									  std::pair<int, float> { 150, 1.0e-3f },
									  std::pair<int, float> { 250, 1.0e-4f },
									  std::pair<int, float> { 350, 1.0e-4f },
									  std::pair<int, float> { 450, 1.0e-5f }}, "cosine");
// @formatter:on

	if (not pathExists(path))
		createDirectory(path);
	else
	{
		if (pathExists(path + "sl_progress.json"))
		{
			FileLoader fl(path + "sl_progress.json");
			std::cout << "loaded progress:\n" << fl.getJson().dump(2) << '\n';
			sl.loadProgress(fl.getJson());
			initial_epoch = fl.getJson()["epoch"].getInt();
			max_epochs = fl.getJson()["max_epochs"].getInt();
			swa_num_epochs = fl.getJson()["swa_num_epochs"].getInt();
			steps_per_epoch = fl.getJson()["steps_per_epoch"].getInt();
			learning_rate = Parameter<float>(fl.getJson()["learning_rate_schedule"]);
		}
		if (pathExists(path + "network_progress.bin"))
			network = loadAGNetwork(path + "network_progress.bin");
	}

	network->moveTo(ml::Device::cuda());
	network->get_graph().context().enableTF32(true);
	network->get_graph().print();

	Dataset dataset;
#pragma omp parallel for
	for (int i = 200; i < 250; i++)
//	for (int i = 0; i < 20; i++)
	{
		std::cout << "loading buffer " << i << "...\n";
		dataset.load(i, "/media/maciek/Data Linux/alphagomoku/runs_2025/standard_15x15/train_buffer/buffer_" + std::to_string(i) + ".bin");
	}
	std::cout << dataset.getStats().toString() << '\n';

	for (int e = initial_epoch; e <= max_epochs; e++)
	{
		const double t0 = getTime();
		const float lr = learning_rate.getValue(e);
		std::cout << "epoch " << e << ", learning rate = " << lr << '\n';
		network->changeLearningRate(lr);
		sl.clearStats();
		sl.train(*network, dataset, steps_per_epoch);
		sl.saveTrainingHistory(path);

		if (e % 25 == 0)
			network->saveToFile(path + "/network_" + std::to_string(e) + ".bin");

		if (e >= max_epochs - swa_num_epochs)
		{
			network->saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
			std::unique_ptr<AGNetwork> opt = loadAGNetwork(path + "/network_" + std::to_string(e) + "_opt.bin");
			opt->moveTo(ml::Device::cuda());
			opt->optimize();
			opt->saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
		}

		Json tmp = sl.saveProgress();
		tmp["epoch"] = e + 1;
		tmp["max_epochs"] = max_epochs;
		tmp["swa_num_epochs"] = swa_num_epochs;
		tmp["steps_per_epoch"] = steps_per_epoch;
		tmp["learning_rate_schedule"] = learning_rate.toJson();
		FileSaver(path + "sl_progress.json").save(tmp, SerializedObject(), true);
		network->saveToFile(path + "/network_progress.bin");

		const double t1 = getTime();
		std::cout << "epoch time = " << (t1 - t0) << '\n';
	}

	std::unique_ptr<AGNetwork> swa_network = loadAGNetwork(path + "/network_" + std::to_string(max_epochs - swa_num_epochs) + "_opt.bin");
	int count = 0;
	for (int i = max_epochs - swa_num_epochs; i <= max_epochs; i++)
	{
		std::unique_ptr<AGNetwork> network = loadAGNetwork(path + "/network_" + std::to_string(i) + "_opt.bin");
		network->setBatchSize(1);
		network->moveTo(ml::Device::cpu());
		count++;
		const float alpha = 1.0f / count;
		ml::averageModelWeights(alpha, network->get_graph(), 1.0f - alpha, swa_network->get_graph());
	}
	swa_network->saveToFile(path + "/network_swa_opt.bin");
}

void run_knowledge_distillation()
{
	ml::Device::setNumberOfThreads(4);
	const std::string path = "/home/maciek/alphagomoku/runs_2026/test_kd/";

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.network_arch = "ConvNextPVQMraw";
	training_config.augment_training_data = true;
	training_config.blocks = 8;
	training_config.filters = 128;
	training_config.patch_size = 1;
	training_config.device_config.batch_size = 1536;
	training_config.l2_regularization = 2.0e-5f;
	training_config.sampler_type = "values";

	SupervisedLearning sl(training_config);

	std::unique_ptr<AGNetwork> teacher = loadAGNetwork("/home/maciek/alphagomoku/runs_2026/standard_v2/network_swa.bin");
	teacher->moveTo(ml::Device::cuda());
	teacher->optimize(2);
	teacher->setBatchSize(std::min(256, training_config.device_config.batch_size));
	teacher->convertToHalfFloats();

	std::unique_ptr<AGNetwork> student = createAGNetwork(training_config.network_arch);
	student->init(game_config, training_config);
//	network->convertToHalfFloats();
//	network->get_graph().setGradientScaler(ml::GradientScaler(1024.0f, 10000));

	int initial_epoch = 0;
	int max_epochs = 500;
	int swa_num_epochs = 20;
	int steps_per_epoch = 2000;
// @formatter:off
	Parameter<float> learning_rate( { std::pair<int, float> {   0, 1.0e-4f },
									  std::pair<int, float> {  25, 1.0e-3f },
									  std::pair<int, float> { 150, 1.0e-3f },
									  std::pair<int, float> { 250, 1.0e-4f },
									  std::pair<int, float> { 350, 1.0e-4f },
									  std::pair<int, float> { 450, 1.0e-5f }}, "cosine");
// @formatter:on

	if (not pathExists(path))
		createDirectory(path);
	else
	{
		if (pathExists(path + "sl_progress.json"))
		{
			FileLoader fl(path + "sl_progress.json");
			std::cout << "loaded progress:\n" << fl.getJson().dump(2) << '\n';
			sl.loadProgress(fl.getJson());
			initial_epoch = fl.getJson()["epoch"].getInt();
			max_epochs = fl.getJson()["max_epochs"].getInt();
			swa_num_epochs = fl.getJson()["swa_num_epochs"].getInt();
			steps_per_epoch = fl.getJson()["steps_per_epoch"].getInt();
			learning_rate = Parameter<float>(fl.getJson()["learning_rate_schedule"]);
		}
		if (pathExists(path + "network_progress.bin"))
			student = loadAGNetwork(path + "network_progress.bin");
	}

	student->moveTo(ml::Device::cuda());
	student->get_graph().context().enableTF32(true);
	student->get_graph().print();

	Dataset dataset;
#pragma omp parallel for
	for (int i = 350; i < 400; i++)
	{
		std::cout << "loading buffer " << i << "...\n";
		dataset.load(i, "/home/maciek/alphagomoku/runs_2026/standard_v2/train_buffer/buffer_" + std::to_string(i) + ".bin");
	}
	std::cout << dataset.getStats().toString() << '\n';

	for (int e = initial_epoch; e <= max_epochs; e++)
	{
		const double t0 = getTime();
		const float lr = learning_rate.getValue(e);
		std::cout << "epoch " << e << ", learning rate = " << lr << '\n';
		student->changeLearningRate(lr);
		sl.clearStats();
		sl.train(*teacher, *student, dataset, steps_per_epoch);
		sl.saveTrainingHistory(path);

		if (e % 25 == 0)
			student->saveToFile(path + "/network_" + std::to_string(e) + ".bin");

		if (e >= max_epochs - swa_num_epochs)
		{
			student->saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
			std::unique_ptr<AGNetwork> opt = loadAGNetwork(path + "/network_" + std::to_string(e) + "_opt.bin");
			opt->moveTo(ml::Device::cuda());
			opt->optimize();
			opt->saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
		}

		Json tmp = sl.saveProgress();
		tmp["epoch"] = e + 1;
		tmp["max_epochs"] = max_epochs;
		tmp["swa_num_epochs"] = swa_num_epochs;
		tmp["steps_per_epoch"] = steps_per_epoch;
		tmp["learning_rate_schedule"] = learning_rate.toJson();
		FileSaver(path + "sl_progress.json").save(tmp, SerializedObject(), true);
		student->saveToFile(path + "/network_progress.bin");

		const double t1 = getTime();
		std::cout << "epoch time = " << (t1 - t0) << '\n';
	}

	std::unique_ptr<AGNetwork> swa_network = loadAGNetwork(path + "/network_" + std::to_string(max_epochs - swa_num_epochs) + "_opt.bin");
	int count = 0;
	for (int i = max_epochs - swa_num_epochs; i <= max_epochs; i++)
	{
		std::unique_ptr<AGNetwork> network = loadAGNetwork(path + "/network_" + std::to_string(i) + "_opt.bin");
		network->setBatchSize(1);
		network->moveTo(ml::Device::cpu());
		count++;
		const float alpha = 1.0f / count;
		ml::averageModelWeights(alpha, network->get_graph(), 1.0f - alpha, swa_network->get_graph());
	}
	swa_network->saveToFile(path + "/network_swa_opt.bin");
}

class MovesLeftDatasetEntry
{
		std::vector<uint16_t> m_moves;
		GameRules m_rules;
	public:
		MovesLeftDatasetEntry(const GameDataStorage &gds, GameRules rules) :
				m_moves(gds.numberOfMoves()),
				m_rules(rules)
		{
			for (int i = 0; i < gds.numberOfMoves(); i++)
				m_moves[i] = gds.getMove(i).toShort();
		}

		int length() const noexcept
		{
			return m_moves.size();
		}
		GameRules rules() const noexcept
		{
			return m_rules;
		}
		Sign fill_board(matrix<Sign> &board, int move_index) const
		{
			assert(0 <= move_index && move_index < length());
			board.clear();
			for (int i = 0; i < move_index; i++)
				Board::putMove(board, Move(m_moves[i]));
			return Move(m_moves[move_index]).sign;
		}
};

void load_buffers(std::vector<MovesLeftDatasetEntry> &result, const std::string &path, int idx0, int idx1, GameRules rules)
{
	Dataset tmp;
#pragma omp parallel for
	for (int i = idx0; i < idx1; i++)
	{
		std::cout << "loading buffer " << i << "...\n";
		tmp.load(i, path + "train_buffer/buffer_" + std::to_string(i) + ".bin");
	}
	std::cout << tmp.getStats().toString();

	for (int i = idx0; i < idx1; i++)
		for (int j = 0; j < tmp.getBuffer(i).numberOfGames(); j++)
			result.emplace_back(tmp.getBuffer(i).getGameData(j), rules);
}

void train_moves_left_network()
{
	ml::Device::setNumberOfThreads(4);
	const std::string path = "/home/maciek/alphagomoku/runs_2026/moves_left_network_15x15_test/";

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.network_arch = "FastPolicy";
	training_config.augment_training_data = true;
	training_config.blocks = 4;
	training_config.filters = 64;
	training_config.patch_size = 1;
	training_config.device_config.batch_size = 128;
	training_config.l2_regularization = 2.0e-5f;
	training_config.sampler_type = "values";

	MovesLeftNetwork network(game_config, training_config);

	int initial_epoch = 0;
	int max_epochs = 500;
	int swa_num_epochs = 20;
	int steps_per_epoch = 1000;
// @formatter:off
	Parameter<float> learning_rate( { std::pair<int, float> {   0, 1.0e-4f },
									  std::pair<int, float> {  25, 1.0e-3f },
									  std::pair<int, float> { 150, 1.0e-3f },
									  std::pair<int, float> { 250, 1.0e-4f },
									  std::pair<int, float> { 350, 1.0e-4f },
									  std::pair<int, float> { 450, 1.0e-5f }}, "cosine");
// @formatter:on

	if (not pathExists(path))
		createDirectory(path);
	else
	{
		if (pathExists(path + "sl_progress.json"))
		{
			FileLoader fl(path + "sl_progress.json");
			std::cout << "loaded progress:\n" << fl.getJson().dump(2) << '\n';
			initial_epoch = fl.getJson()["epoch"].getInt();
			max_epochs = fl.getJson()["max_epochs"].getInt();
			swa_num_epochs = fl.getJson()["swa_num_epochs"].getInt();
			steps_per_epoch = fl.getJson()["steps_per_epoch"].getInt();
			learning_rate = Parameter<float>(fl.getJson()["learning_rate_schedule"]);
		}
		if (pathExists(path + "network_progress.bin"))
			network.loadFromFile(path + "network_progress.bin");
	}

	network.moveTo(ml::Device::cuda());
	network.get_graph().context().enableTF32(true);
	network.get_graph().print();

	std::vector<MovesLeftDatasetEntry> dataset;

	const std::string data_path = "/media/maciek/Data Linux/alphagomoku/runs_2025/";

	load_buffers(dataset, data_path + "freestyle_15x15/", 200, 250, GameRules::FREESTYLE);
	load_buffers(dataset, data_path + "standard_15x15/", 200, 250, GameRules::STANDARD);
	load_buffers(dataset, data_path + "renju_8x128/", 200, 250, GameRules::RENJU);
	load_buffers(dataset, data_path + "caro5/", 150, 250, GameRules::CARO5);
	load_buffers(dataset, data_path + "caro6/", 150, 250, GameRules::CARO6);
	std::cout << "Dataset contains " << dataset.size() << " entries\n";

	std::vector<int> sample_order(dataset.size());
	size_t order_index = sample_order.size();
	std::iota(sample_order.begin(), sample_order.end(), 0);

	for (int e = initial_epoch; e <= max_epochs; e++)
	{
		std::vector<float> accuracy(6, 0.0f);
		const double t0 = getTime();
		const float lr = learning_rate.getValue(e);
		std::cout << "epoch " << e << ", learning rate = " << lr << '\n';
		network.changeLearningRate(lr);

		ml::Device::cpu().setNumberOfThreads(1);

		const int batch_size = network.getBatchSize();
		for (int step = 0; step < steps_per_epoch; step++)
		{
			if ((step + 1) % std::max(1, (steps_per_epoch / 10)) == 0)
				std::cout << step + 1 << '\n';

			matrix<Sign> board(game_config.rows);
			for (int b = 0; b < batch_size; b++)
			{
				if (order_index >= sample_order.size())
				{
					std::random_shuffle(sample_order.begin(), sample_order.end());
					order_index = 0;
				}

				const MovesLeftDatasetEntry &entry = dataset[sample_order[order_index]];
				const int move_index = randInt(entry.length());
				const Sign sign_to_move = entry.fill_board(board, move_index);

				network.packInputData(b, board, sign_to_move, entry.rules());
				network.packTarget(b, entry.length() - move_index);

				order_index++;
			}

			network.train(batch_size);
			std::vector<float> acc = network.getAccuracy(batch_size, accuracy.size() - 1);
			addVectors(accuracy, acc);
		}

		std::cout << "accuracy =";
		for (size_t i = 1; i < accuracy.size(); i++)
			std::cout << ' ' << accuracy[i] / accuracy[0];
		std::cout << '\n';

		if (e % 25 == 0)
			network.saveToFile(path + "/network_" + std::to_string(e) + ".bin");

		if (e >= max_epochs - swa_num_epochs)
		{
			network.saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
			MovesLeftNetwork opt;
			opt.loadFromFile(path + "/network_" + std::to_string(e) + "_opt.bin");
			opt.moveTo(ml::Device::cuda());
			opt.optimize();
			opt.saveToFile(path + "/network_" + std::to_string(e) + "_opt.bin");
		}

		Json tmp;
		tmp["epoch"] = e + 1;
		tmp["max_epochs"] = max_epochs;
		tmp["swa_num_epochs"] = swa_num_epochs;
		tmp["steps_per_epoch"] = steps_per_epoch;
		tmp["learning_rate_schedule"] = learning_rate.toJson();
		FileSaver(path + "sl_progress.json").save(tmp, SerializedObject(), true);
		network.saveToFile(path + "/network_progress.bin");

		const double t1 = getTime();
		std::cout << "epoch time = " << (t1 - t0) << '\n';
	}

	MovesLeftNetwork swa_network;
	swa_network.loadFromFile(path + "/network_" + std::to_string(max_epochs - swa_num_epochs) + "_opt.bin");
	int count = 0;
	for (int i = max_epochs - swa_num_epochs; i <= max_epochs; i++)
	{
		MovesLeftNetwork network;
		network.loadFromFile(path + "/network_" + std::to_string(i) + "_opt.bin");
		network.setBatchSize(1);
		network.moveTo(ml::Device::cpu());
		count++;
		const float alpha = 1.0f / count;
		ml::averageModelWeights(alpha, network.get_graph(), 1.0f - alpha, swa_network.get_graph());
	}
	swa_network.saveToFile(path + "/network_swa_opt.bin");
}

std::string get_BOARD_command(const matrix<Sign> &board, Sign signToMove)
{
	std::vector<Move> moves_cross;
	std::vector<Move> moves_circle;
	for (int r = 0; r < board.rows(); r++)
		for (int c = 0; c < board.cols(); c++)
			switch (board.at(r, c))
			{
				default:
					break;
				case Sign::CROSS:
					moves_cross.push_back(Move(r, c, Sign::CROSS));
					break;
				case Sign::CIRCLE:
					moves_circle.push_back(Move(r, c, Sign::CIRCLE));
					break;
			}

	std::string result = "BOARD\n";
	if (moves_cross.size() == moves_circle.size()) // I started the game as first player (CROSS)
	{
		for (size_t i = 0; i < moves_cross.size(); i++)
		{
			result += std::to_string(moves_cross[i].col) + ',' + std::to_string(moves_cross[i].row) + ",1\n";
			result += std::to_string(moves_circle[i].col) + ',' + std::to_string(moves_circle[i].row) + ",2\n";
		}
	}
	else
	{
		if (moves_cross.size() == moves_circle.size() + 1) // opponent started the game as first player (CROSS)
		{
			for (size_t i = 0; i < moves_circle.size(); i++)
			{
				result += std::to_string(moves_cross[i].col) + ',' + std::to_string(moves_cross[i].row) + ",2\n";
				result += std::to_string(moves_circle[i].col) + ',' + std::to_string(moves_circle[i].row) + ",1\n";
			}
			result += std::to_string(moves_cross.back().col) + ',' + std::to_string(moves_cross.back().row) + ",2\n";
		}
		else
			throw ProtocolRuntimeException("Invalid position - too many stones either color");
	}

	return result + "DONE\n";
}

std::vector<Move> load_psq_file(const std::string &path)
{
	std::vector<Move> result;
	std::fstream file(path, std::fstream::in);
	if (file.good() == false)
		throw std::runtime_error("File '" + path + "' could not be opened");
	Sign s = Sign::CROSS;
	for (int i = 0;; i++)
	{
		std::string line;
		std::getline(file, line);
		if (line.empty())
			break;

		if (i > 0)
		{
			const auto tmp = split(line, ',');
			if (tmp.size() == 3)
			{
				const Move m(std::stoi(tmp.at(0)) - 1, std::stoi(tmp.at(1)) - 1, s);
				s = invertSign(s);
				result.push_back(m);
			}
		}
	}
	file.close();
	return result;
}

void train_simple_evaluation()
{
////	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 0; i < 17; i++)
//#else
//	for (int i = 0; i < 1; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator calculator(game_config);
//	nnue::TrainingNNUE evaluator(game_config);
//
//	float learning_rate = 0.001f;
//	const float weight_decay = 0.0e-5f;
//	const int batch_size = 32;
//	TimedStat forward("forward");
//
////	for (int i = 0; i < 10; i++)
////		std::cout << buffer.getFromBuffer(i).getNumberOfSamples() << '\n';
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	for (int epoch = 0; epoch < 100; epoch++)
//	{
//		std::vector<int> ordering = permutation(buffer.size());
//		float avg_loss = 0.0f;
//		for (int step = 0; step < buffer.size(); step += batch_size)
//		{
//			for (int batch = 0; batch < batch_size; batch++)
//			{
////				std::cout << epoch << ", " << step << ", " << batch << " : " << avg_loss << "\n";
//				const int idx = ordering[(step + batch) % ordering.size()];
//				const SearchData &sample = buffer.getFromBuffer(idx).getSample();
////				const SearchData &sample = buffer.getFromBuffer(step % 5).getSample();
//
//				sample.getBoard(board);
//				augment(board, randInt(8));
//				const Sign sign_to_move = sample.getMove().sign;
//
////				Value target = convertOutcome(sample.getOutcome(), sign_to_move);
//				Value target = sample.getMinimaxValue();
//
//				target.win_rate = std::max(0.0f, std::min(1.0f, target.win_rate));
//				target.draw_rate = std::max(0.0f, std::min(1.0f - target.win_rate, target.draw_rate));
//
//				calculator.setBoard(board, sign_to_move);
//				forward.startTimer();
//				const Value output = evaluator.forward(calculator);
//				forward.stopTimer();
////				std::cout << output.toString() << " vs " << target.toString() << '\n';
//				avg_loss += evaluator.backward(calculator, target);
//			}
//			evaluator.learn(learning_rate, weight_decay);
//			if (std::isnan(avg_loss))
//			{
//				std::cout << "NaN loss encountered, exiting\n";
//				return;
//			}
//			if (epoch == 30 or epoch == 60)
//				learning_rate *= 0.1f;
//		}
//		std::cout << "epoch " << epoch << ", avg loss = " << avg_loss / buffer.size() << '\n';
////		std::cout << "epoch " << epoch << ", avg loss = " << avg_loss << '\n';
//	}
//	nnue::NNUEWeights w = evaluator.dump();
//	SerializedObject so;
//	Json json = w.save(so);
//	FileSaver("/home/maciek/Desktop/standard_nnue_32x8x1.bin").save(json, so);
////	FileSaver("/home/maciek/Desktop/freestyle_nnue_32x8x1.bin").save(json, so);
////	nnue::InferenceNNUE nn(game_config, w);
////	nn.refresh(calculator);
////
////	calculator.addMove(Move("Oh5"));
////	nn.update(calculator);
////	calculator.print();
////	std::cout << "baseline  = " << evaluator.forward(calculator).win << '\n';
////	std::cout << "optimized = " << nn.forward() << '\n';
//
////	double t0, t1;
////	t0 = getTime();
////	for (int i = 0; i < 1000000; i++)
////		nn.refresh(calculator);
////	t1 = getTime();
////	std::cout << "nn.refresh() = " << (t1 - t0) << "us\n";
////
////	t0 = getTime();
////	for (int i = 0; i < 1000000; i++)
////		nn.update(calculator);
////	t1 = getTime();
////	std::cout << "nn.update() = " << (t1 - t0) << "us\n";
////
////	t0 = getTime();
////	float tmp = 0.0f;
////	for (int i = 0; i < 1000000; i++)
////		tmp += nn.forward();
////	t1 = getTime();
////	std::cout << "nn.forward() = " << (t1 - t0) << "us\n";
////
////	std::cout << nn.forward() << '\n';
////	std::cout << tmp << '\n';
//	std::cout << "END" << std::endl;
}

void test_pattern_calculator()
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::STANDARD, 15);

	PatternTable::get(game_config.rules);
	ThreatTable::get(game_config.rules);
	DefensiveMoveTable::get(game_config.rules);

	GameDataBuffer buffer;
#ifdef NDEBUG
	for (int i = 200; i <= 224; i++)
#else
	for (int i = 224; i <= 224; i++)
#endif
		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/valid_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	PatternCalculator extractor_new(game_config);
	PatternCalculator extractor_new2(game_config);

	SearchDataPack pack(game_config.rows, game_config.cols);

	int count = 0;
	for (int i = 0; i < buffer.numberOfGames(); i++)
	{
		if (i % (buffer.numberOfGames() / 10) == 0)
			std::cout << i << " / " << buffer.numberOfGames() << '\n';

		for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
		{
			buffer.getGameData(i).getSample(pack, j);
			extractor_new.setBoard(pack.board, pack.played_move.sign);

//			if (extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::FIVE).size() > 0
//					and (extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::OPEN_4).size() > 0
//							or extractor_new.getThreatHistogram(Sign::CROSS).get(ThreatType::FORK_4x4).size() > 0))
//				count++;
//			if (extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::FIVE).size() > 0
//					and (extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::OPEN_4).size() > 0
//							or extractor_new.getThreatHistogram(Sign::CIRCLE).get(ThreatType::FORK_4x4).size() > 0))
//				count++;

			for (int k = 0; k < 100; k++)
			{
				const int x = randInt(game_config.rows);
				const int y = randInt(game_config.cols);
				if (pack.board.at(x, y) == Sign::NONE)
				{
					Sign sign = static_cast<Sign>(randInt(1, 3));
					extractor_new.addMove(Move(x, y, sign));
					pack.board.at(x, y) = sign;
				}
				else
				{
					extractor_new.undoMove(Move(x, y, pack.board.at(x, y)));
					pack.board.at(x, y) = Sign::NONE;
				}
			}

//			extractor_new2.setBoard(board, sample.getMove().sign);
//			for (int x = 0; x < game_config.rows; x++)
//				for (int y = 0; y < game_config.cols; y++)
//				{
//					for (Direction dir = 0; dir < 4; dir++)
//					{
//						if (extractor_new2.getNormalPatternAt(x, y, dir) != extractor_new.getNormalPatternAt(x, y, dir))
//						{
//							std::cout << "Raw pattern mismatch\n";
//							std::cout << "Single step\n";
//							extractor_new2.printRawFeature(x, y);
//							std::cout << "incremental\n";
//							extractor_new.printRawFeature(x, y);
//							exit(-1);
//						}
//						if (extractor_new2.getPatternTypeAt(Sign::CROSS, x, y, dir) != extractor_new.getPatternTypeAt(Sign::CROSS, x, y, dir)
//								or extractor_new2.getPatternTypeAt(Sign::CIRCLE, x, y, dir)
//										!= extractor_new.getPatternTypeAt(Sign::CIRCLE, x, y, dir))
//						{
//							std::cout << "Pattern type mismatch\n";
//							std::cout << "Single step\n";
//							extractor_new2.printRawFeature(x, y);
//							std::cout << "incremental\n";
//							extractor_new.printRawFeature(x, y);
//							exit(-1);
//						}
//					}
//					if (extractor_new2.getThreatAt(Sign::CROSS, x, y) != extractor_new.getThreatAt(Sign::CROSS, x, y)
//							or extractor_new2.getThreatAt(Sign::CIRCLE, x, y) != extractor_new.getThreatAt(Sign::CIRCLE, x, y))
//					{
//						std::cout << "Threat type mismatch\n";
//						std::cout << "Single step\n";
//						extractor_new2.printThreat(x, y);
//						std::cout << "incremental\n";
//						extractor_new.printThreat(x, y);
//						exit(-1);
//					}
//
//					for (int i = 0; i < 10; i++)
//					{
//						if (extractor_new2.getThreatHistogram(Sign::CROSS).get((ThreatType) i).size()
//								!= extractor_new.getThreatHistogram(Sign::CROSS).get((ThreatType) i).size())
//						{
//							std::cout << "Threat histogram mismatch for cross\n";
//							std::cout << "Single step\n";
//							extractor_new2.getThreatHistogram(Sign::CROSS).print();
//							std::cout << "incremental\n";
//							extractor_new.getThreatHistogram(Sign::CROSS).print();
//							exit(-1);
//						}
//						if (extractor_new2.getThreatHistogram(Sign::CIRCLE).get((ThreatType) i).size()
//								!= extractor_new.getThreatHistogram(Sign::CIRCLE).get((ThreatType) i).size())
//						{
//							std::cout << "Threat histogram mismatch for circle\n";
//							std::cout << "Single step\n";
//							extractor_new2.print();
//							extractor_new2.getThreatHistogram(Sign::CIRCLE).print();
//							std::cout << "incremental\n";
//							extractor_new.print();
//							extractor_new.getThreatHistogram(Sign::CIRCLE).print();
//							exit(-1);
//						}
//					}
//				}
		}
	}

	std::cout << "count  = " << count << '\n';
	std::cout << "New extractor\n";
	extractor_new.print_stats();
}

void test_move_generator()
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::STANDARD, 15);
	game_config.draw_after = 200;

	PatternTable::get(game_config.rules);
	ThreatTable::get(game_config.rules);
	DefensiveMoveTable::get(game_config.rules);

//	GameDataBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 200; i <= 200; i++)
//#else
//	for (int i = 200; i <= 200; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';

	PatternCalculator calculator(game_config);
	MoveGenerator generator(game_config, calculator);

	SearchDataPack pack(game_config.rows, game_config.cols);

	ActionStack action_stack(1024);

//	AlphaBetaSearch search(game_config, MoveGeneratorMode::REDUCED);
	TSSConfig tss_cfg;
	tss_cfg.mode = 2;
	tss_cfg.max_positions = 1000000;
	ThreatSpaceSearch search(game_config, tss_cfg);

	SearchTask task(game_config);

	matrix<Sign> board(game_config.rows, game_config.cols);
	Sign sign_to_move;

	double total_nodes = 0;
	double total_samples = 0;
	for (int b = 200; b <= 210; b++)
//	int b = 200;
	{
		GameDataBuffer buffer;
		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(b) + ".bin");
		std::cout << buffer.getStats().toString() << '\n';
		total_samples += buffer.numberOfSamples();

		for (int i = 0; i < buffer.numberOfGames(); i++)
//		int i = 9;
		{
			if (i % (buffer.numberOfGames() / 10) == 0)
				std::cout << i << " / " << buffer.numberOfGames() << '\n';

			for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
//			int j = 30;
			{
				buffer.getGameData(i).getSample(pack, j);
				calculator.setBoard(pack.board, pack.played_move.sign);

				ActionList actions(action_stack);
				const Score static_score = generator.generate(actions, MoveGeneratorMode::THREATS);
				total_nodes += actions.size();

//				if (actions.must_defend)
//				{
//					const Score base_score = actions.baseline_score;
//					const int depth = base_score.getDistance();
//					std::cout << "checking " << base_score.toString() << '\n';
//
//					task.set(pack.board, pack.played_move.sign);
//					search.setDepthLimit(depth);
//					search.setNodeLimit(1000000);
//					search.solve(task);
//
//					for (int row = 0; row < task.getBoard().rows(); row++)
//						for (int col = 0; col < task.getBoard().cols(); col++)
//							if (task.getBoard().at(row, col) == Sign::NONE)
//							{
//								const Move m(row, col, task.getSignToMove());
//								if (not task.getActionScores().at(row, col).isLoss())
//								{
//									const bool is_found = actions.contains(m);
//									const Score sc = actions.getScoreOf(m);
//									if (sc.isLoss() or not is_found)
//									{
//										std::cout << "b = " << 200 << ", i=" << i << ", j=" << j << '\n';
//										std::cout << "MISSING MOVE " << m.toString() << " " << m.text() << '\n';
//										std::cout << m.text() << " " << task.getActionScores().at(row, col) << " "
//												<< (actions.contains(m) ? actions.getScoreOf(m).toString() : "N/A") << '\n';
//										std::cout << task.toString() << '\n';
//										calculator.print();
//										calculator.printAllThreats();
//										std::cout << "\nGenerated actions\n";
//										actions.print();
//										std::cout << "True non-losing moves\n";
//										for (int r = 0; r < task.getBoard().rows(); r++)
//											for (int c = 0; c < task.getBoard().cols(); c++)
//												if (task.getBoard().at(r, c) == Sign::NONE and not task.getActionScores().at(r, c).isLoss())
//													std::cout << Move(r, c, task.getSignToMove()).text() << '\n';
//										return;
//									}
//								}
//							}
//				}
//				if (static_score.isWin())
//				if (static_score == Score::win_in(7))
//				{
//					const int depth = static_score.getDistance();
//					std::cout << "checking " << static_score.toString() << '\n';
//
//					task.set(pack.board, pack.played_move.sign);
//					search.solve(task, TssMode::RECURSIVE, 1000000);
//					std::cout << task.toString() << '\n';
//					if(task.getScore() != static_score)
////						exit(-1);
////					for (auto iter = actions.begin(); iter < actions.end(); iter++)
////						if (task.getActionScores().at(iter->move.row, iter->move.col) < iter->score)
//						{
//							std::cout << "b = " << 200 << ", i=" << i << ", j=" << j << '\n';
////							std::cout << "FALSE WINNING MOVE " << iter->move.toString() << " " << iter->move.text() << '\n';
//							std::cout << task.toString() << '\n';
//							calculator.print();
//							calculator.printAllThreats();
//							std::cout << "\nGenerated actions\n";
//							actions.print();
//							return;
//						}
//				}

//			if (static_score == Score::win_in(5))
//			{
//				const int depth = static_score.getDistance();
//				task.set(pack.board, pack.played_move.sign);
//				search.solve(task, depth, 100000000);
//				for (auto iter = actions.begin(); iter < actions.end(); iter++)
//				{
//					const Score exact_score = task.getActionScores().at(iter->move.row, iter->move.col);
//					if (iter->score.isProven() and iter->score != exact_score)
//					{
//						std::cout << task.toString() << '\n';
//						actions.print();
//						return;
//					}
//				}
//
//			}
			}
		}
	}
	std::cout << "avg nodes  = " << total_nodes / total_samples << '\n';
//	calculator.print_stats();
}

void test_proven_positions(int pos)
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::RENJU, 15);
//	GameConfig game_config(GameRules::CARO5, 15);
//	GameConfig game_config(GameRules::CARO6, 20);

	PatternTable::get(game_config.rules);
	ThreatTable::get(game_config.rules);
	DefensiveMoveTable::get(game_config.rules);

	GameDataBuffer buffer("/home/maciek/alphagomoku/proven_positions_big.bin");
//	GameDataBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 200; i <= 200; i++)
//#else
//	for (int i = 200; i <= 200; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

//	AlphaBetaSearch ab_search(game_config);
	VCFSolver ab_search(game_config);
	ab_search.loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
	ab_search.setNodeLimit(pos);

	TSSConfig tss_config;
	ThreatSpaceSearch ts_search(game_config, tss_config);
	ts_search.loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));

	SearchDataPack pack(game_config.rows, game_config.cols);
	SearchTask task(game_config);
	matrix<Sign> board(game_config.rows, game_config.cols);
	int total_samples = 0;
	int tss_solved_count = 0, ab_solved_count = 0;
	int total_positions = 0;

	std::map<Score, std::pair<int, int>> score_stats;

#ifndef NDEBUG
	for (int i = 0; i < buffer.numberOfGames(); i += 100)
#else
	for (int i = 0; i < buffer.numberOfGames(); i += 100)
#endif
//	int i = 102600;
	{
		if (i % (buffer.numberOfGames() / 10) == 0)
			std::cout << i << " / " << buffer.numberOfGames() << " " << ab_solved_count << "/" << total_samples << '\n';

		total_samples += buffer.getGameData(i).numberOfSamples();
		for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
//		int j = 0;
		{
//			std::cout << i << " " << j << '\n';
			buffer.getGameData(i).getSample(pack, j);

//			task.set(pack.board, pack.played_move.sign);
//			ts_search.solve(task, TssMode::RECURSIVE, pos);
//			tss_solved_count += task.getScore().isProven();

			task.set(pack.board, pack.played_move.sign);

			const int positions = ab_search.solve(task);
//			vcf_search.setDepthLimit(4);
//			vcf_search.setNodeLimit(pos);
//			const int positions = vcf_search.solve(task);
			total_positions += positions;
			ab_solved_count += task.getScore().isProven();
//			if (task.getScore().isUnproven())
//			{
//				vcf_search.setDepthLimit(100);
//				vcf_search.setNodeLimit(pos - positions);
//				total_positions += vcf_search.solve(task);
//				ab_solved_count += task.getScore().isProven();
//			}
			if (task.getScore().isProven())
			{
				auto iter = score_stats.find(task.getScore());
				if (iter == score_stats.end())
					score_stats.insert( { task.getScore(), { 1, positions } });
				else
				{
					iter->second.first++;
					iter->second.second += positions;
				}
			}
//			return;
		}
	}

	for (auto iter = score_stats.begin(); iter != score_stats.end(); iter++)
		std::cout << iter->first.toString() << " : " << iter->second.first << " (" << (100.0 * iter->second.first / total_samples)
				<< "%), avg positions = " << (double) iter->second.second / iter->second.first << '\n';

//	std::cout << "TSS\n";
	ab_search.print_stats();
//	std::cout << "tss solved " << tss_solved_count << " samples (" << 100.0f * tss_solved_count / total_samples << "%)\n";
	std::cout << "a-b solved " << ab_solved_count << " samples (" << 100.0f * ab_solved_count / total_samples << "%)\n";
//	ts_search.print_stats();
	std::cout << "total_positions = " << total_positions << ", avg = " << total_positions / total_samples << '\n';
}

void test_solver(int pos)
{
////	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 0; i < 17; i++)
//#else
//	for (int i = 0; i < 1; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	int total_samples = 0;
//
//	std::fstream output("/home/maciek/Desktop/win_all.txt", std::fstream::out);
//
//	for (int i = 0; i < buffer.size(); i++)
//	{
//		if (i % (buffer.size() / 10) == 0)
//			std::cout << i << " / " << buffer.size() << '\n';
//
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
//			sample.getBoard(board);
//			const Sign sign_to_move = sample.getMove().sign;
//			output << get_BOARD_command(board, sign_to_move) << "nodes 0\n";
//		}
//	}
}

void test_forbidden_moves()
{
//	GameConfig game_config(GameRules::RENJU, 15);
//
//	GameBuffer buffer;
////#ifdef NDEBUG
//	for (int i = 0; i <= 21; i++)
////#else
////		for (int i = 0; i < 1; i++)
////#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator calc(game_config);
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	int total_samples = 0;
//
//	const double start = getTime();
//	for (int i = 0; i < buffer.size(); i++)
////	int i = 143634;
//	{
//		if (i % (buffer.size() / 100) == 0)
//			std::cout << i << " / " << buffer.size() << '\n';
//
//		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		total_samples += buffer.getFromBuffer(i).getNumberOfSamples();
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
////		int j = 0;
//		{
//			const SearchData &sample = buffer.getFromBuffer(i).getSample(j);
//			sample.getBoard(board);
//			const Sign sign_to_move = sample.getMove().sign;
//
//			calc.setBoard(board, sign_to_move);
//			for (int r = 0; r < game_config.rows; r++)
//				for (int c = 0; c < game_config.cols; c++)
//				{
//					const bool b0 = isForbidden(board, Move(r, c, Sign::CROSS));
//					const bool b1 = calc.isForbidden(Sign::CROSS, r, c);
//					if (b0 != b1)
//					{
//						std::cout << "mismatch\n";
//						return;
//					}
//				}
//
////			bool flag = false;
////			IsOverline is_overline(game_config.rules, Sign::CIRCLE);
////			for (int r = 0; r < game_config.rows; r++)
////			{
////				if (flag == false)
////					for (int c = 0; c < game_config.cols; c++)
////						for (int d = 0; d < 4; d++)
////							if (is_overline(Pattern(11, calc.getNormalPatternAt(r, c, d))))
////							{
////								flag = true;
////								break;
////							}
////			}
//		}
//	}
//
//	std::cout << "time " << (getTime() - start) << ", samples = " << total_samples << '\n';
}

void test_search()
{
//	GameConfig game_config(GameRules::CARO5, 15);
//	game_config.draw_after = 200;
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::FREESTYLE, 20);

	SearchConfig search_config;
	search_config.max_batch_size = 1;
	search_config.mcts_config.max_children = 225;
	search_config.mcts_config.policy_expansion_threshold = 0.0f;
	search_config.tss_config.mode = 0;
	search_config.tss_config.max_positions = 100;
	search_config.tss_config.hash_table_size = 1048576;

	Tree tree(search_config.tree_config);

	DeviceConfig device_config;
	device_config.batch_size = 12;
//#ifdef NDEBUG
	device_config.device = ml::Device::cuda();
//#else
//	device_config.device = ml::Device::cpu();
//#endif
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
//	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku582/networks/standard_conv_8x128.bin");
	const std::string path = "/home/maciek/alphagomoku/runs_2026/standard_20x256/";
	std::vector<std::string> paths;
	for (int i = 24; i <= 28; i++)
		paths.push_back(path + "checkpoint/network_" + std::to_string(i) + ".bin");
	nn_evaluator.loadGraph(NetworkLoader(paths));
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin");
//	nn_evaluator.loadGraph("./old_6x64s.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_255_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs/btl_pv_8x128f/checkpoint/network_242_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs/standard_15x15/checkpoint/network_0_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/tl/checkpoint/network_99_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/new_runs_2023/test_6x64s/checkpoint/network_70_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/alphagomoku/tests_2023/base_q_head/checkpoint/network_15_opt.bin");
//	nn_evaluator.loadGraph("/home/maciek/cpp_workspace/AlphaGomoku/Release/networks/network_44_opt.bin");

	Sign sign_to_move;
	matrix<Sign> board(15, 15);

//	board = Board::fromString(" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ X _ O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ O X _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ O O X _ _ _ _ _\n"
//			" _ _ _ _ _ X _ X O _ _ _ _ _ _\n"
//			" _ _ _ _ _ X X _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ X _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//	board = Board::fromString(" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ O X _ _ _ _ _ _ _\n"
//			" _ _ _ _ X _ O O O X _ _ _ _ _\n"
//			" _ _ _ _ _ O _ X O O O _ _ _ _\n"
//			" _ _ _ _ _ O O X _ X X _ _ _ _\n"
//			" _ _ _ _ _ _ X O X X _ _ _ _ _\n"
//			" _ _ _ _ _ _ X X O O O _ _ _ _\n"
//			" _ _ _ _ _ _ _ O X X X X O X _\n"
//			" _ _ _ _ _ X _ _ O O X O O _ _\n"
//			" _ _ _ _ _ _ _ _ _ O X X X _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ X _ _ X _\n"
//			" _ _ _ _ _ _ _ _ _ _ O O _ O X\n");
//
//	sign_to_move = Sign::CIRCLE;

	board = Board::fromString(" X O X X O X O O O X O X O X X\n"
			" O O X O O O X X O X O X X O O\n"
			" O X O X X X X O X O O X O X X\n"
			" O X X X O X O O X X X O O O X\n"
			" X X O X O X O O X O X X O O X\n"
			" O X O O O X X O O X X O X X O\n"
			" X O X O X O X X O X O O X O O\n"
			" X O X O O X O _ O X X X O X O\n"
			" O X X X _ O X X X O O O X X X\n"
			" O O O O X X X O O X X X O X O\n"
			" X O X X O O X O O X O X O O O\n"
			" X X X O O O X X O O O O X O X\n"
			" O X O X O X O O O X X O O O X\n"
			" O X X X O O O X X O X O X X X\n"
			" X O O O X X O X O X X X O X O\n");
	// @formatter:on
	sign_to_move = Sign::CIRCLE;

//	ag::pab::Search pab_search(game_config);
//	pab_search.loadNetwork("/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin");
//
//	pab_search.setBoard(board, sign_to_move);
//	pab_search.iterative_deepening();

//	return;

//	std::cout << (int) getOutcome(game_config.rules, board, Move(Sign::CROSS, 0, 0), game_config.draw_after) << '\n';

//	MovesLeftNetwork network;
//	network.loadFromFile("/home/maciek/alphagomoku/runs_2026/moves_left_network_15x15_test/network_swa_opt.bin");
//	network.setBatchSize(1);
//	network.moveTo(ml::Device::cpu());
//	network.packInputData(0, board, sign_to_move, game_config.rules);
//	network.forward(1);
//	float moves_left = 0.0f;
//	network.unpackOutput(0, moves_left);
//	std::cout << "moves left = " << moves_left << '\n';

//	ThreatSpaceSearch ts_search(game_config, search_config.tss_config);
	AlphaBetaSearch ts_search(game_config);
//	ts_search.loadWeights(nnue::NNUEWeights("networks/freestyle_nnue_64x16x16x1.bin"));
	ts_search.setNodeLimit(100);

//	std::cout << "Forbidden moves : \n";
//	for (int i = 0; i < 15; i++)
//		for (int j = 0; j < 15; j++)
//			if (board.at(i, j) == Sign::NONE and isForbidden(board, Move(i, j, Sign::CROSS)))
//				std::cout << Move(i, j, Sign::CROSS).text() << '\n';

	SearchTask task(game_config);

	SearchTask task2(game_config);
	task2.set(board, sign_to_move);
	ts_search.solve(task2);
	std::cout << "\n\n\n";
	ts_search.print_stats();
	std::cout << "\n\n\n" << task2.toString() << '\n';

//	VCFSolver vcf_solver(game_config);
//	task.set(board, sign_to_move);
//	vcf_solver.solve(task);
//	std::cout << task.toString() << '\n';
//	return;
//	std::cout << get_BOARD_command(board, sign_to_move);
//	return;

	Search search(game_config, search_config);
//	search.getSolver().loadWeights(nnue::NNUEWeights("networks/freestyle_nnue_64x16x16x1.bin"));
	tree.setBoard(board, sign_to_move);
//	tree.setEdgeSelector(NoisyPUCTSelector(Board::numberOfMoves(board), 1.25f));

//	search_config.mcts_config.edge_selector_config.init_to = "q_head";
//	search_config.mcts_config.edge_selector_config.noise_type = "none";
//	search_config.mcts_config.edge_selector_config.noise_weight = 1.0f;

//	search_config.mcts_config.edge_selector_config.init_to = "loss";
	tree.setEdgeSelector(PUCTSelector(search_config.mcts_config.edge_selector_config));
//	tree.setEdgeSelector(ThompsonSelector(search_config.mcts_config.edge_selector_config));
//	tree.setEdgeSelector(KLUCBSelector(search_config.mcts_config.edge_selector_config));
	tree.setEdgeGenerator(UnifiedGenerator(search_config.mcts_config.max_children, search_config.mcts_config.policy_expansion_threshold, false));
//	tree.setEdgeGenerator(UnifiedGenerator(true));

	int next_step = 1;
	double time_per_sample = 0.1;
	for (int j = 0; j <= 1; j++)
	{
		if (tree.getSimulationCount() >= next_step)
		{
//			std::cout << tree.getSimulationCount() << " ..." << std::endl;
//			Node info = tree.getInfo( { });
//			info.sortEdges();
//			std::cout << info.toString() << '\n';
//			for (int i = 0; i < info.numberOfEdges(); i++)
//				std::cout << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
//			std::cout << '\n';
			next_step++;
		}
		search.select(tree, 10000);
		search.solve(-1); //getTime() + 0.5 * search.getBatchSize() * time_per_sample);
		search.scheduleToNN(nn_evaluator);
		time_per_sample = nn_evaluator.evaluateGraph();

		search.generateEdges(tree);
		search.expand(tree);
		search.backup(tree);

//		std::cout << "\n\n\n";
//		tree.printSubtree(10, true, 5);
		if (tree.isRootProven())
			break;
	}
	search.cleanup(tree);

//	tree.printSubtree(100);

	std::cout << search.getStats().toString() << '\n';
	std::cout << "memory = " << ((tree.getMemory() + search.getMemory()) / 1048576.0) << "MB\n\n";
	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
	std::cout << tree.getNodeCacheStats().toString() << '\n';
	std::cout << nn_evaluator.getStats().toString() << '\n';
//	search.getSolver().print_stats();

	Node info = tree.getInfo( { });
	info.sortEdges();
	std::cout << info.toString() << '\n';
	for (int i = 0; i < info.numberOfEdges(); i++)
		std::cout << info.getEdge(i).getMove().toString() << " : " << info.getEdge(i).toString() << '\n';
	std::cout << '\n';

	LCBSelector lcb(search_config.mcts_config.edge_selector_config);
	lcb.select(&info);

	matrix<float> policy(game_config.rows, game_config.cols);
	matrix<ProvenValue> proven_values(game_config.rows, game_config.cols);
	proven_values.fill(ProvenValue::UNKNOWN);
	for (int i = 0; i < info.numberOfEdges(); i++)
	{
		const Edge &edge = info.getEdge(i);
		Move move = edge.getMove();
		policy.at(move.row, move.col) = edge.getVisits();
		proven_values.at(move.row, move.col) = edge.getProvenValue();
	}
	float r = std::accumulate(policy.begin(), policy.end(), 0.0f);
	if (r > 0.0f)
	{
		r = 1.0f / r;
		for (int i = 0; i < policy.size(); i++)
			policy.data()[i] *= r;
	}

	std::cout << Board::toString(board, true) << '\n';

	std::string result;
	for (int i = 0; i < board.rows(); i++)
	{
		for (int j = 0; j < board.cols(); j++)
		{
			if (board.at(i, j) == Sign::NONE)
			{
				switch (proven_values.at(i, j))
				{
					case ProvenValue::UNKNOWN:
					{
						int t = (int) (1000 * policy.at(i, j));
						if (t == 0)
							result += "  _ ";
						else
						{
							if (t < 1000)
								result += ' ';
							if (t < 100)
								result += ' ';
							if (t < 10)
								result += ' ';
							result += std::to_string(t);
						}
						break;
					}
					case ProvenValue::LOSS:
						result += " >L<";
						break;
					case ProvenValue::DRAW:
						result += " >D<";
						break;
					case ProvenValue::WIN:
						result += " >W<";
						break;
				}
			}
			else
				result += (board.at(i, j) == Sign::CROSS) ? "  X " : "  O ";
		}
		result += '\n';
	}
	std::cout << result;

	tree.setEdgeSelector(BestEdgeSelector());
	tree.select(task);
	tree.cancelVirtualLoss(task);

	std::cout << task.toString() << '\n';

	for (int i = 0; i < task.visitedPathLength(); i++)
	{
		if (i < 10 and task.visitedPathLength() >= 10)
			std::cout << " ";
		if (i < 100 and task.visitedPathLength() >= 100)
			std::cout << " ";
		std::cout << i << " : " << task.getPair(i).edge->getMove().toString() << " : " << task.getPair(i).edge->toString() << '\n';
	}

//	task.getLastPair().node->sortEdges();
//	std::cout << task.getLastPair().node->toString() << '\n';
//	for (int i = 0; i < task.getLastPair().node->numberOfEdges(); i++)
//		std::cout << task.getLastPair().node->getEdge(i).getMove().toString() << " : " << task.getLastPair().node->getEdge(i).toString() << '\n';
//	std::cout << '\n';

	std::cout << "END" << std::endl;

//	Board::putMove(board, Move(Sign::CIRCLE, 0, 0));
//	Board::putMove(board, Move(Sign::CROSS, 0, 1));
//
//	std::cout << Board::toString(board) << '\n';
//
//	tree.setBoard(board, Sign::CIRCLE);
//	tree.setEdgeSelector(PuctSelector(1.25f));
//	std::cout << tree.getNodeCacheStats().toString() << '\n';
//
//	search = Search(game_config, search_config);
//	for (int i = 0; i <= 1000; i++)
//	{
//		search.select(tree, 1000);
//		search.tryToSolve();
//
//		search.scheduleToNN(nn_evaluator);
//		nn_evaluator.evaluateGraph();
//
//		search.generateEdges(tree);
//		search.expand(tree);
//		search.backup(tree);
//
//		if (tree.isProven())
//			break;
//	}
//	tree.printSubtree(0, true);
//	std::cout << "max depth = " << tree.getMaximumDepth() << '\n';
//	std::cout << tree.getNodeCacheStats().toString() << '\n';
}

void time_manager()
{
//	std::string path = "/home/maciek/Desktop/test_tm/";
//	MasterLearningConfig config(FileLoader(path + "config.json").getJson());
//
//	GeneratorManager manager(config.game_config, config.generation_config);
////	manager.generate(path + "freestyle_10x128.bin", 10000, 0);
//
//	const GameBuffer &buffer = manager.getGameBuffer();
//	buffer.save(path + "buffer_1000f.bin");
//
////	GameBuffer buffer(path + "buffer.bin");
//
//	Json stats(JsonType::Array);
//	matrix<Sign> board(config.game_config.rows, config.game_config.cols);
//
//	for (int i = 0; i < buffer.size(); i++)
//	{
////		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		int nb_moves = Board::numberOfMoves(board);
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//		{
//			Value value = buffer.getFromBuffer(i).getSample(j).getMinimaxValue();
////			ProvenValue pv = buffer.getFromBuffer(i).getSample(j).getProvenValue();
//
//			Json entry;
//			entry["move"] = nb_moves + j;
//			entry["winrate"] = value.win;
//			entry["drawrate"] = value.draw;
////			entry["proven value"] = toString(pv);
//			stats[i][j] = entry;
//		}
//	}
//
//	FileSaver fs(path + "stats_1000f.json");
//	fs.save(stats, SerializedObject());
//	std::cout << "END" << std::endl;
}

void test_nnue()
{
//	GameConfig game_config(GameRules::FREESTYLE, 20);
//	GameConfig game_config(GameRules::STANDARD, 15);
//
//	GameBuffer buffer;
//#ifdef NDEBUG
//	for (int i = 0; i < 17; i++)
//#else
//	for (int i = 0; i < 1; i++)
//#endif
//		buffer.load("/home/maciek/alphagomoku/run2022_15x15s2/train_buffer/buffer_" + std::to_string(i) + ".bin");
////		buffer.load("/home/maciek/alphagomoku/run2022_20x20f/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	PatternCalculator calculator(game_config);
//	nnue::InferenceNNUE incremental(game_config);
//	nnue::InferenceNNUE one_step(game_config);
//
//	matrix<Sign> board(game_config.rows, game_config.cols);
//	const SearchData &sample = buffer.getFromBuffer(0).getSample(0);
//
////	sample.getBoard(board);
//	const Sign sign_to_move = sample.getMove().sign;
//	calculator.setBoard(board, sign_to_move);
//
//	std::vector<Move> moves;
//	while (moves.size() < 10u)
//	{
//		int x = randInt(game_config.rows);
//		int y = randInt(game_config.cols);
//		if (board.at(x, y) == Sign::NONE)
//		{
//			const Sign sign = moves.empty() ? sign_to_move : invertSign(moves.back().sign);
//			moves.push_back(Move(x, y, sign));
//			board.at(x, y) = sign;
//		}
//	}
////	sample.getBoard(board);
//	calculator.setBoard(board, sign_to_move);
//	incremental.refresh(calculator);
//	one_step.refresh(calculator);
//
//	for (auto iter = moves.begin(); iter < moves.end(); iter++)
//	{
//		std::cout << '\n';
//		calculator.addMove(*iter);
//		incremental.update(calculator);
//
//		one_step.refresh(calculator);
//	}
//	std::cout << '\n';
//	for (auto iter = moves.end() - 1; iter >= moves.begin(); iter--)
//	{
//		std::cout << '\n';
//		calculator.undoMove(*iter);
//		incremental.update(calculator);
//
//		one_step.refresh(calculator);
//	}
//	calculator.addMove(Move(0, 0, sign_to_move));
//	incremental.update(calculator);
//
//	one_step.refresh(calculator);
}

void test_evaluate(int idx = 0)
{
	const std::string path = "/home/maciek/alphagomoku/runs_2026/testy_bn/";
	MasterLearningConfig config(FileLoader(path + "config.json").getJson());
	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

	SelfplayConfig cfg(config.evaluation_config.selfplay_options);

//	const std::string p1 = "standard_15x15";
//	const std::string p2 = "standard_test_100po";
//
//	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/final_runs_2025/" + p1 + "/checkpoint/network_" + std::to_string(idx) + ".bin",
//			p1 + "_" + zfill(idx, 3));
//	manager.setSecondPlayer(cfg, path + p2 + "/checkpoint/network_" + std::to_string(idx) + ".bin", p2 + "_" + zfill(idx, 3));

//	cfg.simulations = 400;
//	cfg.search_config.mcts_config.edge_selector_config.init_to = "parent";
//	cfg.search_config.mcts_config.edge_selector_config.policy = "puct_fpu";
//	manager.setFirstPlayer(cfg, "/home/maciek/Desktop/AlphaGomoku583/networks/standard_conv_8x128.bin", "old");
//	manager.setFirstPlayer(cfg, path + "size_tests/baseline_8x128/network_swa_opt.bin", "baseline");

//	cfg.search_config.mcts_config.edge_selector_config.policy = "puct";
//	cfg.search_config.mcts_config.edge_selector_config.init_to = "q_head";
//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = 0.5f;
//	cfg.search_config.tss_config.max_positions = 100;

//	cfg.search_config.tss_config.mode = 2;
	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin", "AG_2025");

//	cfg.search_config.tss_config.mode = 1;
//	manager.setSecondPlayer(cfg, path + "standard_15x15/network_swa.bin", "no_solver");

//	cfg.search_config.mcts_config.edge_selector_config.policy = "puct_variance";
//	cfg.search_config.mcts_config.edge_selector_config.exploration_constant = idx / 100.0f;
//	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs_2025/test_pooling/pvq_8x128/network_swa_opt.bin",
//			"variance_" + std::to_string(idx));

	manager.setSecondPlayer(cfg, path + "ffn_x4/network_swa_opt.bin", "ffn_x4");
//	manager.setFirstPlayer(cfg, path + "final_tests/convnext_8x128_adamw/network_swa_opt.bin", "convnext_8x128_adamw");
//	manager.setSecondPlayer(cfg, path + "size_tests/convnext_8x128/network_swa_opt.bin", "convnext_8x128");

//	manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs_2025/size_tests/convnext_1x128/network_150_opt_int8.bin", "int8");

	const double start = getTime();
	manager.generate(4000);
	const double stop = getTime();
	std::cout << "generated in " << (stop - start) << '\n';

	const std::string to_save = manager.getPGN();
	std::ofstream file(path + "rating.pgn", std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();
}

void evaluate_vs_old(const std::string &path, int idx)
{
	MasterLearningConfig config(FileLoader(path + "/config.json").getJson());
	EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

	config.evaluation_config.selfplay_options.simulations = 400;
	SelfplayConfig cfg(config.evaluation_config.selfplay_options);

	std::vector<std::string> list;
	for (int i = std::max(0, idx - 4); i <= idx; i++)
		list.push_back(path + "checkpoint/network_" + std::to_string(idx) + ".bin");
	manager.setSecondPlayer(cfg, list, "AG_" + zfill(idx, 3));

	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin", "old");
//	manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/runs_2026/standard_test/network_swa.bin", "old");

	const double start = getTime();
	manager.generate(1000);
	const double stop = getTime();
	std::cout << "generated in " << (stop - start) << '\n';

	const std::string to_save = manager.getPGN();
	std::ofstream file(path + "compare.pgn", std::ios::out | std::ios::app);
	file.write(to_save.data(), to_save.size());
	file.close();
}

void parameter_tuning()
{
//	std::vector<double> exploration_c = { 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5 };
//	std::vector<double> exploration_exp = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 };

//	std::vector<double> exploration_tree = { 0.25, 0.5, 0.75, 1.0, 1.25 };
//	std::vector<double> exploration_final = { 0.25, 0.5, 0.75, 1.0, 1.25 };

	std::vector<double> exploration_tree = { 1.25 };
	std::vector<double> exploration_final = { 1.25 };

	for (size_t i1 = 0; i1 < exploration_tree.size(); i1++)
		for (size_t i2 = 0; i2 < exploration_final.size(); i2++)
		{
			const std::string path = "/home/maciek/alphagomoku/new_runs/";
			MasterLearningConfig config(FileLoader(path + "tuning_config.json").getJson());
			EvaluationManager manager(config.game_config, config.evaluation_config.selfplay_options);

			SelfplayConfig cfg(config.evaluation_config.selfplay_options);
			cfg.simulations = 1000;
			cfg.search_config.tss_config.max_positions = 200;

			manager.setFirstPlayer(cfg, "/home/maciek/alphagomoku/new_runs/test8_test/checkpoint/network_144_opt.bin", "base");

			cfg.search_config.mcts_config.edge_selector_config.exploration_constant = exploration_tree[i1];
//			cfg.final_selector.policy = "lcb";
//			cfg.final_selector.init_to = "loss";
//			cfg.search_config.mcts_config.edge_selector_config.init_to = "parent";
			cfg.search_config.tree_config.information_leak_threshold = -0.01;
			cfg.final_selector.exploration_constant = exploration_final[i2];
			manager.setSecondPlayer(cfg, "/home/maciek/alphagomoku/new_runs/test8_test/checkpoint/network_144_opt.bin",
					"lcb_" + std::to_string(exploration_tree[i1]) + "_" + std::to_string(exploration_final[i2]));

			std::cout << "testing " << exploration_tree[i1] << " " << exploration_final[i2] << '\n';
			const double start = getTime();
			manager.generate(400);
			const double stop = getTime();
			std::cout << "generated in " << (stop - start) << '\n';

			const std::string to_save = manager.getPGN();
			std::ofstream file("/home/maciek/alphagomoku/new_runs/testy_minimax2.pgn", std::ios::out | std::ios::app);
			file.write(to_save.data(), to_save.size());
			file.close();
		}
}

class Embedding
{
		/*
		 * row 0 - is cross (black) to move. If set to 0 it means that circle (white) is moving.
		 * Input encoding
		 *   0:6 - cross (black) threats (OPEN_3 to FIVE)
		 *   7:13 - circle (white) threats (OPEN_3 to FIVE)
		 *     14 - is cross (black) stone
		 *     15 - is circle (white) stone
		 */
	public:
		void set(const PatternCalculator &calc, float *tensor)
		{
			tensor[0] = static_cast<int>(calc.getSignToMove() == Sign::CROSS);

			int idx = 1;
			for (int row = 0; row < calc.getConfig().rows; row++)
				for (int col = 0; col < calc.getConfig().cols; col++, idx += 16)
				{
					const ThreatEncoding te = calc.getThreatAt(row, col);
					const ThreatType cross = std::max(ThreatType::OPEN_3, std::min(ThreatType::FIVE, te.forCross()));
					const ThreatType circle = std::max(ThreatType::OPEN_3, std::min(ThreatType::FIVE, te.forCircle()));
					tensor[idx + (int) cross - 2] = 1.0f;
					tensor[idx + 7 + (int) circle - 2] = 1.0f;
					switch (calc.signAt(row, col))
					{
						default:
							break;
						case Sign::CROSS:
							tensor[idx + 14] = 1.0f;
							break;
						case Sign::CIRCLE:
							tensor[idx + 15] = 1.0f;
							break;
					}
				}
		}
};

std::string convert(float f)
{
	if (f == 0.0f)
		return "0.0f";
	else
		return std::to_string(f) + 'f';
}

void train_nnue()
{
	ml::Device::setNumberOfThreads(1);
	GameDataBuffer buffer;
	for (int i = 200; i < 225; i++)
		buffer.load("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer/buffer_" + std::to_string(i) + ".bin");
	std::cout << buffer.getStats().toString() << '\n';

	GameConfig game_config(GameRules::STANDARD, 15);

	SearchDataPack pack(game_config.rows, game_config.cols);
	const int batch_size = 1024;
	nnue::TrainingNNUE_policy model(game_config, { 32, 32, 32 }, batch_size);

	for (int e = 0; e < 100; e++)
	{
		if (e == 50)
			model.setLearningRate(1.0e-4f);
		if (e == 75)
			model.setLearningRate(1.0e-5f);

		const double start = getTime();
		float loss = 0.0f;
		int count = 0;
		for (int step = 0; step < 1000; step++)
		{
			for (int b = 0; b < batch_size; b++)
			{
				const int i = randInt(buffer.numberOfGames());
				const int j = randInt(buffer.getGameData(i).numberOfSamples());

				buffer.getGameData(i).getSample(pack, j);
				const Symmetry r = int_to_symmetry(randInt(8));
				apply_symmetry_in_place(pack.board, r);
				apply_symmetry_in_place(pack.visit_count, r);
				model.packInputData(b, pack.board, pack.played_move.sign);
//				model.packTargetData(b, convertOutcome(pack.game_outcome, pack.played_move.sign).getExpectation());
//				model.packTargetData(b, pack.minimax_value.getExpectation());
				matrix<float> policy_target(pack.visit_count.rows(), pack.visit_count.cols());
				for (int k = 0; k < policy_target.size(); k++)
					policy_target[k] = pack.visit_count[k];
				normalize(policy_target);
				model.packTargetData(b, policy_target);
			}

			model.forward(batch_size);
			loss += model.backward(batch_size);
			model.learn();
			count++;
		}
		std::cout << "epoch " << e << ", loss " << loss / count << " in " << (getTime() - start) << "s\n";
		model.save("nnue_policy_s2_5x5_32x32x32x1.bin");
	}
}

void prepare_proven_dataset()
{
	SearchDataPack pack(15, 15);
	GameDataBuffer result;

	for (int b = 100; b <= 150; b++)
	{
		GameDataBuffer buffer;
		buffer.load("/home/maciek/alphagomoku/new_runs/test8_test/train_buffer/buffer_" + std::to_string(b) + ".bin");
//		buffer.load("/home/maciek/alphagomoku/new_runs/test8_test/valid_buffer/buffer_" + std::to_string(b) + ".bin");

		for (int i = 0; i < buffer.numberOfGames(); i++)
		{
			if (i % (buffer.numberOfGames() / 10) == 0)
				std::cout << i << " / " << buffer.numberOfGames() << '\n';

			for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
			{
				buffer.getGameData(i).getSample(pack, j);
				if (pack.minimax_score.isProven())
				{
					GameDataStorage tmp(15, 15, 100);
					for (int k = 0; k < buffer.getGameData(i).numberOfMoves(); k++)
						tmp.addMove(buffer.getGameData(i).getMove(k));
					tmp.addSample(pack);
					result.addGameData(tmp);
					break;
				}
			}
		}
	}

	std::cout << result.getStats().toString() << '\n';
	result.save("/home/maciek/alphagomoku/proven_positions_big.bin");
}

void test_proven_search(int mcts_nodes, int tss_nodes, bool fast)
{
//	GameConfig game_config(GameRules::FREESTYLE, 15);
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::RENJU, 15);
//	GameConfig game_config(GameRules::CARO5, 15);
//	GameConfig game_config(GameRules::CARO6, 20);

	GameDataBuffer buffer("/home/maciek/alphagomoku/proven_positions.bin");
	std::cout << buffer.getStats().toString() << '\n';

	SearchConfig search_config;
	search_config.max_batch_size = 32;
	search_config.mcts_config.edge_selector_config.exploration_constant = 1.25f;
	search_config.mcts_config.max_children = 32;
	search_config.tss_config.mode = 2;
	search_config.tss_config.max_positions = tss_nodes;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.device = ml::Device::cpu();
	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(false);
	nn_evaluator.loadGraph("/home/maciek/Desktop/AlphaGomoku560/networks/standard_conv_8x128.bin");
//	nn_evaluator.loadGraph("./old_6x64s.bin");

	Search search(game_config, search_config);
	Tree tree(search_config.tree_config);

	SearchDataPack pack(game_config.rows, game_config.cols);
	int total_samples = 0;
	int solved_count = 0;
	int total_visits = 0;

	const int count = fast ? 1000 : buffer.numberOfGames();

	for (int i = 0; i < buffer.numberOfGames(); i += (buffer.numberOfGames() / count))
//	int i = 0;
	{
		if (i % (count / 10) == 0)
			std::cout << "processed " << i << " / " << count << ", solved " << solved_count << "/" << total_samples << ", visits " << total_visits
					<< '\n';

		total_samples += buffer.getGameData(i).numberOfSamples();
		for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
		{
			buffer.getGameData(i).getSample(pack, j);

			tree.setBoard(pack.board, pack.played_move.sign);
			search.setBoard(pack.board, pack.played_move.sign);
			tree.setEdgeSelector(PUCTSelector(search_config.mcts_config.edge_selector_config));
			tree.setEdgeGenerator(UnifiedGenerator(search_config.mcts_config.max_children, search_config.mcts_config.policy_expansion_threshold));

			for (int k = 0; k <= mcts_nodes; k++)
			{
				search.select(tree, mcts_nodes);
				search.solve();
				search.scheduleToNN(nn_evaluator);
				nn_evaluator.evaluateGraph();

				search.generateEdges(tree);
				search.expand(tree);
				search.backup(tree);
				if (tree.isRootProven())
					break;
			}
			search.cleanup(tree);

			const Node info = tree.getInfo( { });
			solved_count += info.getScore().isProven();
			total_visits += info.getVisits();
//			if (info.getScore().isProven())
//			{
//				pack.print();
//				tree.printSubtree(5);
//				std::cout << info.toString() << '\n';
//				for (int k = 0; k < info.numberOfEdges(); k++)
//					std::cout << info.getEdge(k).toString() << '\n';
//				std::cout << i << '\n';
//				exit(0);
//			}
		}

	}

//	search.getSolver().print_stats();
	std::cout << search.getStats().toString() << '\n';
	std::cout << tree.getNodeCacheStats().toString() << '\n';
	std::cout << '\n' << "solved " << solved_count << "/" << total_samples << " (" << 100.0f * solved_count / total_samples << "%)\n";
	std::cout << "total visits = " << total_visits << '\n';
}

template<typename T>
double matrix_L1_diff(const matrix<T> &lhs, const matrix<T> &rhs)
{
	assert(equal_shape(lhs, rhs));
	double result = 0.0;
	for (int i = 0; i < lhs.size(); i++)
		result += std::abs(lhs[i] - rhs[i]) / (1.0e-8 + lhs[i]);
	return result;
}

double matrix_L1_diff(const matrix<Value> &lhs, const matrix<Value> &rhs)
{
	assert(equal_shape(lhs, rhs));
	double result = 0.0;
	for (int i = 0; i < lhs.size(); i++)
		result += std::abs(lhs[i].win_rate - rhs[i].win_rate) / (1.0e-8 + lhs[i].win_rate)
				+ std::abs(lhs[i].draw_rate - rhs[i].draw_rate) / (1.0e-8 + lhs[i].draw_rate);
	return result * 0.5;
}

void convert_dataset(const std::string &src, const std::string &dst, bool run_check = false)
{
	GameDataBuffer buffer_v100(src);
	std::cout << buffer_v100.getStats().toString() << '\n';

	const int rows = buffer_v100.getConfig().rows;
	const int cols = buffer_v100.getConfig().cols;

	SearchDataPack pack_v100(rows, cols);
	SearchDataPack pack_v200(rows, cols);

	double mean_policy_error = 0.0;
	double mean_value_error = 0.0;
	double mean_visit_error = 0.0;
	int data_count = 0;

	GameDataBuffer buffer_v200(buffer_v100.getConfig());
	for (int i = 0; i < buffer_v100.numberOfGames(); i++)
	{
		const GameDataStorage gds100 = buffer_v100.getGameData(i);
		GameDataStorage gds200(rows, cols, 200);

		gds200.setOutcome(gds100.getOutcome());
		for (int j = 0; j < gds100.numberOfMoves(); j++)
			gds200.addMove(gds100.getMove(j));

		for (int j = 0; j < gds100.numberOfSamples(); j++)
		{
			pack_v100.clear();
			pack_v200.clear();
			gds100.getSample(pack_v100, j);
			gds200.addSample(pack_v100);
		}
		buffer_v200.addGameData(gds200);
	}

	buffer_v200.save(dst);

	if (not run_check)
		return;

	buffer_v200.clear();
	buffer_v200.load(dst);

	for (int i = 0; i < buffer_v200.numberOfGames(); i++)
	{
		const GameDataStorage gds100 = buffer_v100.getGameData(i);
		const GameDataStorage gds200 = buffer_v200.getGameData(i);

		for (int j = 0; j < gds200.numberOfSamples(); j++)
		{
			pack_v100.clear();
			pack_v200.clear();
			gds100.getSample(pack_v100, j);
			gds200.getSample(pack_v200, j);

//			std::cout << "---OLD-FORMAT---\n";
//			pack_v100.print();
//			SearchDataStorage v100;
//			v100.loadFrom(pack_v100);
//			v100.print();
//
//			std::cout << "---NEW-FORMAT---\n";
//			pack_v200.print();
			SearchDataStorage_v2 v200;
			v200.loadFrom(pack_v200);
//			v200.print();
//			std::cout << "\n\n\n";
//			exit(0);
//			v200.storeTo(pack_v200);
//			if (matrix_L1_diff(pack_v100.visit_count, pack_v200.visit_count) > 1.0)
//			{
//				std::cout << i << " " << j << '\n';
//				std::cout << "---OLD-FORMAT---\n";
//				std::cout << Board::toString(matrix<Sign>(15, 15), pack_v100.visit_count) << '\n';
//				v100.print();
//				std::cout << "---NEW-FORMAT---\n";
//				std::cout << Board::toString(matrix<Sign>(15, 15), pack_v200.visit_count) << '\n';
//				v200.print();
//				exit(0);
//			}
//			exit(0);
			mean_policy_error += matrix_L1_diff(pack_v100.policy_prior, pack_v200.policy_prior);
			mean_value_error += matrix_L1_diff(pack_v100.action_values, pack_v200.action_values);
			mean_visit_error += matrix_L1_diff(pack_v100.visit_count, pack_v200.visit_count);
			data_count += v200.numberOfEntries();
		}
	}
	std::cout << "mean policy error = " << mean_policy_error / data_count << '\n';
	std::cout << "mean value  error = " << mean_value_error / data_count << '\n';
	std::cout << "mean visit  error = " << mean_visit_error / data_count << '\n';
}

void transfer_learning()
{
	ml::Device::setNumberOfThreads(1);
	GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/transfer/saved_state/buffer.bin");
//	buffer.load("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/transfer/valid_buffer/buffer_0.bin");
	std::cout << buffer.getStats().toString() << '\n';

	const int batch_size = 256;

	GameConfig game_config(GameRules::STANDARD, 15);
	TrainingConfig training_config;
	training_config.network_arch = "ResnetOld";
	training_config.augment_training_data = true;
	training_config.blocks = 6;
	training_config.filters = 64;
	training_config.device_config.batch_size = batch_size;
	training_config.l2_regularization = 1.0e-4f;

	std::unique_ptr<AGNetwork> teacher = loadAGNetwork("./old_10x128s.bin");
//	std::unique_ptr<AGNetwork> teacher = loadAGNetwork(
//			"/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/corrected/checkpoint/network_99_opt.bin");
	teacher->setBatchSize(batch_size);
	teacher->moveTo(ml::Device::cuda(0));

	std::unique_ptr<AGNetwork> network = createAGNetwork(training_config.network_arch);
	network->init(game_config, training_config);
	network->moveTo(ml::Device::cuda(0));
	network->changeLearningRate(1.0e-3f);

	SamplerVisits sampler;
//	sampler.init(buffer, batch_size);

	std::vector<TrainingDataPack> training_batch;
	for (size_t i = 0; i < batch_size; i++)
		training_batch.push_back(TrainingDataPack(15, 15));

	const std::string path = "/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/corrected_full/checkpoint/";
	for (int e = 0; e < 160; e++)
	{
		std::vector<float> loss(2, 0.0f);
		if (e == 80)
			network->changeLearningRate(1.0e-4f);
		if (e == 120)
			network->changeLearningRate(1.0e-5f);
		for (int i = 0; i < 1000; i++)
		{
			if (i % 100 == 0)
				std::cout << i << '\n';
			for (int b = 0; b < batch_size; b++)
			{
				sampler.get(training_batch.at(b));
				if (training_config.augment_training_data)
					ag::apply_symmetry_in_place(training_batch.at(b).board, int_to_symmetry(randInt(8)));
				teacher->packInputData(b, training_batch.at(b).board, training_batch.at(b).sign_to_move);
				network->packInputData(b, training_batch.at(b).board, training_batch.at(b).sign_to_move);
			}
			teacher->forward(batch_size);

//			for (int b = 0; b < batch_size; b++)
//			{
//				teacher->unpackOutput(b, training_batch.at(b).policy_target, training_batch.at(b).action_values_target,
//						training_batch.at(b).value_target);
//				network->packTargetData(b, training_batch.at(b).policy_target, training_batch.at(b).action_values_target,
//						training_batch.at(b).value_target);
//			}
			network->train(batch_size);
			std::vector<float> tmp = network->getLoss(batch_size);
			loss[0] += tmp[0];
			loss[1] += tmp[1];
		}
		std::cout << "epoch " << e << " : " << loss[0] / 1000 << " " << loss[1] / 1000 << '\n';
		network->saveToFile(path + "network_" + std::to_string(e) + ".bin");

		std::unique_ptr<AGNetwork> tmp = loadAGNetwork(path + "network_" + std::to_string(e) + ".bin");
		tmp->optimize();
		tmp->saveToFile(path + "network_" + std::to_string(e) + "_opt.bin");
	}
}

#include <minml/layers/Conv2D.hpp>
#include <minml/layers/Dense.hpp>
#include <minml/layers/Add.hpp>
#include <minml/layers/BatchNormalization.hpp>
#include <minml/layers/Softmax.hpp>

void compare_models()
{
	const ml::Shape input_shape( { 32, 15, 15, 4 });
	const int blocks = 6;
	const int filters = 64;

	ml::Graph graph;
	auto x = graph.addInput(input_shape);

	x = graph.add(ml::Conv2D(filters, 5, "linear").useBias(false), x);
	x = graph.add(ml::BatchNormalization("relu").useGamma(true), x);

	for (int i = 0; i < blocks; i++)
	{
		auto y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
		y = graph.add(ml::BatchNormalization("relu").useGamma(true), y);

		y = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), y);
		y = graph.add(ml::BatchNormalization("linear").useGamma(true), y);

		x = graph.add(ml::Add("relu"), { x, y });
	}

// policy head
	auto p = graph.add(ml::Conv2D(filters, 3, "linear").useBias(false), x);
	p = graph.add(ml::BatchNormalization("relu").useGamma(true), p);

	p = graph.add(ml::Conv2D(1, 1, "linear").useBias(false), p);
	p = graph.add(ml::Dense(15 * 15, "linear"), p);
	p = graph.add(ml::Softmax( { 1 }), p);
	graph.addOutput(p);

// value head
	auto v = graph.add(ml::Conv2D(2, 1, "linear").useBias(false), x);
	v = graph.add(ml::BatchNormalization("relu").useGamma(true), v);

	v = graph.add(ml::Dense(std::min(256, 2 * filters), "linear").useBias(false), v);
	v = graph.add(ml::BatchNormalization("relu").useGamma(true), v);
	v = graph.add(ml::Dense(3, "linear"), v);
	v = graph.add(ml::Softmax( { 1 }), v);
	graph.addOutput(v);

	graph.init();
	graph.setOptimizer(ml::RAdam());
//	graph.setRegularizer(ml::RegularizerL2(1.0e-4f));
	graph.moveTo(ml::Device::cpu());
//	graph.moveTo(ml::Device::cuda(0));

	for (int i = 0; i < 1; i++)
	{
		std::cout << "step " << i << " -----------------------------------------\n";
		ml::testing::initForTest(graph.getInput(), 0.1 * i);
		graph.predict(input_shape[0]);
		graph.context().synchronize();
		std::cout << '\n';
		graph.train(input_shape[0]);
		graph.context().synchronize();
	}
	std::cout << "inference-----------------------------------------\n";
	graph.makeTrainable(false);
	ml::testing::initForTest(graph.getInput(), 0);
	graph.predict(input_shape[0]);
}

float square(float x)
{
	return x * x;
}
float cross_entropy(const float *target, const float *output, int size)
{
	float result = 0.0;
	for (int i = 0; i < size; i++)
	{
		result += -target[i] * log_eps(output[i]) - (1.0f - target[i]) * log_eps(1.0f - output[i]);
		result -= -target[i] * log_eps(target[i]) - (1.0f - target[i]) * log_eps(1.0f - target[i]);
	}
	return result;
}
float cross_entropy(Value target, Value output)
{
	float t[3] = { std::max(0.0f, target.win_rate), std::max(0.0f, target.draw_rate), std::max(0.0f, target.loss_rate()) };
	float o[3] = { std::max(0.0f, output.win_rate), std::max(0.0f, output.draw_rate), std::max(0.0f, output.loss_rate()) };
	return cross_entropy(t, o, 3);
}

namespace evaluation
{
	class Player
	{
		protected:
			std::string name;
			Sign sign = Sign::NONE;
		public:
			virtual ~Player() = default;
			virtual void setSign(Sign s) noexcept
			{
				sign = s;
			}
			Sign getSign() const noexcept
			{
				return sign;
			}
			std::string getName() const
			{
				return name;
			}
			virtual void setBoard(const matrix<Sign> &board, Sign signToMove) = 0;
			virtual Move getMove() const noexcept = 0;
			virtual std::unique_ptr<Player> clone() const = 0;
	};
	class MinimaxPlayer: public Player
	{
			SearchTask task;
			MinimaxSearch search;
			int max_depth;
			MoveGeneratorMode mode;
		public:
			MinimaxPlayer(const std::string &name, GameConfig cfg, int depth, MoveGeneratorMode mode) :
					task(cfg),
					search(cfg),
					max_depth(depth),
					mode(mode)
			{
				this->name = name;
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove)
			{
				task.set(board, signToMove);
				search.solve(task, max_depth, mode);
			}
			Move getMove() const noexcept
			{
				matrix<Score> scores = task.getActionScores();
				Score best_score = Score::min_value();
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) > best_score and task.getBoard().at(row, col) == Sign::NONE)
							best_score = scores.at(row, col);

				std::vector<Move> moves;
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) == best_score and task.getBoard().at(row, col) == Sign::NONE)
							moves.emplace_back(task.getSignToMove(), row, col);
				return moves.at(randInt(moves.size()));
			}
			std::unique_ptr<Player> clone() const
			{
				return std::make_unique<MinimaxPlayer>(name, task.getConfig(), max_depth, mode);
			}
	};
	class PolicyHeadPlayer: public Player
	{
			SearchTask task;
			std::string path_to_network;
			NNEvaluator evaluator;
		public:
			PolicyHeadPlayer(const std::string &name, GameConfig cfg, const std::string &path) :
					task(cfg),
					path_to_network(path),
					evaluator(DeviceConfig())
			{
				this->name = name;
				evaluator.loadGraph(path);
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove)
			{
				task.set(board, signToMove);
				evaluator.addToQueue(task);
				evaluator.evaluateGraph();
			}
			Move getMove() const noexcept
			{
				matrix<float> policy = task.getPolicy();
				Move best_move;
				float best_policy = std::numeric_limits<float>::lowest();
				for (int row = 0; row < policy.rows(); row++)
					for (int col = 0; col < policy.cols(); col++)
						if (policy.at(row, col) > best_policy and task.getBoard().at(row, col) == Sign::NONE)
						{
							best_policy = policy.at(row, col);
							best_move = Move(row, col, task.getSignToMove());
						}
				return best_move;
			}
			std::unique_ptr<Player> clone() const
			{
				return std::make_unique<PolicyHeadPlayer>(name, task.getConfig(), path_to_network);
			}
	};
	class AlphaBetaPlayer: public Player
	{
			SearchTask task;
			AlphaBetaSearch search;
			int max_depth, max_nodes;
			MoveGeneratorMode mode;
			Score best_score = Score::minus_infinity();
			matrix<Sign> last_board;
		public:
			AlphaBetaPlayer(const std::string &name, GameConfig cfg, int depth, int nodes, MoveGeneratorMode mode) :
					task(cfg),
					search(cfg),
					max_depth(depth),
					max_nodes(nodes),
					mode(mode)
			{
				this->name = name;
				search.loadWeights(nnue::NNUEWeights("/home/maciek/Desktop/AlphaGomoku560/networks/standard_nnue_64x16x16x1.bin"));
				search.setDepthLimit(max_depth);
				search.setNodeLimit(max_nodes);
			}
			void setSign(Sign s) noexcept
			{
				Player::setSign(s);
				best_score = Score::minus_infinity();
			}
			void setBoard(const matrix<Sign> &board, Sign signToMove)
			{
				task.set(board, signToMove);
				search.solve(task);
				const Score s = task.getScore();
				if (best_score.isWin() and not s.isWin())
				{
					std::cout << "Using " << (int) mode << " mode\n";
					std::cout << "score changed from " << best_score << " to " << s << '\n';

					std::cout << signToMove << " to move\n";
					std::cout << Board::toString(last_board, true);
					PatternCalculator calc(task.getConfig());
					calc.setBoard(last_board, signToMove);
					calc.printAllThreats();

					std::cout << "current board\n" << Board::toString(board, true);

					exit(-2);
				}
				best_score = std::max(best_score, s);
				last_board = board;
			}
			Move getMove() const noexcept
			{
				matrix<Score> scores = task.getActionScores();
				Score best_score = Score::min_value();
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) > best_score and task.getBoard().at(row, col) == Sign::NONE)
							best_score = scores.at(row, col);

				std::vector<Move> moves;
				for (int row = 0; row < scores.rows(); row++)
					for (int col = 0; col < scores.cols(); col++)
						if (scores.at(row, col) == best_score and task.getBoard().at(row, col) == Sign::NONE)
							moves.emplace_back(task.getSignToMove(), row, col);
				return moves.at(randInt(moves.size()));
			}
			std::unique_ptr<Player> clone() const
			{
				return std::make_unique<AlphaBetaPlayer>(name, task.getConfig(), max_depth, max_nodes, mode);
			}
	};

	class EvaluationGame
	{
		private:
			struct GameResult
			{
					std::string cross_player;
					std::string circle_player;
					GameOutcome outcome;
			};
			GameConfig game_config;
			int games_to_play;
			std::vector<std::vector<Move>> openings;
			std::vector<GameResult> game_results;
			std::mutex main_mutex;

			std::unique_ptr<Player> first_player;
			std::unique_ptr<Player> second_player;
			std::vector<std::future<void>> threads;
		public:
			EvaluationGame(GameConfig gameConfig, int gamesToPlay, int numberOfThreads) :
					game_config(gameConfig),
					games_to_play(gamesToPlay),
					threads(numberOfThreads)
			{
				openings = generate_openings(gamesToPlay / 2, game_config, "/home/maciek/Desktop/AlphaGomoku560/networks/standard_conv_8x128.bin");
			}
			void start()
			{
				for (size_t i = 0; i < threads.size(); i++)
				{
					threads[i] = std::async(std::launch::async, [this]()
					{	this->run();});
				}
			}
			void waitForFinish()
			{
				for (size_t i = 0; i < threads.size(); i++)
				{
					for (int j = 0;; j++)
					{
						if (j % 60 == 0)
						{
							std::lock_guard<std::mutex> lg(main_mutex);
							std::cout << openings.size() * 2 << " games left\n";
							summarize_results();
						}
						if (threads[i].valid())
						{
							std::future_status input_status = threads[i].wait_for(std::chrono::milliseconds(1000));
							if (input_status == std::future_status::ready) // there is some input to process
							{
								threads[i].get();
								break;
							}
						}
						else
							break;
					}
				}
				summarize_results();
			}
			void setPlayers(const Player &firstPlayer, const Player &secondPlayer)
			{
				first_player = firstPlayer.clone();
				second_player = secondPlayer.clone();
			}
			void run()
			{
				for (;;)
				{
					std::vector<Move> opening;
					{ /* artificial scope for lock */
						std::lock_guard<std::mutex> lg(main_mutex);
						if (openings.empty())
							return;
						opening = openings.back();
						openings.pop_back();
					}

					std::unique_ptr<Player> player0 = first_player->clone();
					std::unique_ptr<Player> player1 = second_player->clone();

					const GameOutcome outcome1 = play_single_game(*player0, *player1, opening);
					const GameResult gr1 { player0->getName(), player1->getName(), outcome1 };

					const GameOutcome outcome2 = play_single_game(*player1, *player0, opening);
					const GameResult gr2 { player1->getName(), player0->getName(), outcome2 };

					std::lock_guard<std::mutex> lg(main_mutex);
					game_results.push_back(gr1);
					game_results.push_back(gr2);
				}
			}
		private:
			GameOutcome play_single_game(Player &crossPlayer, Player &circlePlayer, const std::vector<Move> &opening)
			{
				crossPlayer.setSign(Sign::CROSS);
				circlePlayer.setSign(Sign::CIRCLE);

				Game game(game_config);
				game.loadOpening(opening);
				while (not game.isOver())
				{
					std::cout << "Move " << game.numberOfMoves() << ", " << game.getSignToMove() << " to move\n";
					std::cout << Board::toString(game.getBoard(), true);
					Move move;
					if (game.getSignToMove() == Sign::CROSS)
					{
						crossPlayer.setBoard(game.getBoard(), game.getSignToMove());
						move = crossPlayer.getMove();
					}
					else
					{
						circlePlayer.setBoard(game.getBoard(), game.getSignToMove());
						move = circlePlayer.getMove();
					}
					game.makeMove(move);
				}
				return game.getOutcome();
			}
			void summarize_results() const
			{
				int wins = 0, draws = 0, losses = 0;
				for (size_t i = 0; i < game_results.size(); i++)
				{
					switch (game_results[i].outcome)
					{
						case GameOutcome::UNKNOWN:
							break;
						case GameOutcome::DRAW:
							draws++;
							break;
						case GameOutcome::CROSS_WIN:
							if (game_results[i].cross_player == first_player->getName())
								wins++;
							else
								losses++;
							break;
						case GameOutcome::CIRCLE_WIN:
							if (game_results[i].circle_player == first_player->getName())
								wins++;
							else
								losses++;
							break;
					}
				}
				std::cout << "'" << first_player->getName() << "' vs '" << second_player->getName() << "':\n";
				std::cout << wins << " : " << draws << " : " << losses << '\n';

				const double ratio = (wins + 0.5 * draws) / (wins + draws + losses);
				std::cout << "win ratio = " << ratio << "\n";
				const double elo_diff = 177.0 * (log_eps(ratio) - log_eps(1.0 - ratio));
				std::cout << "estimated elo difference = " << std::fabs(elo_diff) << "\n\n";
			}
	};
}

void test_quantization()
{
	const int batch_size = 1024;
	const int bits = 12;

	Dataset dataset;
	for (int i = 240; i < 250; i++)
		dataset.load(i, "/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer_v200/buffer_" + std::to_string(i) + ".bin");
	std::cout << dataset.getStats().toString() << '\n';

	std::unique_ptr<AGNetwork> network = loadAGNetwork(
			"/home/maciek/alphagomoku/new_runs_2025/test_pooling/pvq_8x128_cosine_150_250_350_450/network_swa_opt.bin");
	network->get_graph().print();
	for (int i = 0; i < network->get_graph().numberOfNodes(); i++)
	{
		ml::Layer &layer = network->get_graph().getNode(i).getLayer();
		std::cout << layer.name() << " " << layer.getOutputShape() << " " << layer.isQuantizable() << '\n';
//		if (i >= 2)
//			layer.quantizable(false);
	}

	network->setBatchSize(batch_size);
	network->moveTo(ml::Device::cuda());
	ml::CalibrationTable calibration_table(16384, 1.0e-5f, 1 << bits);
	calibration_table.init(network->get_graph().numberOfNodes());
	calibration_table.getHistogram(0).setBinary();

	TrainingDataPack pack(15, 15);
	SamplerVisits sampler;

	sampler.init(dataset, batch_size);
	for (int i = 0; i < 1000; i++)
	{
		for (int j = 0; j < batch_size; j++)
		{
			sampler.get(pack);
			network->packInputData(j, pack.board, pack.sign_to_move);
		}
		network->forward(batch_size);

		network->get_graph().calibrate(calibration_table);
		if (calibration_table.isReady())
		{
			std::cout << "table is ready after " << (i + 1) * batch_size << " samples\n";
			break;
		}
		else
			std::cout << "table is " << (int) (100 * calibration_table.getCompletionFactor()) << "% ready\n";
	}

	for (int i = 0; i < calibration_table.size(); i++)
		std::cout << calibration_table.getHistogram(i).getInfo() << '\n';

	network->moveTo(ml::Device::cpu());
	ml::Quantize quantizer;
	quantizer.optimize(network->get_graph(), bits);
	network->get_graph().print();

//	network->moveTo(ml::Device::cuda());

// @formatter:off
	const matrix<Sign> board = Board::fromString(
				/*         a b c d e f g h i j k l m n o          */
				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  2 */
				/*  3 */ " _ _ _ _ _ _ X O _ X _ _ _ _ _\n" /*  3 */
				/*  4 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  4 */
				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
				/*  6 */ " _ _ _ _ X _ _ _ _ X _ _ _ _ _\n" /*  6 */
				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */ " _ _ X _ _ O _ _ _ _ _ O _ _ _\n" /*  8 */
				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
				/*         a b c d e f g h i j k l m n o          */);
// @formatter:on
	const Sign sign_to_move = Sign::CROSS;
	for (int i = 0; i < network->getBatchSize(); i++)
		network->packInputData(i, board, sign_to_move);
	network->forward(1);

	matrix<float> policy(board.rows(), board.cols());
	matrix<Value> action_values(board.rows(), board.cols());
	Value value;
	float moves_left = 0.0f;

	network->unpackOutput(0, policy, action_values, value, moves_left);

	std::cout << "value = " << value.toString() << '\n';
	std::cout << "moves left = " << moves_left << '\n';
	std::cout << Board::toString(board, policy) << '\n';

//	network->saveToFile("/home/maciek/alphagomoku/new_runs_2025/final_tests/test_int8/network_swa_opt_int8.bin");

	std::cout << "END" << std::endl;
}

void prepare_selection_dataset(int max_simulations)
{
	SearchDataPack pack(15, 15);
	GameDataBuffer result;

//	GameConfig game_config(GameRules::FREESTYLE, 15);
	GameConfig game_config(GameRules::STANDARD, 15);
//	GameConfig game_config(GameRules::RENJU, 15);
//	GameConfig game_config(GameRules::CARO5, 15);
//	GameConfig game_config(GameRules::CARO6, 20);

	SearchConfig search_config;
	search_config.max_batch_size = 32;
	search_config.mcts_config.edge_selector_config.exploration_constant = 0.5f;
	search_config.mcts_config.max_children = 32;
	search_config.tss_config.max_positions = 100;

	DeviceConfig device_config;
	device_config.batch_size = 32;
	device_config.device = ml::Device::cuda();

	NNEvaluator nn_evaluator(device_config);
	nn_evaluator.useSymmetries(true);
	nn_evaluator.loadGraph("/home/maciek/alphagomoku/final_runs_2025/standard_15x15/network_swa.bin");

	Search search(game_config, search_config);
	Tree tree(search_config.tree_config);

	EdgeSelectorConfig config;
	config.exploration_constant = 0.2f;
	LCBSelector selector(config);

	int match_count = 0;
	int total_count = 0;

	const double start = getTime();
	for (int b = 0; b < 1; b++)
	{
		GameDataBuffer buffer;
		buffer.load("/home/maciek/alphagomoku/final_runs_2025/buffer_0.bin");
		std::cout << buffer.getStats().toString() << '\n';

		for (int i = 1110; i < buffer.numberOfGames(); i++)
		{
			std::cout << "game " << i << "\n";

			const int samples_count = buffer.getGameData(i).numberOfSamples();
			const std::vector<int> order = permutation(samples_count);
			bool found_unproven_state = false;
			for (size_t j = 0; j < order.size(); j++)
			{
				buffer.getGameData(i).getSample(pack, order[j]);
				std::cout << "trying " << order[j] << " : " << pack.minimax_score << '\n';
				if (reduce_add(pack.visit_count) > 100 or not pack.minimax_score.isProven())
				{
					found_unproven_state = true;
					break;
				}
			}

			if (found_unproven_state)
			{
				tree.setBoard(pack.board, pack.played_move.sign);
				search.setBoard(pack.board, pack.played_move.sign);
				tree.setEdgeSelector(PUCTSelector(search_config.mcts_config.edge_selector_config));
				tree.setEdgeGenerator(UnifiedGenerator(search_config.mcts_config.max_children, search_config.mcts_config.policy_expansion_threshold));

				int sample_visits = 5; //1.0 + std::exp(randFloat() * std::log(max_simulations - 1));
				std::cout << "sampling at " << sample_visits << " visits\n";

				Node sample_info;

				for (int k = 0; k <= max_simulations; k++)
				{
					search.setBatchSize(std::max(1.0, std::min((double) search_config.max_batch_size, std::sqrt(tree.getSimulationCount()))));
					search.select(tree, max_simulations);
					search.solve();
					search.scheduleToNN(nn_evaluator);
					nn_evaluator.evaluateGraph();

					search.generateEdges(tree);
					search.expand(tree);
					search.backup(tree);
					if (tree.getSimulationCount() >= sample_visits)
					{
						sample_visits = std::numeric_limits<int>::max();
						sample_info = tree.getInfo( { });
					}
//					if (tree.isRootProven())
//						break;
				}
				search.cleanup(tree);

				const Node target_info = tree.getInfo( { });

				const Move best_move_at_sampling = selector.select(&sample_info)->getMove();
				const Move best_move_at_target = selector.select(&target_info)->getMove();

//				if (best_move_at_sampling != best_move_at_target)
//				{
				std::cout << best_move_at_sampling.text() << " vs " << best_move_at_target.text() << '\n';

				std::cout << "Sample:\n";
				sample_info.sortEdges();
				std::cout << sample_info.toString() << '\n';
				for (int i = 0; i < sample_info.numberOfEdges(); i++)
					std::cout << sample_info.getEdge(i).getMove().toString() << " : " << sample_info.getEdge(i).toString() << '\n';
				std::cout << '\n';

				std::cout << "Target:\n";
				target_info.sortEdges();
				std::cout << target_info.toString() << '\n';
				for (int i = 0; i < target_info.numberOfEdges(); i++)
					std::cout << target_info.getEdge(i).getMove().toString() << " : " << target_info.getEdge(i).toString() << '\n';
				std::cout << '\n';
				return;
//				}
				match_count += (best_move_at_sampling == best_move_at_target);
				total_count++;
			}

			if (i % (buffer.numberOfGames() / 10000) == 0)
			{
				std::cout << i << " / " << buffer.numberOfGames() << '\n';
				const double stop = getTime();
				std::cout << (stop - start) / total_count << "s per sample\n";
			}
		}
	}

	std::cout << match_count << "/" << total_count << " (" << 100 * match_count / total_count << "%)\n";

//	std::cout << result.getStats().toString() << '\n';
//	result.save("/home/maciek/alphagomoku/proven_positions_big.bin");
}

float inv_erf(float p) noexcept
{
	float x = 0.5f;
	for (int i = 0; i < 10; i++)
	{
		const float fx = std::erf(x) - p;
		const float dfx = 2.0f / std::sqrt(M_PI) * std::exp(-x * x);
		x -= fx / dfx;
	}
	return x;
}

float my_logf(float a)
{
	float i, m, r, s, t;
	int e;

	m = frexpf(a, &e);
	if (m < 0.666666667f)
	{ // 0x1.555556p-1
		m = m + m;
		e = e - 1;
	}
	i = (float) e;
	/* m in [2/3, 4/3] */
	m = m - 1.0f;
	s = m * m;
	/* Compute log1p(m) for m in [-1/3, 1/3] */
	r = -0.130310059f;  // -0x1.0ae000p-3
	t = 0.140869141f;  //  0x1.208000p-3
	r = fmaf(r, s, -0.121484190f); // -0x1.f19968p-4
	t = fmaf(t, s, 0.139814854f); //  0x1.1e5740p-3
	r = fmaf(r, s, -0.166846052f); // -0x1.55b362p-3
	t = fmaf(t, s, 0.200120345f); //  0x1.99d8b2p-3
	r = fmaf(r, s, -0.249996200f); // -0x1.fffe02p-3
	r = fmaf(t, m, r);
	r = fmaf(r, m, 0.333331972f); //  0x1.5554fap-2
	r = fmaf(r, m, -0.500000000f); // -0x1.000000p-1
	r = fmaf(r, s, m);
	r = fmaf(i, 0.693147182f, r); //  0x1.62e430p-1 // log(2)
	if (!((a > 0.0f) && (a <= 3.40282346e+38f)))
	{ // 0x1.fffffep+127
		r = a + a;  // silence NaNs if necessary
		if (a < 0.0f)
			r = (0.0f / 0.0f); //  NaN
		if (a == 0.0f)
			r = (-1.0f / 0.0f); // -Inf
	}
	return r;
}

float inv_erf_v2(float a)
{
	float p, r, t;
	t = fmaf(a, 0.0f - a, 1.0f);
	t = std::log(t);
	if (fabsf(t) > 6.125f)
	{ // maximum ulp error = 2.35793
		p = 3.03697567e-10f; //  0x1.4deb44p-32
		p = fmaf(p, t, 2.93243101e-8f); //  0x1.f7c9aep-26
		p = fmaf(p, t, 1.22150334e-6f); //  0x1.47e512p-20
		p = fmaf(p, t, 2.84108955e-5f); //  0x1.dca7dep-16
		p = fmaf(p, t, 3.93552968e-4f); //  0x1.9cab92p-12
		p = fmaf(p, t, 3.02698812e-3f); //  0x1.8cc0dep-9
		p = fmaf(p, t, 4.83185798e-3f); //  0x1.3ca920p-8
		p = fmaf(p, t, -2.64646143e-1f); // -0x1.0eff66p-2
		p = fmaf(p, t, 8.40016484e-1f); //  0x1.ae16a4p-1
	}
	else
	{ // maximum ulp error = 2.35002
		p = 5.43877832e-9f;  //  0x1.75c000p-28
		p = fmaf(p, t, 1.43285448e-7f); //  0x1.33b402p-23
		p = fmaf(p, t, 1.22774793e-6f); //  0x1.499232p-20
		p = fmaf(p, t, 1.12963626e-7f); //  0x1.e52cd2p-24
		p = fmaf(p, t, -5.61530760e-5f); // -0x1.d70bd0p-15
		p = fmaf(p, t, -1.47697632e-4f); // -0x1.35be90p-13
		p = fmaf(p, t, 2.31468678e-3f); //  0x1.2f6400p-9
		p = fmaf(p, t, 1.15392581e-2f); //  0x1.7a1e50p-7
		p = fmaf(p, t, -2.32015476e-1f); // -0x1.db2aeep-3
		p = fmaf(p, t, 8.86226892e-1f); //  0x1.c5bf88p-1
	}
	r = a * p;
	return r;
}

template<int Level = 0>
float inv_erf_v3(float p)
{
	const float sgn = (p < 0) ? -1.0f : 1.0f;

	const float lnx = std::log(1.0f - p * p);

	const float tt1 = 2.0f / (M_PI * 0.147f) + 0.5f * lnx;
	const float tt2 = 1.0f / 0.147f * lnx;

	const float x = sgn * std::sqrt(-tt1 + std::sqrt(tt1 * tt1 - tt2));

	if constexpr (Level == 1)
	{
		const float fx = std::erf(x) - p;
		const float dfx = 2.0f / std::sqrt(M_PI) * std::exp(-x * x);
		return x - fx / dfx;
	}
	else
		return x;
}

class Bandit
{
		float mean = 0.0f;
		float stddev = 0.0f;
		float cdf0 = 0.0f;
		float cdf1 = 1.0f;
	public:
		Bandit() :
				mean(-1.0f + 3.0f * randFloat()),
				stddev(0.1f + 0.9f * randFloat()),
				cdf0(gaussian_cdf(0.0f, mean, stddev)),
				cdf1(gaussian_cdf(1.0f, mean, stddev))
		{
		}
		float get_reward() const noexcept
		{
			const float p = cdf0 + randFloat() * (cdf1 - cdf0);
			return mean + std::sqrt(2.0f) * stddev * inverse_erf(2.0f * p - 1.0f);
		}
		std::string to_string() const
		{
			return "mean=" + std::to_string(mean) + ", stddev=" + std::to_string(stddev) + " (" + std::to_string(cdf0) + ", " + std::to_string(cdf1)
					+ ")";
		}
		float get_expectation(int N = 1000) const noexcept
		{
			float result = 0.0f;
			float sum_p = 0.0f;
			const float c = 1.0f / (stddev * std::sqrt(2.0f * M_PI));
			for (int n = 0; n <= N; n++)
			{
				const float x = n / static_cast<double>(N);
				const float p = c * std::exp(-(x - mean) * (x - mean) / (2.0f * stddev * stddev));
				const float w = (n == 0 or n == N) ? 0.5f : 1.0f;

				result += w * x * p;
				sum_p += w * p;
			}
			return result / sum_p;
		}
};

struct Choice
{
		int index = 0;
		float reward = 0.0f;
		Choice(int i, float r) noexcept :
				index(i),
				reward(r)
		{
		}
};

class Policy
{
	public:
		virtual ~Policy() = default;
		virtual Choice select(const std::vector<Bandit> &bandits) = 0;
};

std::vector<float> get_priors(const std::vector<Bandit> &bandits, int N)
{
	std::vector<float> result(bandits.size(), 0.0f);

	for (int n = 0; n < N; n++)
	{
		int best_idx = -1;
		float best_reward = std::numeric_limits<float>::lowest();
		for (size_t i = 0; i < bandits.size(); i++)
		{
			const float r = bandits[i].get_reward();
			if (r > best_reward)
			{
				best_reward = r;
				best_idx = i;
			}
		}
		result[best_idx] += 1.0f;
	}

	for (size_t i = 0; i < result.size(); i++)
		result[i] /= N;

	return result;
}
std::vector<float> get_values(const std::vector<Bandit> &bandits, int N)
{
	std::vector<float> result(bandits.size(), 0.0f);

	for (size_t i = 0; i < bandits.size(); i++)
	{
		for (int n = 0; n < N; n++)
			result[i] += bandits[i].get_reward();
		result[i] /= N;
	}
	return result;
}

class EpsilonGreedy: public Policy
{
		struct data
		{
				float reward_sum = 0.0f;
				int count = 0;

				float get_avg() const noexcept
				{
					return reward_sum / (0.001f + count);
				}
				void update(float r) noexcept
				{
					reward_sum += r;
					count++;
				}
		};

		std::vector<data> arm_stats;
		float epsilon = 0.0f;
	public:
		EpsilonGreedy(float eps) noexcept :
				epsilon(eps)
		{
		}
		Choice select(const std::vector<Bandit> &bandits)
		{
			if (arm_stats.size() != bandits.size())
				arm_stats = std::vector<data>(bandits.size());

			size_t best_idx = 0;
			float best_reward = std::numeric_limits<float>::lowest();
			for (size_t i = 0; i < bandits.size(); i++)
			{
				const float r = (arm_stats[i].count > 0) ? arm_stats[i].get_avg() : randFloat();
//				std::cout << i << " avg = " << arm_stats[i].get_avg() << " (" << arm_stats[i].count << ")\n";
				if (r > best_reward)
				{
					best_reward = r;
					best_idx = i;
				}
			}

			if (randFloat() < epsilon)
			{
				int r = randInt(bandits.size() - 1);
				for (size_t i = 0; i < bandits.size(); i++)
					if (i != best_idx)
					{
						if (r == 0)
						{
							best_idx = i;
							break;
						}
						r--;
					}
			}
			const float reward = bandits[best_idx].get_reward();
			arm_stats[best_idx].update(reward);

//			std::cout << "------------------------------------------\n";
			return Choice(best_idx, reward);
		}
};

class UpperConfidenceBound: public Policy
{
		struct data
		{
				float reward_sum = 0.0f;
				int count = 0;

				float get_avg() const noexcept
				{
					return reward_sum / (0.001f + count);
				}
				float get_bound(int N) const noexcept
				{
					return std::sqrt(std::log(N) / (0.001f + count));
				}
				void update(float r) noexcept
				{
					reward_sum += r;
					count++;
				}
		};

		std::vector<data> arm_stats;
		float exploration = 0.0f;
		int total_count = 0;
	public:
		UpperConfidenceBound(float c) noexcept :
				exploration(c)
		{
		}
		Choice select(const std::vector<Bandit> &bandits)
		{
			if (arm_stats.size() != bandits.size())
				arm_stats = std::vector<data>(bandits.size());

			total_count++;

			size_t best_idx = 0;
			float best_reward = std::numeric_limits<float>::lowest();
			for (size_t i = 0; i < bandits.size(); i++)
			{
				const float r = arm_stats[i].get_avg() + exploration * arm_stats[i].get_bound(total_count);
//				std::cout << i << " avg = " << arm_stats[i].get_avg() << " " << arm_stats[i].get_bound(total_count) << " (" << arm_stats[i].count
//						<< ")\n";
				if (r > best_reward)
				{
					best_reward = r;
					best_idx = i;
				}
			}

			const float reward = bandits[best_idx].get_reward();
			arm_stats[best_idx].update(reward);
//			std::cout << "------------------------------------------\n";
			return Choice(best_idx, reward);
		}
};

class PUCT: public Policy
{
		struct data
		{
				float prior = 0.0f;
				float reward_sum = 0.0f;
				int count = 0;

				float get_avg() const noexcept
				{
					return reward_sum / (0.001f + count);
				}
				float get_bound(int N) const noexcept
				{
					return prior * std::sqrt(N) / (1 + count);
				}
				void update(float r) noexcept
				{
					reward_sum += r;
					count++;
				}
		};

		std::vector<data> arm_stats;
		float exploration = 0.0f;
		int prior_accuracy = 0;
		int total_count = 0;
	public:
		PUCT(float c, int pa = 2) noexcept :
				exploration(c),
				prior_accuracy(pa)
		{
		}
		Choice select(const std::vector<Bandit> &bandits)
		{
			if (arm_stats.size() != bandits.size())
			{
				arm_stats = std::vector<data>(bandits.size());
				const std::vector<float> priors = get_priors(bandits, prior_accuracy * bandits.size());
				const std::vector<float> values = get_values(bandits, prior_accuracy);
				const std::vector<float> noise = createCustomNoise(priors.size());

				const float w = 0.25f;
				for (size_t i = 0; i < arm_stats.size(); i++)
				{
					arm_stats[i].prior = noise[i] * w + priors[i] * (1.0f - w);
					arm_stats[i].reward_sum = 0.001f * values[i];
				}
			}

			total_count++;

			size_t best_idx = 0;
			float best_reward = std::numeric_limits<float>::lowest();
			for (size_t i = 0; i < bandits.size(); i++)
			{
				const float Q = arm_stats[i].get_avg();
				const float U = exploration * arm_stats[i].get_bound(total_count);
//				std::cout << i << " avg = " << arm_stats[i].get_avg() << " " << arm_stats[i].get_bound(total_count) << " " << arm_stats[i].prior
//						<< " (" << arm_stats[i].count << ")\n";
				if ((Q + U) > best_reward)
				{
					best_reward = Q + U;
					best_idx = i;
				}
			}

			const float reward = bandits[best_idx].get_reward();
			arm_stats[best_idx].update(reward);
//			std::cout << "------------------------------------------\n";
			return Choice(best_idx, reward);
		}
};

int measure_regret(Policy &policy, const std::vector<Bandit> &bandits, int N)
{
	int best_idx = 0;
	float best_reward = std::numeric_limits<float>::lowest();
	for (size_t i = 0; i < bandits.size(); i++)
	{
		const float r = bandits[i].get_expectation();
		if (r > best_reward)
		{
			best_reward = r;
			best_idx = i;
		}
	}

	int regret = 0;

	for (int n = 0; n < N; n++)
	{
		const Choice c = policy.select(bandits);
		if (c.index != best_idx)
			regret++;
		if (n % 100 == 0)
			std::cout << regret << '\n';
	}

	return regret;
}

void asdf()
{
	std::vector<Bandit> bandits(10);

//	EpsilonGreedy policy(0.1f);
//	UpperConfidenceBound policy(0.4f);
	PUCT policy(0.1f);

	const int regret = measure_regret(policy, bandits, 10000);
	std::cout << "regret = " << regret << '\n';
	return;

//	const std::vector<float> priors = get_priors(bandits, 1000);
//
//	const double t0 = getTime();
//	for (int i = 0; i < 10000000; i++)
//	{
//		bandits[0].get_reward();
//		bandits[1].get_reward();
//		bandits[2].get_reward();
//		bandits[3].get_reward();
//		bandits[4].get_reward();
//		bandits[5].get_reward();
//		bandits[6].get_reward();
//		bandits[7].get_reward();
//		bandits[8].get_reward();
//		bandits[9].get_reward();
//	}
//	const double t1 = getTime();
//	std::cout << (t1 - t0) * 10.0 << "ns\n";
//
//	for (size_t i = 0; i < bandits.size(); i++)
//		std::cout << bandits[i].to_string() << "   " << priors[i] << "   " << bandits[i].get_expectation() << '\n';
}

int main(int argc, char *argv[])
{
	ml::Device::flushDenormalsToZero(true);
	std::cout << "BEGIN" << std::endl;
	std::cout << ml::Device::hardwareInfo() << '\n';

//	train_moves_left_network();
//	return 0;

	if (false)
	{
		TrainingConfig tcfg;
		tcfg.blocks = 4;
		tcfg.filters = 64;
		GameConfig cfg(GameRules::FREESTYLE, 15, 15);
		MovesLeftNetwork mln(cfg, tcfg);

		mln.setBatchSize(1);
		mln.optimize(1);
		mln.saveToFile("/home/maciek/cpp_workspace/AlphaGomoku/moves_left_network.bin");
		mln.moveTo(ml::Device::cpu());
		mln.optimize(2);
		mln.forward(1);
		mln.get_graph().print();

//		return 0;

		std::cout << "starting benchmark\n";
		const double start = getTime();
		int repeats = 0;
		for (; repeats < 1000000; repeats++)
		{
			mln.forward(mln.getBatchSize());
			if ((getTime() - start) > 30.0)
				break;
		}
		const double stop = getTime();
		const double time = stop - start;
		std::cout << "time = " << time << "s, repeats = " << repeats << ", n/s = " << mln.getBatchSize() * repeats / time << "\n";
		return 0;
	}

//	asdf();
//	return 0;

//	{
//		EngineSettings settings(FileLoader("/home/maciek/Desktop/AlphaGomoku590/config.json").getJson());
//		TimeManager tm;
//
//		settings.setOption(Option { "rows", "20" });
//		settings.setOption(Option { "columns", "20" });
//		settings.setOption(Option { "draw_after", "400" });
//		settings.setOption(Option { "rules", "FREESTYLE" });
//		settings.setOption(Option { "time_for_turn", "300000" });
//
//		int time_left = 180000;
//
//		for (int i = 0; i < 400; i++)
//		{
//			settings.setOption(Option { "time_left", std::to_string(time_left) });
//			const int dt = 1000 * tm.getTimeForTurn(settings, i, 0.5f);
//
//			std::cout << "move " << i << ", time_left " << time_left << ", dt " << dt << '\n';
//			time_left -= dt;
//		}
//		return 0;
//	}

//	{
//		std::string game =
//				"i7 k6 j9 i4 k5 j6 j5 i5 h4 i6 h6 h5 k9 j8 j7 j3 k2 f3 i3 g4 e2 g5 g6 f5 e5 h3 e6 g2 f6 d6 e4 e3 e7 e8 g3 i2 j1 f1 e0 f2 f4 m6 l6 j2 h2";
//		std::vector<std::string> tmp = split(game, ' ');
//
//		std::vector<Move> moves;
//		for (size_t i = 0; i < tmp.size(); i++)
//			moves.push_back(Move(static_cast<Sign>(1 + i % 2), Location(tmp[i])));
//
//		matrix<Sign> board = Board::fromListOfMoves(15, 15, moves);
//		std::cout << Board::toString(board, true);
//		return 0;
//	}

//	{
//		const std::string root_path = "/home/maciek/alphagomoku/final_runs_2025/caro6/";
//		std::vector<std::string> paths;
//		for (int i = 390; i <= 400; i++)
//			paths.push_back(root_path + "checkpoint/network_" + std::to_string(i) + ".bin");
//		NetworkLoader loader(paths);
//		auto network = loader.get();
//		network->saveToFile(root_path + "network_swa.bin");
//		return 0;
//	}

//	{
//		const std::vector<std::vector<Move>> openings = generate_openings(1000, GameConfig(GameRules::FREESTYLE, 20),
//				"/home/maciek/alphagomoku/final_runs_2025/freestyle_20x20/network_swa.bin");
//
//		const std::string path = "/home/maciek/Desktop/tournament/freestyle.txt";
//		std::ofstream fs(path);
//		for (size_t i = 0; i < openings.size(); i++)
//		{
//			std::string tmp;
//			for (size_t j = 0; j < openings[i].size(); j++)
//				tmp += std::to_string((int) openings[i][j].row) + ',' + std::to_string((int) openings[i][j].col) + ' ';
//			fs << tmp + '\n';
//		}
//		fs.close();
//		return 0;
//	}

//	Dataset dataset;
//	for (int i = 2; i < 3; i++)
//	{
//		std::cout << "loading buffer " << i << "...\n";
//		dataset.load(i, "/home/maciek/alphagomoku/new_runs_2025/standard_dataset/buffers/buffer_" + std::to_string(i) + ".bin");
//	}
//	std::cout << dataset.getStats().toString() << '\n';
//	SearchDataPack pack(15, 15);
//	dataset.getBuffer(2).getGameData(0).getSample(pack, 10);
//	pack.print();

//	test_search();
//	test_proven_positions(1000);
//	test_proven_search(100, 1000, true);
//	train_nnue();
//	prepare_selection_dataset(1000);
//	return 0;

//	generate_games();
//	run_training();
//	test_evaluate();

//	evaluate_vs_old("/home/maciek/alphagomoku/runs_2026/standard_v2/", 400);

	run_knowledge_distillation();
	return 0;

//	evaluate_vs_old("/home/maciek/alphagomoku/final_runs_2025/caro6/", 400);
//	return 0;
//
//	{
//		const std::string path = "/home/maciek/alphagomoku/new_runs_2025/size_tests/convnext_ls01_se2_8x128/";
//		std::unique_ptr<AGNetwork> swa_network = loadAGNetwork(path + "/network_140_opt.bin");
//		int count = 0;
//		for (int i = 140; i <= 150; i++)
//		{
//			std::unique_ptr<AGNetwork> network = loadAGNetwork(path + "/network_" + std::to_string(i) + "_opt.bin");
//			network->setBatchSize(1);
//			network->moveTo(ml::Device::cpu());
//			count++;
//			const float alpha = 1.0f / count;
//			ml::averageModelWeights(alpha, network->get_graph(), 1.0f - alpha, swa_network->get_graph());
//		}
//		swa_network->saveToFile(path + "/network_swa_opt.bin");
//		return 0;
////		swa_network->optimize();
////		swa_network->saveToFile(path + "/network_swa_opt_v2.bin");
//
//		const int batch_size = 1024;
//		swa_network->setBatchSize(batch_size);
//		swa_network->moveTo(ml::Device::cuda(0));
//
//		Dataset dataset;
//		for (int i = 240; i < 250; i++)
//			dataset.load(i, "/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/train_buffer_v200/buffer_" + std::to_string(i) + ".bin");
//		std::cout << dataset.getStats().toString() << '\n';
//
//		TrainingDataPack pack(15, 15);
//		SamplerVisits sampler;
//
//		sampler.init(dataset, batch_size);
//		for (int i = 0; i < 100; i++)
//		{
//			std::cout << i << '\n';
//			for (int j = 0; j < batch_size; j++)
//			{
//				sampler.get(pack);
//				swa_network->packInputData(j, pack.board, pack.sign_to_move);
//			}
//			swa_network->forward(batch_size);
//			ml::updateBatchNormStats(swa_network->get_graph());
//		}
//		swa_network->optimize();
//		swa_network->saveToFile(path + "/network_swa_opt_v3.bin");
//	}
//	return 0;
////
//	convert_dataset("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/valid_buffer/buffer_149.bin",
//			"/home/maciek/alphagomoku/new_runs/new_buffer.bin");
//	for (int i = 0; i < 500; i++)
//	{
//		std::cout << "buffer " << i << '\n';
//		const std::string path = "/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/";
//		convert_dataset(path + "train_buffer/buffer_" + std::to_string(i) + ".bin", path + "train_buffer_v200/buffer_" + std::to_string(i) + ".bin");
//		convert_dataset(path + "valid_buffer/buffer_" + std::to_string(i) + ".bin", path + "valid_buffer_v200/buffer_" + std::to_string(i) + ".bin");
//	}
//	return 0;
//
//	if (true)
//
	{
// @formatter:off
		matrix<Sign> board = Board::fromString(
				" X O X X O X O O O X O X O X X\n"
				" O O X O O O X X O X O X X O O\n"
				" O X O X X X X O X O O X O X X\n"
				" O X X X O X O O X X X O O O X\n"
				" X X O X O X O O X O X X O O X\n"
				" O X O O O X X O O X X O X X O\n"
				" X O X O X O X X O X O O X O O\n"
				" X O X O O X O _ O X X X O X O\n"
				" O X X X _ O X X X O O O X X X\n"
				" O O O O X X X O O X X X O X O\n"
				" X O X X O O X O O X O X O O O\n"
				" X X X O O O X X O O O O X O X\n"
				" O X O X O X O O O X X O O O X\n"
				" O X X X O O O X X O X O X X X\n"
				" X O O O X X O X O X X X O X O\n"
);
//				" _ _ _ _ _ _ X O O _ _ _ _ _ _\n"
//				" _ X O X X O O X O X X O X O _\n"
//				" _ _ O X O X O X O O X X O X _\n"
//				" X _ O X O O O X X O O X O X O\n"
//				" _ O X O X X O X X X O X O X _\n"
//				" _ X X O X O X O X O O O X O X\n"
//				" O O O X X O X O X O X X X O _\n"
//				" _ X X O X X X O O O X X O O _\n"
//				" X O X O O O X X X O X O O X _\n"
//				" O X O X O X O O O X O O X X _\n"
//				" O O O O X X O X O X X X O O _\n"
//				" O X O O X O O X O O O O X X _\n"
//				" _ X X X O O X X X O X X X O _\n"
//				" X O X X X O O X O X X X X O _\n"
//				" O O X O O X X O O O O X X _ _\n");
//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ X O _ X _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ X _ _ _ _ X _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ X _ _ O _ _ _ _ _ O _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */);
// @formatter:on
		const Sign sign_to_move = Sign::CIRCLE;

		const std::string path = "/home/maciek/alphagomoku/runs_2026/testy_bn/baseline/";
		std::unique_ptr<AGNetwork> network = NetworkLoader(path + "/network_swa_opt.bin").get();

//		network->moveTo(ml::Device::opencl());
		network->setBatchSize(12);
		network->optimize(2);
		network->moveTo(ml::Device::cpu());

		network->get_graph().print();

		network->convertToHalfFloats();
		network->forward(1);

		matrix<float> policy(board.rows(), board.cols());
		matrix<Value> action_values(board.rows(), board.cols());
		Value value;
		float moves_left = 0.0f;

//		network->unpackOutput(0, policy, action_values, value, moves_left);

//		Value pq;
//		for (int i = 0; i < policy.size(); i++)
//			pq += action_values[i] * policy[i];
//
//		std::cout << "value = " << value.toString() << '\n';
//		std::cout << "P*Q = " << pq.toString() << '\n';
//		std::cout << "moves left = " << moves_left << '\n';
//		std::cout << "Policy:\n" << Board::toString(board, policy) << '\n';
//		std::cout << "Action values:\n" << Board::toString(board, action_values) << '\n';

//		return 0;
		std::cout << "starting benchmark\n";
		const double start = getTime();
		int repeats = 0;
		for (; repeats < 1000000; repeats++)
		{
			network->forward(network->getBatchSize());
			if ((getTime() - start) > 30.0)
				break;
		}
		const double stop = getTime();
		const double time = stop - start;
		std::cout << "time = " << time << "s, repeats = " << repeats << ", n/s = " << network->getBatchSize() * repeats / time << "\n";
		return 0;
	}

	if (false)
	{
		GameConfig cfg(GameRules::STANDARD, 15, 15);
		evaluation::EvaluationGame eg(cfg, 100, 1);
//		evaluation::MinimaxPlayer player0("minimax_v0_reduced", cfg, 3, MoveGeneratorMode::REDUCED);
		evaluation::PolicyHeadPlayer player0("policy_head", cfg, "/home/maciek/Desktop/AlphaGomoku560/networks/standard_conv_8x128.bin");
//		evaluation::MinimaxPlayer player1("minimax_v0_optimal", cfg, 3, MoveGeneratorMode::OPTIMAL);
//		evaluation::AlphaBetaPlayer player0("alphabeta_v1_reduced", cfg, 7, 1000000, MoveGeneratorMode::REDUCED);
		evaluation::AlphaBetaPlayer player1("alphabeta_v1_optimal", cfg, 20, 1000000, MoveGeneratorMode::OPTIMAL);

		eg.setPlayers(player0, player1);
		eg.start();
		eg.waitForFinish();
		return 0;
	}
	if (false)
	{
		GameConfig cfg(GameRules::CARO5, 15, 15);
		evaluation::AlphaBetaPlayer player("", cfg, 20, 2000000, MoveGeneratorMode::OPTIMAL);
// @formatter:off
	matrix<Sign> board = Board::fromString(
			/*         a b c d e f g h i j k l m n o        */
			/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
			/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
			/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
			/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
			/*  4 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
			/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
			/*  6 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
			/*  7 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
			/*  8 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
			/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
			/* 10 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
			/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
			/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
			/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
			/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
			/*         a b c d e f g h i j k l m n o        */

//				/*         a b c d e f g h i j k l m n o          */
//				/*  0 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ O _ _ X O X _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ X X O O O O X O _ _ _\n" /*  3 */
//				/*  4 */ " _ _ X O O O O X X O X _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ X O O O X X X _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ X X X O X O O _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ X _ _ O O X X X _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ O O O O X O O _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ O _ X _ X O _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ X _ O _ X _ _ X _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ X O _ _ X _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ X O X _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o          */
//				/*         a b c d e f g h i j k l m n o        */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ O _ X _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ O X _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ O _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ _ O _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ X _ _ _ X _ X _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o        */
//				/*         a b c d e f g h i j k l m n o        */
//				/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
//				/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */ " _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */ " _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */ " _ _ _ O _ _ _ _ O _ _ _ _ _ _\n" /* 11 */
//				/* 12 */ " _ X _ _ X _ O _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */ " _ _ _ X _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */ " _ X X O _ O O X _ _ _ _ _ _ _\n" /* 14 */
//				/*         a b c d e f g h i j k l m n o        */
//				/*        a b c d e f g h i j k l m n o          */
//				/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */" _ _ _ _ _ X X X O _ _ _ _ _ _\n" /*  4 */
//				/*  5 */" _ _ _ _ _ X O X X _ _ _ _ _ _\n" /*  5 */
//				/*  6 */" _ _ _ _ _ X O O O O X _ _ _ _\n" /*  6 */
//				/*  7 */" _ _ _ _ _ O O _ _ _ _ _ _ _ _\n" /*  7 */
//				/*  8 */" _ _ _ _ X O _ _ O _ _ _ _ _ _\n" /*  8 */
//				/*  9 */" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */" _ _ _ _ _ _ _ X O _ _ O _ _ _\n" /* 10 */
//				/* 11 */" _ _ _ _ _ _ _ O X _ _ _ _ _ _\n" /* 11 */
//				/* 12 */" _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//				/*        a b c d e f g h i j k l m n o          */
	);
// @formatter:on

		std::vector<std::pair<int, int>> moves = { { 4, 2 }, { 3, 10 }, { 3, 2 }, { 5, 6 }, { 4, 4 }, { 5, 2 }, { 5, 5 }, { 6, 5 }, { 6, 6 },
				{ 3, 3 }, { 7, 4 }, { 8, 4 }, { 4, 6 }, { 7, 3 }, { 6, 8 }, { 9, 5 }, { 10, 6 }, { 7, 9 }, { 10, 7 }, { 7, 8 }, { 10, 5 }, { 10, 9 },
				{ 8, 9 }, { 7, 7 }, { 7, 10 }, { 9, 8 }, { 8, 7 }, { 10, 4 }, { 6, 10 }, { 8, 6 }, { 11, 3 }, { 6, 7 } };

		Sign sign_to_move = Sign::CROSS;

		for (auto iter = moves.begin(); iter < moves.end(); iter++)
		{
			Board::putMove(board, Move(iter->second, iter->first, sign_to_move));
			sign_to_move = invertSign(sign_to_move);
		}
		std::cout << Board::toString(board, true) << '\n';

//		std::cout << "START 15\n";
//		std::cout << "INFO rule 1\n";
//		std::cout << "INFO timeout_turn 5000\n";
//		std::cout << "INFO timeout_match 300000\n";
//		std::cout << "INFO max_memory 350000000\n";
//		std::cout << get_BOARD_command(board, sign_to_move);
//		return 0;

		player.setBoard(board, sign_to_move);
		return 0;
	}

//	{
//		FileLoader fl("/home/maciek/alphagomoku/new_runs/test2_test/config.json");
//		MasterLearningConfig cfg(fl.getJson());
//
//		for (int i = 0; i < 300; i++)
//		{
//			const int size = cfg.training_config.buffer_size.getValue(i);
//			std::cout << "iteration " << (i + 1) << ", size = " << size << ", from " << std::max(0, i + 1 - size) << " to " << i << '\n';
//		}
//		return 0;
//	}

//	if (false)
//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/retrain_10x128s/valid_buffer/buffer_1.bin");
//		std::cout << buffer.getStats().toString() << '\n';
//
//		std::unique_ptr<AGNetwork> old_network = loadAGNetwork("./old_6x64s.bin");
//		std::unique_ptr<AGNetwork> pvq_network = loadAGNetwork(
//				"/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/libml/minml_pvq_1024/network_99_opt.bin");
//		std::unique_ptr<AGNetwork> pv_network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_62_opt.bin");
////		std::unique_ptr<AGNetwork> pv_network = loadAGNetwork("./old_6x64s.bin");
//
//		old_network->setBatchSize(256);
//		pvq_network->setBatchSize(256);
//		pv_network->setBatchSize(256);
//		old_network->moveTo(ml::Device::cuda(0));
//		pvq_network->moveTo(ml::Device::cuda(0));
//		pv_network->moveTo(ml::Device::cuda(0));
//
//		SearchDataPack pack0(15, 15);
//		SearchDataPack pack1(15, 15);
//		SearchDataPack pack2(15, 15);
//		SearchDataPack pack3(15, 15);
//
//		int count = 0;
//		float old_value_error = 0.0f;
//		float pv_value_error = 0.0f;
//		float pvq_value_error = 0.0f;
//
//		float old_policy_error = 0.0f;
//		float pv_policy_error = 0.0f;
//		float pvq_policy_error = 0.0f;
//
//		for (int i = 0; i < buffer.size(); i++)
//		{
//			const int batch_size = buffer.getGameData(i).numberOfSamples();
//			for (int j = 0; j < batch_size; j++)
//			{
//				buffer.getGameData(i).getSample(pack0, j);
//
//				old_network->packInputData(j, pack0.board, pack0.played_move.sign);
//				pvq_network->packInputData(j, pack0.board, pack0.played_move.sign);
//				pv_network->packInputData(j, pack0.board, pack0.played_move.sign);
//			}
//			old_network->forward(batch_size);
//			pvq_network->forward(batch_size);
//			pv_network->forward(batch_size);
//
//			for (int j = 0; j < batch_size; j++)
//			{
//				buffer.getGameData(i).getSample(pack0, j);
//				buffer.getGameData(i).getSample(pack1, j);
//				buffer.getGameData(i).getSample(pack2, j);
//				buffer.getGameData(i).getSample(pack3, j);
////				std::cout << "\n\n" << j << "--------------------------------------------------------------\n\n";
////				std::cout << toString(pack1.played_move.sign) << " to move (" << pack1.played_move.text() << ")\n";
////				std::cout << "Minimax value = " << pack1.minimax_value.toString() << '\n';
//				old_network->unpackOutput(j, pack1.policy_prior, pack1.action_values, pack1.minimax_value);
////				std::cout << "Old Network\n";
////				std::cout << "Value = " << pack1.minimax_value.toString() << '\n';
////				std::cout << Board::toString(pack1.board, pack1.policy_prior, true) << '\n';
//
//				pvq_network->unpackOutput(j, pack2.policy_prior, pack2.action_values, pack2.minimax_value);
////				std::cout << "PVQ Network\n";
////				std::cout << "Value = " << pack2.minimax_value.toString() << '\n';
////				std::cout << Board::toString(pack2.board, pack2.policy_prior, true) << '\n';
//
//				pv_network->unpackOutput(j, pack3.policy_prior, pack3.action_values, pack3.minimax_value);
////				std::cout << "PV Network\n";
////				std::cout << "Value = " << pack3.minimax_value.toString() << '\n';
////				std::cout << Board::toString(pack3.board, pack3.policy_prior, true) << '\n';
//
//				old_value_error += cross_entropy(pack0.minimax_value, pack1.minimax_value);
//				pvq_value_error += cross_entropy(pack0.minimax_value, pack2.minimax_value);
//				pv_value_error += cross_entropy(pack0.minimax_value, pack3.minimax_value);
//
//				old_policy_error += cross_entropy(pack0.policy_prior.data(), pack1.policy_prior.data(), pack0.policy_prior.size());
//				pvq_policy_error += cross_entropy(pack0.policy_prior.data(), pack2.policy_prior.data(), pack0.policy_prior.size());
//				pv_policy_error += cross_entropy(pack0.policy_prior.data(), pack3.policy_prior.data(), pack0.policy_prior.size());
//				count++;
//			}
//		}
//		std::cout << "old error = " << old_value_error / count << ", " << old_policy_error / count << '\n';
//		std::cout << "pvq error = " << pvq_value_error / count << ", " << pvq_policy_error / count << '\n';
//		std::cout << "pv error  = " << pv_value_error / count << ", " << pv_policy_error / count << '\n';
//
////
////		SearchDataPack pack(15, 15);
////		for (int i = 0; i < 10; i++)
////		{
////			buffer.getGameData(10000).getSample(pack, i);
////			pack.print();
////		}
////
////		GameConfig cfg(GameRules::CARO5, 15);
////		TSSConfig tss_cfg;
////		tss_cfg.mode = 2;
////
////// @formatter:off
////		matrix<Sign> board = Board::fromString(
////		/*         a b c d e f g h i j k l m n o          */
////		/*  0 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
////		/*  1 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
////		/*  2 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
////		/*  3 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
////		/*  4 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  4 */
////		/*  5 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
////		/*  6 */ " _ _ _ X X _ X _ _ X X _ _ _ _\n" /*  6 */
////		/*  7 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  7 */
////		/*  8 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  8 */
////		/*  9 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
////		/* 10 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 10 */
////		/* 11 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
////		/* 12 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
////		/* 13 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
////		/* 14 */ " _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
/////*         a b c d e f g h i j k l m n o          */);
////// @formatter:on
////		const Sign sign_to_move = Sign::CROSS;
////
////		PatternCalculator calc(cfg);
////		calc.setBoard(board, sign_to_move);
////		calc.print();
////		calc.printAllThreats();
////
////		SearchDataPack pack(cfg.rows, cfg.cols);
////		for (int i = 0; i < 10; i++)
////		{
////			buffer.getGameData(i).getSample(pack, 0);
//////			pack.print();
////
////			ThreatSpaceSearch tss(cfg, tss_cfg);
////			SearchTask task(cfg);
////			task.set(pack.board, pack.played_move.sign);
////			tss.solve(task, TssMode::RECURSIVE, 1000);
////		}
//		return 0;
//	}

//	for (int i = 0; i < 15; i++)
//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/standard_15x15/valid_buffer/buffer_10.bin");
//
//		SearchDataPack pack(15, 15);
//		buffer.getGameData(0).getSample(pack, 18);
//		pack.print();
//
//		std::vector<double> hist(23);
//		for (int g = 0; g < buffer.size(); g++)
//			hist[static_cast<int>(buffer.getGameData(g).numberOfMoves()) / 10]++;
//
//		double sum = std::accumulate(hist.begin(), hist.end(), 0.0);
//
//		std::cout << "buffer " << i << " :";
//		for (size_t j = 0; j < hist.size(); j++)
//			std::cout << " " << hist[j] / sum;
//		std::cout << '\n';
//	}

//	run_training();
//	std::cout << "END" << std::endl;
//	return 0;

//	transfer_learning();

//	convert_old_network();
	test_search();
//	prepare_proven_dataset();
//	test_proven_positions(1000);
//	train_simple_evaluator();
//	test_evaluate();
//	test_pattern_calculator();
//	test_move_generator();
//	test_proven_search(1000, 1000, true);
//	parameter_tuning();
//	train_nnue();
	return 0;

//	nnue::TrainingNNUE nnue(GameConfig(GameRules::RENJU, 15), 1, "nnue_c5_64x16x16x1.bin");
//	nnue::NNUEWeights weights = nnue.dump();
//	SerializedObject so;
//	Json json = weights.save(so);
//	FileSaver fs("networks/caro5_nnue_64x16x16x1.bin");
//	fs.save(json, so, 2, false);
//	return 0;
//
//	{
//		matrix<Sign> board = Board::fromString(""
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
//				" _ _ _ O _ O _ O _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"
//				" _ _ _ _ _ O X X X X _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
//		const Sign sign_to_move = Sign::CIRCLE;
//		nnue.packInputData(0, board, sign_to_move);
//		nnue.forward(1);
//		const float eval = nnue.unpackOutput(0);
//		std::cout << "eval = " << eval << '\n';
////		return 0;
//
//		nnue::InferenceNNUE inf_nnue(GameConfig(GameRules::STANDARD, 15), weights);
//		PatternCalculator calc(GameConfig(GameRules::STANDARD, 15));
//		calc.setBoard(board, sign_to_move);
//		inf_nnue.refresh(calc);
//
////		calc.addMove(Move(Sign::CIRCLE, 8, 10));
////		inf_nnue.update(calc);
//		std::cout << "eval = " << inf_nnue.forward() << '\n';
//
////		Board::putMove(board, Move(Sign::CIRCLE, 8, 10));
////		calc.setBoard(board, invertSign(sign_to_move));
////		inf_nnue.refresh(calc);
//
//		const double start = getTime();
//		int repeats = 0;
//		for (; repeats < 100000000; repeats += 10)
//		{
//			for (int j = 0; j < 10; j++)
////				calc.setBoard(board, sign_to_move);
////				inf_nnue.refresh(calc);
//				inf_nnue.forward();
////				inf_nnue.update(calc);
//			if ((getTime() - start) > 10.0)
//				break;
//		}
//		const double stop = getTime();
//		const double time = stop - start;
//		std::cout << "time = " << time << "s, repeats = " << repeats << '\n';
//		std::cout << "n/s = " << repeats / time << " (" << time / repeats * 1.0e6 << "ms)\n";
//		std::cout << "eval = " << inf_nnue.forward() << '\n';
////		inf_nnue.forward();
//	}

//	test_proven_search(1000, 200, true);
//	for (int i = 0; i < 120; i++)
//	{
//		const std::string path = "freestyle_20x20/train_buffer/buffer_" + std::to_string(i) + ".bin";
//		convert_old_buffer("/home/maciek/alphagomoku/" + path, "/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/" + path);
//	}
//	train_simple_evaluator();
//	return 0;

//	TrainingManager tm("/home/maciek/alphagomoku/new_runs/test9_test/");
//	for (int i = 0; i < 1; i++)
//		tm.runIterationRL();

//	TrainingManager tm("/home/mkozarzewski/alphagomoku/new_runs/sl_res_pv_8x64s/", "/home/mkozarzewski/alphagomoku/new_runs/btl_pv_8x128s/");
//	for (int i = 0; i < 200; i++)
//		tm.runIterationSL();
//	return 0;

//	TrainingManager tm("/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/history1/", "/home/maciek/alphagomoku/new_runs_2023/old_runs_2021/standard_15x15/");

//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/test_test/train_buffer/buffer_0.bin");
//		SearchDataPack pack(15, 15);
//		SamplerVisits sampler;
//		sampler.init(buffer, 1);
//		TrainingDataPack target(15, 15);
//		sampler.get(target);
////		for (int i = 0; i < 10;i++)//buffer.getGameData(1000).numberOfSamples(); i++)
////		{
////			buffer.getGameData(1000).getSample(pack, i);
////			pack.print();
////		}
//		return 0;
//	}

//	test_expand();

//	return 0;

//	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/valid_buffer/buffer_200.bin");
//		std::cout << buffer.getStats().toString() << '\n';
//
//		std::unique_ptr<AGNetwork> network_fp32, network_fp16;
//		network_fp32 = loadAGNetwork("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_200_opt.bin");
//		network_fp16 = loadAGNetwork("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_200_opt.bin");
//
//		network_fp32->setBatchSize(1);
//		network_fp32->moveTo(ml::Device::cuda(0));
//
//		network_fp16->setBatchSize(1);
//		network_fp16->moveTo(ml::Device::cpu());
//		network_fp16->convertToHalfFloats();
//
//		SearchDataPack pack(15, 15);
//
//		matrix<Value> action_values;
//		matrix<float> policy_fp32(15, 15), policy_fp16(15, 15);
//		Value value_fp32, value_fp16;
//
//		float ce_policy = 0.0f, ce_value = 0.0f;
//		int count = 0;
//		for (int i = 0; i < buffer.numberOfGames() / 10; i++)
//		{
//			std::cout << i << "/" << buffer.numberOfGames() / 10 << '\n';
//			for (int j = 0; j < buffer.getGameData(i).numberOfSamples(); j++)
//			{
//				buffer.getGameData(i).getSample(pack, j);
//				network_fp32->packInputData(0, pack.board, pack.played_move.sign);
//				network_fp16->packInputData(0, pack.board, pack.played_move.sign);
//
//				network_fp32->forward(1);
//				network_fp16->forward(1);
//
//				network_fp32->unpackOutput(0, policy_fp32, action_values, value_fp32);
//				network_fp16->unpackOutput(0, policy_fp16, action_values, value_fp16);
//
////				std::cout << value_fp32.toString() << "\n" << Board::toString(pack.board, policy_fp32, true) << '\n';
////				std::cout << value_fp16.toString() << "\n" << Board::toString(pack.board, policy_fp16, true) << '\n';
//
//				ce_policy += cross_entropy(policy_fp32.data(), policy_fp16.data(), policy_fp32.size());
//				ce_value += cross_entropy(value_fp32, value_fp16);
//				count++;
//			}
//			std::cout << "diff policy = " << ce_policy / count << '\n';
//			std::cout << "diff value  = " << ce_value / count << '\n';
//		}
//
//		return 0;
//	}

	{
//		GameDataBuffer buffer("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/train_buffer/buffer_34.bin");
//		std::cout << buffer.getStats().toString() << '\n';

		GameConfig game_config(GameRules::STANDARD, 15, 15);
		TrainingConfig cfg;
		cfg.blocks = 8;
		cfg.filters = 128;
//		std::unique_ptr<AGNetwork> network;
		std::unique_ptr<AGNetwork> network = std::make_unique<BottleneckPoolingPVraw>();
//		network = loadAGNetwork("./old_6x64s.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_90_opt.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/test_sampler_v2/checkpoint/network_94_opt.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/btl_pv_8x128s/checkpoint/network_255_opt.bin");
//		network = loadAGNetwork("/home/maciek/alphagomoku/new_runs/supervised/broadcast_skip2_relu_8x128s/network_99_opt.bin");

		network->init(game_config, cfg);
		network->optimize();
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_60_opt.bin");
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs/test_6x64/checkpoint/network_60_opt.bin");
//		network.optimize();
//		network.loadFromFile("/home/maciek/alphagomoku/new_runs_2023/test_6x64s_2/checkpoint/network_33_opt.bin");
//		network->optimize();
		network->setBatchSize(12);
		network->moveTo(ml::Device::cpu());
//		network->moveTo(ml::Device::cuda(0));
		network->convertToHalfFloats();

		matrix<Sign> board(15, 15);
//		board = Board::fromString(""
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" X _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ X _ O X _ _ _ _ _ _ _ _ _ _\n"
//				" _ O X X _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ X O O X _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ X X _ _ _ _ _ _ _ _ _ _ _\n"
//				" O _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");

//		board = Board::fromString(""
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ O _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");

//		board = Board::fromString(" O _ O X X O O X O _ _ X _ _ _\n"
//				" _ X O X O X O X X X O O _ _ _\n"
//				" X O X O X O X O X O X O X _ _\n"
//				" O O X O X O X O X O X O X _ _\n"
//				" O X O X X O O X X O O O X _ _\n"
//				" X X X X O X X X O X X X O _ _\n"
//				" O O X O X O O X X O O X O O _\n"
//				" O O X O X O X O O X O O X X _\n"
//				" O X O X O X O X X O X O X O _\n"
//				" O X O X O X O X O O X O X X _\n"
//				" X O X O X O O X X X O X O O _\n"
//				" X O X O X O X O X X O O X X _\n"
//				" O _ X O O X X O X O X X X O _\n"
//				" _ _ O X O O X X O O X O O _ _\n"
//				" _ X _ _ X X O X O _ O O X _ _\n");
//		network.packInputData(0, board, Sign::CROSS);

//		network.packInputData(0, pack.board, pack.played_move.sign);

		board = Board::fromString(""
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n"
				" _ _ _ O _ O _ O _ _ _ _ _ _ _\n"
				" _ _ _ _ _ X O _ X _ _ _ _ _ _\n"
				" _ _ _ _ _ O X X X X _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ X _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
				" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");

//		board = Board::fromString(""
//		/*        a b c d e f g h i j k l m n o          */
//		/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
//				/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
//				/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
//				/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
//				/*  4 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  4 */
//				/*  5 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  5 */
//				/*  6 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  6 */
//				/*  7 */" _ _ _ _ X _ O O O _ X _ _ _ _\n" /*  7 */
//				/*  8 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /*  8 */
//				/*  9 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  9 */
//				/* 10 */" _ _ _ _ _ X _ _ _ _ _ _ _ _ _\n" /* 10 */
//				/* 11 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 11 */
//				/* 12 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 12 */
//				/* 13 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 13 */
//				/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
//		/*        a b c d e f g h i j k l m n o          */);

		const Sign sign_to_move = Sign::CIRCLE;
//		PatternCalculator calc(game_config);
//		calc.setBoard(board, sign_to_move);
//		calc.printAllThreats();
//		calc.printForbiddenMoves();
//		network.packInputData(0, board, sign_to_move);

		SearchDataPack pack(15, 15);
		const int game_index = 0;
		const int sample_index = 0;

		matrix<float> policy(15, 15);
		matrix<Value> action_values(15, 15);
		Value value;

//		network.packInputData(0, pack.board, pack.played_move.sign);
		for (int i = 0; i < network->getBatchSize(); i++)
			network->packInputData(i, board, sign_to_move);
		network->forward(1);
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//		network->forward(2);
//		return 0;

		const double start = getTime();
		int repeats = 0;
		for (; repeats < 10000; repeats++)
		{
			network->forward(network->getBatchSize());
			if ((getTime() - start) > 20.0)
				break;
		}
		const double stop = getTime();
		const double time = stop - start;
		std::cout << "time = " << time << "s, repeats = " << repeats << '\n';
		std::cout << "n/s = " << network->getBatchSize() * repeats / time << '\n';
//		return 0;
//		network->unpackOutput(0, policy, action_values, value);
		std::cout << "\n\n--------------------------------------------------------------\n\n";
		std::cout << "Value = " << value.toString() << '\n'; //<< " (" << value_target.toString() << ")\n";
		std::cout << Board::toString(board, policy, true) << '\n';
//		std::cout << Board::toString(board, action_values, true) << '\n';

		return 0;
	}

//	run_training();
//	test_evaluate();
//	generate_openings(32);
//	{
//		GameConfig gc(GameRules::STANDARD, 15);
//		TrainingConfig tc;
//		tc.device_config.batch_size = 2;
//		AGNetwork model(gc, tc);
//		model.optimize();
//		matrix<Sign> asdf(15, 15);
//		model.packInputData(0, asdf, Sign::CROSS);
//		model.forward(2);
//		model.loadFromFile("/home/maciek/alphagomoku/minml_test/minml_btl_10x128_opt.bin");
//		model.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3v7_10x128_opt.bin");
//		model.setBatchSize(256);
//		model.moveTo(ml::Device::cpu());
//		ml::Device::setNumberOfThreads(1);
//		model.moveTo(ml::Device::cuda(0));
//		model.convertToHalfFloats();
//		model.forward(1);
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//		const int repeats = 400;
//		const double start = getTime();
//		for (int i = 0; i < repeats; i++)
//			model.forward(8);
//		const double stop = getTime();
//		std::cout << 8 * repeats / (stop - start) << "n/s\n";
//	}

	std::cout << "END" << std::endl;
	return 0;

	GameBuffer buffer("/home/maciek/alphagomoku/standard_test/valid_buffer/buffer_0.bin");
//	GameBuffer buffer("/home/maciek/alphagomoku/standard_test_2/valid_buffer/buffer_19.bin");
	std::cout << buffer.getStats().toString() << '\n';

//	AGNetwork network(GameConfig(GameRules::STANDARD, 15, 15));
////	network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/checkpoint/network_20_opt.bin");
//	network.loadFromFile("/home/maciek/alphagomoku/minml_test/minml3_10x128.bin");
//	network.optimize();
////	network.convertToHalfFloats();
//
////	network.loadFromFile("/home/maciek/alphagomoku/standard_test_2/network_int8.bin");
//////	return 0;
//////	network.optimize();
//	network.setBatchSize(10);
//	network.moveTo(ml::Device::cuda(0));
////
//	matrix<Sign> board(15, 15);
//	board = Board::fromString(""
//			" _ _ _ O _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ X X O _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ O O _ X _ X _ _ _ _ _\n"
//			" _ _ O X O O O O X _ _ _ _ _ _\n"
//			" _ _ _ X _ O _ O X X _ _ _ _ _\n"
//			" _ _ _ _ X X _ _ _ X _ _ _ _ _\n"
//			" _ _ _ _ _ O _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n"
//			" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n");
////	board.at(7, 7) = Sign::CIRCLE;
//	const Sign sign_to_move = Sign::CIRCLE;

//	for (int i = 0; i < 1; i++)
//	{
//		const SearchData &sample = buffer.getFromBuffer(i).getSample(0);
////		sample.print();
////		return 0;
////		const Sign sign_to_move = sample.getMove().sign;
////		sample.getBoard(board);
//
//		network.packInputData(i, board, sign_to_move);
//	}
//
//	network.forward(1);
//
//	matrix<float> policy(15);
//	matrix<Value> action_values(15);
//	Value value;

//	for (int i = 0; i < 1; i++)
//	{
//		const Value value_target = convertOutcome(buffer.getFromBuffer(i).getSample(0).getOutcome(),
//				buffer.getFromBuffer(i).getSample(0).getMove().sign);
////		const Value minimax = buffer.getFromBuffer(i).getSample(0).getMinimaxValue();
////		buffer.getFromBuffer(i).getSample(0).getBoard(board);
//		network.unpackOutput(i, policy, action_values, value);
//		std::cout << '\n';
//		std::cout << Board::toString(board);
//		std::cout << "Network value   = " << value.toString() << " (target = " << value_target.toString() << ")\n";
////		std::cout << "minimax = " << minimax.toString() << "\n";
////		std::cout << "Proven value = " << toString(buffer.getFromBuffer(i).getSample(0).getProvenValue()) << "\n";
//		std::cout << "Network policy:\n" << Board::toString(board, policy) << '\n';
//		std::cout << "Network action values:\n" << Board::toString(board, action_values) << '\n';
////		buffer.getFromBuffer(i).getSample(0).getPolicy(policy);
////		std::cout << "Target:\n" << Board::toString(board, policy) << '\n';
////		std::cout << buffer.getFromBuffer(i).getSample(0).getMove().text() << '\n';
//	}

//	network.collectCalibrationStats();

//	GameBuffer buffer("/home/maciek/alphagomoku/standard_test_2/valid_buffer/buffer_0.bin");
//	std::cout << buffer.getStats().toString() << '\n';
//
//	for (int i = 0; i < buffer.size(); i++)
//		for (int j = 0; j < buffer.getFromBuffer(i).getNumberOfSamples(); j++)
//			buffer.getFromBuffer(i).getSample(j).print();

	std::cout << "END" << std::endl;
	return 0;

//	TrainingManager tm("/home/maciek/alphagomoku/small_20x20f/", "/home/maciek/alphagomoku/run2022_20x20f/");
//	for (int i = 0; i < 22; i++)
//		tm.runIterationSL();
//	return 0;

//	test_evaluate();
//	return 0;
//	GameConfig game_config(GameRules::STANDARD, 15, 15);
//	TrainingConfig training_config;
//	training_config.blocks = 10;
//	training_config.filters = 128;
//	training_config.augment_training_data = true;
//	training_config.device_config.device = ml::Device::cuda(1);
//	training_config.device_config.batch_size = 256;
//
//	GameBuffer buffer;
//	for (int i = 105; i < 120; i++)
//	{
//		std::cout << "Loading buffer " << i << '\n';
//		buffer.load("/home/maciek/alphagomoku/standard_15x15/train_buffer/buffer_" + std::to_string(i) + ".bin");
//	}
//	std::cout << buffer.getStats().toString() << '\n';
//
//	AGNetwork model(game_config, training_config);
//
//	SupervisedLearning sl(training_config);
//	std::string working_dir = "/home/maciek/alphagomoku/pretrain_15x15s/";
//
//	sl.saveTrainingHistory(working_dir);
//	model.saveToFile(working_dir + "model_0.bin");
//	for (int i = 0; i < 20; i++)
//	{
//		sl.train(model, buffer, 1000);
//		sl.saveTrainingHistory(working_dir);
//		sl.clearStats();
//		model.saveToFile(working_dir + "new_model_" + std::to_string(i + 1) + ".bin");
//	}
//
//	return 0;

//	std::cout << "Compiled on " << __DATE__ << " at " << __TIME__ << std::endl;
//	std::cout << ml::Device::hardwareInfo() << '\n';

//	FeatureTable ft(GameRules::FREESTYLE);
//	return 0;

//	check_all_dataset("/home/maciek/alphagomoku/test7_12x12_standard/train_buffer/", 35);
//	return 0;

//	test_expand();
//	return 0;

//	test_evaluate();
//	generate_openings(500);

//	benchmark_features();
//	find_proven_positions("/home/maciek/alphagomoku/standard_15x15/train_buffer/", 100);
//	return 0;

	return 0;
}
