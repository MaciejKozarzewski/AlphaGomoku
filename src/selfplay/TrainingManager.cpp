/*
 * TrainingManager.cpp
 *
 *  Created on: Mar 27, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/selfplay/EvaluationGame.hpp>
#include <alphagomoku/selfplay/SearchData.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <string>

namespace
{
	using namespace ag;

	std::string get_name(int id)
	{
		if (id < 10)
			return "AG_00" + std::to_string(id);
		if (id < 100)
			return "AG_0" + std::to_string(id);
		return "AG_" + std::to_string(id);
	}
	MasterLearningConfig load_config(std::string path)
	{
		if(not pathExists(path))
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
			path_to_data(pathToData.empty() ? working_dir : pathToData),
			generator_manager(config.game_config, config.generation_config),
			evaluator_manager(config.game_config, config.evaluation_config.selfplay_options),
			supervised_learning_manager(config.training_config)
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
			supervised_learning_manager.saveTrainingHistory(working_dir);
		}
	}

	void TrainingManager::runIterationRL()
	{
		generateGames();
		runIterationSL();
	}
	void TrainingManager::runIterationSL()
	{
		train();
		validate();
		supervised_learning_manager.saveTrainingHistory(working_dir);
		metadata["last_checkpoint"] = get_last_checkpoint() + 1;
		saveMetadata();

		if (config.evaluation_config.use_evaluation)
		{
			if (config.evaluation_config.in_parallel)
			{
				if (evaluation_future.valid())
				{
					std::cout << "Waiting for previous evaluation to finish..." << std::endl;
					evaluation_future.wait();
				}
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
		AGNetwork model(config.game_config, config.training_config);
		model.saveToFile(working_dir + "/checkpoint/network_0.bin");
		std::cout << "Saved training model\n";
		model.optimize();
		std::cout << "Optimized model\n";
		model.saveToFile(working_dir + "/checkpoint/network_0_opt.bin");
		std::cout << "Saved optimized model\n";
	}
	void TrainingManager::generateGames()
	{
		if (pathExists(working_dir + "/train_buffer/buffer_" + std::to_string(get_last_checkpoint()) + ".bin"))
		{
			std::cout << "Buffer " + std::to_string(get_last_checkpoint()) + " already exists\n";
			return;
		}

		std::string path_to_last_network = working_dir + "/checkpoint/network_" + std::to_string(get_last_checkpoint()) + "_opt.bin";
		std::cout << "Using " << path_to_last_network << '\n';
		const int training_games = config.generation_config.games_per_iteration;
		const int validation_games = std::max(1.0, training_games * config.training_config.validation_percent);
		std::cout << "Generating " << (training_games + validation_games) << " games\n";

		generator_manager.setWorkingDirectory(working_dir);
		generator_manager.getGameBuffer().clear();
		generator_manager.generate(path_to_last_network, training_games + validation_games);
		if (generator_manager.getGameBuffer().isCorrect() == false)
			throw std::runtime_error("generated buffer is invalid");
		std::cout << "Finished generating games\n";

		splitBuffer(generator_manager.getGameBuffer(), training_games, validation_games);
	}

	void TrainingManager::train()
	{
		const int epoch = get_last_checkpoint();

		GameBuffer buffer;
		loadBuffer(buffer, path_to_data + "/train_buffer/");
		std::cout << buffer.getStats().toString() << '\n';

		AGNetwork model(config.game_config, working_dir + "/checkpoint/network_" + std::to_string(epoch) + ".bin");
		model.setBatchSize(config.training_config.device_config.batch_size);
		model.moveTo(config.training_config.device_config.device);
		const double learning_rate = config.training_config.learning_rate.getValue(epoch);
		std::cout << "Using learning rate " << learning_rate << '\n';
		model.changeLearningRate(learning_rate);

		supervised_learning_manager.loadProgress(metadata);
		supervised_learning_manager.train(model, buffer, config.training_config.steps_per_iteration);
		metadata["learning_steps"] = supervised_learning_manager.saveProgress()["learning_steps"];

		model.saveToFile(working_dir + "/checkpoint/network_" + std::to_string(epoch + 1) + ".bin");
		model.moveTo(ml::Device::cpu());
		model.optimize();
		model.saveToFile(working_dir + "/checkpoint/network_" + std::to_string(epoch + 1) + "_opt.bin");
		std::cout << "Training finished\n";
	}
	void TrainingManager::validate()
	{
		const int epoch = get_last_checkpoint();

		GameBuffer buffer;
		loadBuffer(buffer, path_to_data + "/valid_buffer/");

		AGNetwork model(config.game_config, working_dir + "/checkpoint/network_" + std::to_string(epoch + 1) + ".bin");
		model.setBatchSize(config.training_config.device_config.batch_size);
		model.moveTo(config.training_config.device_config.device);

		supervised_learning_manager.validate(model, buffer);
		std::cout << "Validation finished\n";
	}
	void TrainingManager::evaluate()
	{
		const int epoch = get_last_checkpoint();

		std::string path_to_networks = working_dir + "/checkpoint/network_";

		std::string first_network = path_to_networks + std::to_string(epoch) + "_opt.bin";
		std::string first_name = get_name(epoch);
		evaluator_manager.setFirstPlayer(config.evaluation_config.selfplay_options, first_network, first_name);

		std::cout << "Evaluating network " << epoch << " against";
		for (int i = 0; i < evaluator_manager.numberOfThreads(); i++)
		{
			const int index = std::max(0, epoch - 1 - i);
			std::string second_network = path_to_networks + std::to_string(index) + "_opt.bin";
			std::string second_name = get_name(index);

			evaluator_manager.setSecondPlayer(i, config.evaluation_config.selfplay_options, second_network, second_name);
			std::cout << ((i == 0) ? " " : ", ") << std::to_string(index);
		}
		std::cout << '\n';

		evaluator_manager.generate(config.evaluation_config.selfplay_options.games_per_iteration);
		std::string to_save;
		for (int i = 0; i < evaluator_manager.numberOfThreads(); i++)
			to_save += evaluator_manager.getGameBuffer(i).generatePGN();
		std::cout << "Evaluation finished\n";

		std::lock_guard<std::mutex> lock(rating_mutex);
		std::ofstream file(working_dir + "/rating.pgn", std::fstream::app);
		file << to_save;
		file.close();
	}
	void TrainingManager::splitBuffer(GameBuffer &buffer, int training_games, int validation_games)
	{
		std::cout << "Saving validation buffer\n";
		GameBuffer tmp;
		for (int i = 0; i < validation_games; i++)
			tmp.addToBuffer(buffer.getFromBuffer(training_games + i));
		tmp.save(working_dir + "/valid_buffer/buffer_" + std::to_string(get_last_checkpoint()) + ".bin");
		tmp.clear();

		std::cout << "Saving training buffer\n";
		buffer.removeRange(training_games, training_games + validation_games);
		buffer.save(working_dir + "/train_buffer/buffer_" + std::to_string(get_last_checkpoint()) + ".bin");
	}
	void TrainingManager::loadBuffer(GameBuffer &result, const std::string &path)
	{
		int last_checkpoint = metadata["last_checkpoint"];
		int buffer_size = config.training_config.buffer_size.getValue(last_checkpoint);

		std::cout << "Loading buffers from " << std::max(0, last_checkpoint + 1 - buffer_size) << " to " << last_checkpoint << '\n';
		for (int i = std::max(0, last_checkpoint + 1 - buffer_size); i <= last_checkpoint; i++)
			result.load(path + "buffer_" + std::to_string(i) + ".bin");
		if (result.isCorrect() == false)
			throw std::runtime_error("loaded buffer is invalid");
	}

	int TrainingManager::get_last_checkpoint() const
	{
		return static_cast<int>(metadata["last_checkpoint"]);
	}

} /* namespace ag */

