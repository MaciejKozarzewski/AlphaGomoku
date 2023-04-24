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
		generateGames();
		if (hasCapturedSignal(SignalType::INT))
			exit(0);
		runIterationSL();
		if (hasCapturedSignal(SignalType::INT))
			exit(0);
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
	}
	/*
	 * private
	 */
	void TrainingManager::initFolderTree()
	{
		createDirectory(working_dir + "/checkpoint/"); // create folder for networks
		createDirectory(working_dir + "/train_buffer/"); // create folder for buffers
		createDirectory(working_dir + "/valid_buffer/"); // create folder for buffers
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
		model->optimize();
		std::cout << "Optimized model\n";
		model->saveToFile(working_dir + "/checkpoint/network_0_opt.bin");
		std::cout << "Saved optimized model\n";
	}
	void TrainingManager::generateGames()
	{
		const int epoch = get_last_checkpoint();
		if (pathExists(working_dir + "/train_buffer/buffer_" + std::to_string(epoch) + ".bin"))
		{
			std::cout << "Buffer " + std::to_string(epoch) + " already exists\n";
			return;
		}

		std::string path_to_last_network = working_dir + "/checkpoint/network_" + std::to_string(epoch) + "_opt.bin";
		std::cout << "Using " << path_to_last_network << '\n';
		const int training_games = config.generation_config.games_per_iteration;
		const int validation_games = std::max(1.0, training_games * config.training_config.validation_percent);
		std::cout << "Generating " << (training_games + validation_games) << " games\n";

		GeneratorManager generator_manager(config.game_config, config.generation_config);
		generator_manager.setWorkingDirectory(working_dir);
		generator_manager.generate(path_to_last_network, training_games + validation_games);
		if (hasCapturedSignal(SignalType::INT))
			return;
		std::cout << "Finished generating games\n";

		save_buffer_stats(generator_manager.getGameBuffer());
		splitBuffer(generator_manager.getGameBuffer(), training_games, validation_games);
	}
	void TrainingManager::train_and_validate()
	{
		const int epoch = get_last_checkpoint();

		SupervisedLearning sl_manager(config.training_config);
		sl_manager.loadProgress(metadata);

		GameDataBuffer train_buffer;
		loadBuffer(train_buffer, path_to_data + "/train_buffer/");
		std::cout << train_buffer.getStats().toString() << '\n';

		std::unique_ptr<AGNetwork> model = loadAGNetwork(working_dir + "/checkpoint/network_" + std::to_string(epoch) + ".bin");

		model->setBatchSize(config.training_config.device_config.batch_size);
		model->moveTo(config.training_config.device_config.device);

		const double learning_rate = config.training_config.learning_rate.getValue(epoch);
		std::cout << "Using learning rate " << learning_rate << '\n';
		model->changeLearningRate(learning_rate);

		const double start = getTime();
		sl_manager.train(*model, train_buffer, config.training_config.steps_per_iteration);

		// save model
		model->saveToFile(working_dir + "/checkpoint/network_" + std::to_string(epoch + 1) + ".bin");
		std::cout << "Training finished in " << (getTime() - start) << "s\n";

		// run validation
		GameDataBuffer validation_buffer;
		loadBuffer(validation_buffer, path_to_data + "/valid_buffer/");
		sl_manager.validate(*model, validation_buffer);
		std::cout << "Validation finished\n";

		// save metadata and training history
		sl_manager.saveTrainingHistory(working_dir);
		metadata["learning_steps"] = sl_manager.saveProgress()["learning_steps"];
		metadata["last_checkpoint"] = get_last_checkpoint() + 1;

		// optimize model
		model->moveTo(ml::Device::cpu());
		model->optimize();
		model->saveToFile(working_dir + "/checkpoint/network_" + std::to_string(epoch + 1) + "_opt.bin");
		std::cout << "Optimized model\n";
	}
	void TrainingManager::evaluate()
	{
		const int epoch = get_last_checkpoint();

		const std::string path_to_networks = working_dir + "/checkpoint/network_";

		const std::string first_network = path_to_networks + std::to_string(epoch) + "_opt.bin";
		const std::string first_name = "AG_" + zfill(epoch, 3);

		EvaluationManager evaluator_manager(config.game_config, config.evaluation_config.selfplay_options);
		evaluator_manager.setFirstPlayer(config.evaluation_config.selfplay_options, first_network, first_name);

		std::cout << "Evaluating network " << epoch << " against";
		const int max_parts = std::min(evaluator_manager.numberOfThreads(), (int) config.evaluation_config.opponents.size());
		for (int i = 0; i < max_parts; i++)
		{
			const int index = std::max(0, epoch + config.evaluation_config.opponents[i]);
			const std::string second_network = path_to_networks + std::to_string(index) + "_opt.bin";
			const std::string second_name = "AG_" + zfill(index, 3);

			evaluator_manager.setSecondPlayer(i, config.evaluation_config.selfplay_options, second_network, second_name);
			std::cout << ((i == 0) ? " " : ", ") << std::to_string(index);
		}
		std::cout << '\n';

		evaluator_manager.generate(config.evaluation_config.selfplay_options.games_per_iteration);

		const std::string to_save = evaluator_manager.getPGN();
		std::cout << "Evaluation finished\n";

		std::lock_guard<std::mutex> lock(rating_mutex);
		std::ofstream file(working_dir + "/rating.pgn", std::fstream::app);
		file << to_save;
		file.close();
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
	void TrainingManager::loadBuffer(GameDataBuffer &result, const std::string &path)
	{
		const int last_checkpoint = metadata["last_checkpoint"];
		const int buffer_size = config.training_config.buffer_size.getValue(last_checkpoint);

		std::cout << "Loading buffers from " << std::max(0, last_checkpoint + 1 - buffer_size) << " to " << last_checkpoint << '\n';
		for (int i = std::max(0, last_checkpoint + 1 - buffer_size); i <= last_checkpoint; i++)
			result.load(path + "buffer_" + std::to_string(i) + ".bin");
	}

	int TrainingManager::get_last_checkpoint() const
	{
		return static_cast<int>(metadata["last_checkpoint"]);
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

