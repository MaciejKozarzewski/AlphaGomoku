/*
 * TrainingManager.cpp
 *
 *  Created on: Mar 27, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/dataset/GameDataBuffer.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/tuning/GSPRT.hpp>

#include <string>

namespace
{
	using namespace ag;

	MasterLearningConfig load_config(std::string path)
	{
		if (not pathExists(path))
			createDirectory(path);

		path += "/config.json";

		if (pathExists(path))
			return MasterLearningConfig(FileLoader(path).getJson());
		else
		{ // create a default config
			MasterLearningConfig cfg;
			std::ofstream output;
			output.open(path.data(), std::ios::app);
			output << cfg.toJson().dump(2) << '\n';
			output.close();
			std::cout << "Created default configuration file, exiting" << std::endl;
			exit(0);
			return cfg;
		}
	}
	NetworkLoader get_network_loader(int from, int to, const std::string &base_path)
	{
		assert(from >= 0);
		assert(to >= 0);
		std::cout << " (" << from << ", " << to << ") ";
		std::vector<std::string> paths;
		for (int i = from; i <= to; i++)
			paths.push_back(base_path + "network_" + std::to_string(i) + ".bin");
		return NetworkLoader(paths);
	}
	NetworkLoader get_network_loader(int idx, const Parameter<int> &param, const std::string &base_path)
	{
		return get_network_loader(std::max(0, idx - param.getValue(idx)), idx, base_path);
	}
}

namespace ag
{
	TrainingManager::TrainingManager(std::string workingDirectory, std::string pathToData) :
			config(load_config(workingDirectory)),
			metadata( { { "last_checkpoint", 0 }, { "learning_steps", 0 } }),
			working_dir(workingDirectory + '/'),
			path_to_data(pathToData.empty() ? working_dir : pathToData)
	{
		if (loadMetadata())
			std::cout << "Loading existing training run\n" << metadata.dump(2) << '\n';
		else
		{
			metadata["last_checkpoint"] = 0;
			metadata["best_checkpoint"] = 0;
			metadata["learning_steps"] = 0;
			std::cout << "Initializing new training run\n";
			saveMetadata();
			std::cout << "Saved metadata\n";
			initFolderTree();
			std::cout << "Initialized folder tree\n";
			initModel();
			std::cout << "Initialized model\n";
			SupervisedLearning sl(config.training_config);
			sl.saveTrainingHistory(working_dir);
		}
	}

	void TrainingManager::runIterationRL()
	{
		const double t0 = getTime();
		generateGames();
		if (hasCapturedSignal(SignalType::INT))
			exit(0);
		runIterationSL();
		if (hasCapturedSignal(SignalType::INT))
			exit(0);
		std::cout << "completed iteration in " << (getTime() - t0) << "s\n";
	}
	void TrainingManager::runIterationSL()
	{
		train_and_validate();
		saveMetadata();

		if (config.evaluation_config.use_evaluation)
		{
			if (config.evaluation_config.in_parallel)
			{
				if (evaluation_future.valid())
				{
					std::future_status status = evaluation_future.wait_for(std::chrono::milliseconds(0));
					if (status != std::future_status::ready)
					{
						std::cout << "Waiting for previous evaluation to finish..." << std::endl;
						evaluation_future.wait();
					}
				}
				if (hasCapturedSignal(SignalType::INT))
					return;
				evaluation_future = std::async(std::launch::async, [this]()
				{
					try
					{
						this->evaluate();
					}
					catch (std::exception &e)
					{
						std::cout << "TrainingManager::runIterationSL() threw " << e.what() << std::endl;
						exit(-1);
					}
				});
			}
			else
				evaluate();
		}

		if (config.evaluation_config.use_gating)
		{
			gating();
			saveMetadata();
		}
	}
	/*
	 * private
	 */
	void TrainingManager::initFolderTree()
	{
		createDirectory(working_dir + "/checkpoint/"); // create folder for networks
		createDirectory(working_dir + "/train_buffer/"); // create folder for buffers
		createDirectory(working_dir + "/valid_buffer/"); // create folder for buffers
		createDirectory(working_dir + "/saved_state/"); // create folder for game generators state
	}
	void TrainingManager::saveMetadata()
	{
		std::string dataPath = working_dir + "/metadata.json";
		if (pathExists(dataPath))
			removeFile(dataPath);

		std::ofstream output;
		output.open(dataPath.data(), std::ios::app);
		output << metadata.dump(2) << '\n';
	}
	bool TrainingManager::loadMetadata()
	{
		if (not pathExists(working_dir + "/metadata.json"))
			return false;
		else
		{
			metadata = FileLoader(working_dir + "/metadata.json").getJson();
			return true;
		}
	}
	void TrainingManager::initModel()
	{
		std::unique_ptr<AGNetwork> model = createAGNetwork(config.training_config.network_arch);
		model->init(config.game_config, config.training_config);
		model->saveToFile(working_dir + "/checkpoint/network_0.bin");
		std::cout << "Saved training model\n";
	}
	void TrainingManager::generateGames()
	{
		const int epoch = get_last_checkpoint();
		if (pathExists(working_dir + "/train_buffer/buffer_" + std::to_string(epoch) + ".bin"))
		{
			std::cout << "Buffer " + std::to_string(epoch) + " already exists\n";
			return;
		}

		const int best_checkpoint = get_best_checkpoint();
		const NetworkLoader network_loader = get_network_loader(best_checkpoint, config.training_config.swa_networks_num,
				working_dir + "/checkpoint/");
		const int training_games = config.generation_config.games_per_iteration;
		const int validation_games = std::max(1.0, training_games * config.training_config.validation_percent);
		std::cout << "Generating " << (training_games + validation_games) << " games\n";

		if (generator_manager == nullptr)
		{
			const double t0 = getTime();
			generator_manager = std::make_unique<GeneratorManager>(config.game_config, config.generation_config);
			generator_manager->setWorkingDirectory(working_dir);
			generator_manager->loadState();
			const double t1 = getTime();
			std::cout << "created game generator in " << (t1 - t0) << "s\n";
		}

		const double start_time = getTime();
		generator_manager->generate(network_loader, training_games + validation_games);
		std::cout << "Finished generating games in " << (getTime() - start_time) << "s\n";

		const bool was_interrupted = hasCapturedSignal(SignalType::INT);
		const double t0 = getTime();
		generator_manager->saveState(was_interrupted);
		std::cout << "saved state in " << (getTime() - t0) << "s\n";
		if (was_interrupted)
			return;

		const double t1 = getTime();
		save_buffer_stats(generator_manager->getGameBuffer());
		splitBuffer(generator_manager->getGameBuffer(), training_games, validation_games);
		std::cout << "copied buffer in " << (getTime() - t1) << "s\n";

		if (config.generation_config.keep_loaded)
			generator_manager->getGameBuffer().clear();
		else
		{
			const double t2 = getTime();
			generator_manager = nullptr;
			std::cout << "cleaned up generator in " << (getTime() - t2) << "s\n";
		}
	}
	void TrainingManager::train_and_validate()
	{
		const double start_time = getTime();

		const int epoch = get_last_checkpoint();

		SupervisedLearning sl_manager(config.training_config);
		sl_manager.loadProgress(metadata);

		loadDataset(training_dataset, path_to_data + "/train_buffer/");
		std::cout << training_dataset.getStats().toString() << '\n';

		std::unique_ptr<AGNetwork> model = loadAGNetwork(working_dir + "/checkpoint/network_" + std::to_string(epoch) + ".bin");

		model->setBatchSize(config.training_config.device_config.batch_size);
		model->moveTo(config.training_config.device_config.device);

		const double learning_rate = config.training_config.learning_rate.getValue(epoch);
		std::cout << "Using learning rate " << learning_rate << '\n';
		model->changeLearningRate(learning_rate);

		sl_manager.train(*model, training_dataset, config.training_config.steps_per_iteration);
		if (not config.training_config.keep_loaded)
			training_dataset.clear();

		// save model
		model->saveToFile(working_dir + "/checkpoint/network_" + std::to_string(epoch + 1) + ".bin");
		std::cout << "Training finished in " << (getTime() - start_time) << "s\n";

		// run validation
//		loadDataset(validation_dataset, path_to_data + "/valid_buffer/");
//		sl_manager.validate(*model, validation_dataset);
//		if (not config.training_config.keep_loaded)
//			validation_dataset.clear();
//		std::cout << "Validation finished\n";

		// save metadata and training history
		sl_manager.saveTrainingHistory(working_dir);
		metadata["learning_steps"] = sl_manager.saveProgress()["learning_steps"];
		metadata["last_checkpoint"] = epoch + 1;
		if (not config.evaluation_config.use_gating)
			metadata["best_checkpoint"] = get_last_checkpoint();

		// save most recent SWA network
		const NetworkLoader loader = get_network_loader(std::max(0, get_last_checkpoint() - 10), get_last_checkpoint(), working_dir + "/checkpoint/");
		std::cout << '\n';
		loader.get()->saveToFile(working_dir + "/network_swa.bin");
	}
	void TrainingManager::evaluate()
	{
		const double start_time = getTime();

		const int epoch = get_last_checkpoint();

		const NetworkLoader loader_1st = get_network_loader(epoch, config.training_config.swa_networks_num, working_dir + "/checkpoint/");

		const std::string first_name = "AG_" + zfill(epoch, 3);

		EvaluationManager evaluator_manager(config.game_config, config.evaluation_config.selfplay_options);
		evaluator_manager.setFirstPlayer(config.evaluation_config.selfplay_options, loader_1st, first_name);

		std::cout << "Evaluating network " << epoch << " against";
		const int max_parts = std::min(evaluator_manager.numberOfThreads(), (int) config.evaluation_config.opponents.size());
		for (int i = 0; i < max_parts; i++)
		{
			const int index = std::max(0, epoch + config.evaluation_config.opponents[i]);
			std::cout << ((i == 0) ? " " : ", ") << std::to_string(index);
			const NetworkLoader loader_2nd = get_network_loader(index, config.training_config.swa_networks_num, working_dir + "/checkpoint/");
			const std::string second_name = "AG_" + zfill(index, 3);

			evaluator_manager.setSecondPlayer(i, config.evaluation_config.selfplay_options, loader_2nd, second_name);
		}
		std::cout << '\n';

		evaluator_manager.generate(config.evaluation_config.selfplay_options.games_per_iteration);

		const std::string to_save = evaluator_manager.getPGN();
		std::cout << "Evaluation finished in " << (getTime() - start_time) << "s\n";

		std::lock_guard<std::mutex> lock(rating_mutex);
		std::ofstream file(working_dir + "/rating.pgn", std::fstream::app);
		file << to_save;
		file.close();
	}
	void TrainingManager::gating()
	{
		const double start_time = getTime();

		const int last_checkpoint = get_last_checkpoint();
		const int best_checkpoint = get_best_checkpoint();

		EvaluationManager evaluator_manager(config.game_config, config.evaluation_config.selfplay_options);

		const NetworkLoader loader_last = get_network_loader(last_checkpoint, config.training_config.swa_networks_num, working_dir + "/checkpoint/");
		const std::string first_name = "AG_" + zfill(last_checkpoint, 3);
		evaluator_manager.setFirstPlayer(config.evaluation_config.selfplay_options, loader_last, first_name);

		const NetworkLoader loader_best = get_network_loader(best_checkpoint, config.training_config.swa_networks_num, working_dir + "/checkpoint/");
		const std::string second_name = "AG_" + zfill(best_checkpoint, 3);
		evaluator_manager.setSecondPlayer(config.evaluation_config.selfplay_options, loader_best, second_name);

		std::cout << "Evaluating network " << last_checkpoint << " against " << best_checkpoint << '\n';
		evaluator_manager.generate(config.evaluation_config.selfplay_options.games_per_iteration);

		const std::string to_save = evaluator_manager.getPGN();
		std::cout << "Gating finished in " << (getTime() - start_time) << "s\n";

		std::lock_guard<std::mutex> lock(rating_mutex);
		std::ofstream file(working_dir + "/gating.pgn", std::fstream::app);
		file << to_save;
		file.close();

		std::array<int, 5> scores = { 0, 0, 0, 0, 0 };
		for (int i = 0; i < evaluator_manager.numberOfThreads(); i++)
		{
			const std::vector<int> match_results = convert_match_results(evaluator_manager.getGameBuffer(i));
			for (size_t j = 0; j < match_results.size(); j++)
				scores[match_results[j]]++;
		}

		const double num_games = 2 * (scores[0] + scores[1] + scores[2] + scores[3] + scores[4]);
		const double winrate = (0.0 * scores[0] + 0.5 * scores[1] + 1.0 * scores[2] + 1.5 * scores[3] + 2.0 * scores[4]) / num_games;
		std::cout << "winrate = " << winrate << " (" << elo_from_winrate(winrate) << " elo)\n";
		if (winrate > 0.5)
		{
			std::cout << "network " << last_checkpoint << " PASSED\n";
			metadata["best_checkpoint"] = last_checkpoint;
		}
		else
			std::cout << "network " << last_checkpoint << " REJECTED\n";
	}
	void TrainingManager::splitBuffer(GameDataBuffer &buffer, int training_games, int validation_games)
	{
		std::cout << "Saving validation buffer\n";
		GameDataBuffer tmp(config.game_config);
		for (int i = 0; i < validation_games; i++)
			tmp.addGameData(buffer.getGameData(training_games + i));
		tmp.save(working_dir + "/valid_buffer/buffer_" + std::to_string(get_last_checkpoint()) + ".bin");
		tmp.clear();

		std::cout << "Saving training buffer\n";
		buffer.removeRange(training_games, training_games + validation_games);
		buffer.save(working_dir + "/train_buffer/buffer_" + std::to_string(get_last_checkpoint()) + ".bin");
	}
	void TrainingManager::loadDataset(Dataset &result, const std::string &path)
	{
		const int last_checkpoint = metadata["last_checkpoint"];
		const int buffer_size = config.training_config.buffer_size.getValue(last_checkpoint);

		const int first_buffer = std::max(0, last_checkpoint + 1 - buffer_size);
		const int last_buffer = last_checkpoint;
		std::cout << "Loading buffers from " << first_buffer << " to " << last_buffer << '\n';

		for (int i = 0; i < first_buffer; i++)
			result.unload(i);
		for (int i = first_buffer; i <= last_buffer; i++)
			result.load(i, path + "buffer_" + std::to_string(i) + ".bin");
	}

	int TrainingManager::get_last_checkpoint() const
	{
		return metadata["last_checkpoint"].getInt();
	}
	int TrainingManager::get_best_checkpoint() const
	{
		return metadata["best_checkpoint"].getInt();
	}
	void TrainingManager::save_buffer_stats(const GameDataBuffer &buffer) const
	{
		if (working_dir.empty())
			return;
		std::string path = working_dir + "/buffer_stats.txt";
		std::ofstream history_file;
		history_file.open(path.data(), std::ios::app);
		if (get_last_checkpoint() == 0)
			history_file << "#step avg_samples avg_length cross_wins draws circle_wins\n";

		const GameDataBufferStats stats = buffer.getStats();
		history_file << get_last_checkpoint() << ' ';
		history_file << (double) stats.samples / stats.games << ' ';
		history_file << (double) stats.game_length / stats.games << ' ';
		history_file << (double) stats.cross_win / stats.games << ' ';
		history_file << (double) stats.draws / stats.games << ' ';
		history_file << (double) stats.circle_win / stats.games;
		history_file << '\n';
		history_file.close();
	}

} /* namespace ag */

