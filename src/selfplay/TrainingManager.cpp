/*
 * TrainingManager.cpp
 *
 *  Created on: Mar 27, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <alphagomoku/selfplay/TrainingManager.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <string>
#include <filesystem>

namespace
{
	std::string get_name(int id)
	{
		if (id < 10)
			return "AG_00" + std::to_string(id);
		if (id < 100)
			return "AG_0" + std::to_string(id);
		return "AG_" + std::to_string(id);
	}
}

namespace ag
{
	TrainingManager::TrainingManager(const std::string &workingDirectory) :
			config(FileLoader(workingDirectory + "/config.json").getJson()),
			metadata( { { "last_checkpoint", 0 }, { "best_checkpoint", 0 }, { "learning_steps", 0 } }),
			working_dir(workingDirectory + '/'),
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
			initFolderTree();
			initModel();
			supervised_learning_manager.saveTrainingHistory(working_dir);
		}
	}

	void TrainingManager::runIterationRL()
	{
		generateGames();

		train();
		validate();
		supervised_learning_manager.saveTrainingHistory(working_dir);

		evaluate();
		metadata["last_checkpoint"] = get_last_checkpoint() + 1;
		saveMetadata();
	}
	void TrainingManager::runIterationSL()
	{
	}
	/*
	 * private
	 */
	void TrainingManager::initFolderTree()
	{
		std::filesystem::create_directory(working_dir + "/checkpoint/"); // create folder for networks
		std::filesystem::create_directory(working_dir + "/train_buffer/"); // create folder for buffers
		std::filesystem::create_directory(working_dir + "/valid_buffer/"); // create folder for buffers
	}
	void TrainingManager::saveMetadata()
	{
		std::string dataPath = working_dir + "/metadata.json";
		if (std::filesystem::exists(dataPath))
			std::filesystem::remove(dataPath);

		std::ofstream output;
		output.open(dataPath.data(), std::ios::app);
		output << metadata.dump(2) << '\n';
	}
	bool TrainingManager::loadMetadata()
	{
		if (not std::filesystem::exists(working_dir + "/metadata.json"))
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
		model.optimize();
		model.saveToFile(working_dir + "/checkpoint/network_0_opt.bin");
	}
	void TrainingManager::generateGames()
	{
		if (std::filesystem::exists(working_dir + "/train_buffer/buffer_" + std::to_string(get_last_checkpoint()) + ".bin"))
		{
			std::cout << "buffer " + std::to_string(get_last_checkpoint()) + " already exists\n";
			return;
		}

		std::string path_to_best_network = working_dir + "/checkpoint/network_" + std::to_string(get_best_checkpoint()) + "_opt.bin";
		std::cout << "Loading " << path_to_best_network << '\n';
		int training_games = std::min(1000 * (1 + get_last_checkpoint()), config.generation_config.games_per_iteration);
		int validation_games = training_games * config.training_config.validation_percent;
		std::cout << "Generating " << (training_games + validation_games) << " games\n";

		generator_manager.getGameBuffer().clear();
		generator_manager.generate(path_to_best_network, training_games + validation_games);
		if (generator_manager.getGameBuffer().isCorrect() == false)
			throw std::runtime_error("generated buffer is invalid");
		std::cout << "Finished generating games\n";

		splitBuffer(generator_manager.getGameBuffer(), training_games, validation_games);
	}

	void TrainingManager::train()
	{
		GameBuffer buffer;
		loadBuffer(buffer, working_dir + "/train_buffer/");

		AGNetwork model;
		model.loadFromFile(working_dir + "/checkpoint/network_" + std::to_string(get_last_checkpoint()) + ".bin");
		model.setBatchSize(config.training_config.device_config.batch_size);
		model.getGraph().moveTo(config.training_config.device_config.device);
		model.changeLearningRate(get_learning_rate());

		supervised_learning_manager.loadProgress(metadata);
		supervised_learning_manager.train(model, buffer, config.training_config.steps_per_iteration);
		metadata["learning_steps"] = supervised_learning_manager.saveProgress()["learning_steps"];

		model.saveToFile(working_dir + "/checkpoint/network_" + std::to_string(get_last_checkpoint() + 1) + ".bin");
		model.getGraph().moveTo(ml::Device::cpu());
		model.optimize();
		model.saveToFile(working_dir + "/checkpoint/network_" + std::to_string(get_last_checkpoint() + 1) + "_opt.bin");
		std::cout << "Training finished\n";
	}
	void TrainingManager::validate()
	{
		GameBuffer buffer;
		loadBuffer(buffer, working_dir + "/valid_buffer/");

		AGNetwork model;
		model.loadFromFile(working_dir + "/checkpoint/network_" + std::to_string(get_last_checkpoint() + 1) + ".bin");
		model.setBatchSize(config.training_config.device_config.batch_size);
		model.getGraph().moveTo(config.training_config.device_config.device);

		supervised_learning_manager.validate(model, buffer);
		std::cout << "Validation finished\n";
	}
	void TrainingManager::evaluate()
	{
		if (config.evaluation_config.use_evaluation == false)
		{
			metadata["best_checkpoint"] = get_last_checkpoint() + 1;
			return;
		}
		std::string path_to_networks = working_dir + "/checkpoint/network_";

		std::string first_network = path_to_networks + std::to_string(get_last_checkpoint() + 1) + "_opt.bin";
		std::string first_name = get_name(get_last_checkpoint() + 1);
		evaluator_manager.setFirstPlayer(config.evaluation_config.selfplay_options, first_network, first_name);

		std::cout << "Evaluating network " << get_last_checkpoint() + 1 << " against";
		for (int i = 0; i < evaluator_manager.numberOfThreads(); i++)
		{
			std::string second_network = path_to_networks + std::to_string(std::max(0, get_best_checkpoint() - i)) + "_opt.bin";
			std::string second_name = get_name(std::max(0, get_best_checkpoint() - i));

			evaluator_manager.setSecondPlayer(i, config.evaluation_config.selfplay_options, second_network, second_name);
			std::cout << ((i == 0) ? " " : ", ") << std::to_string(std::max(0, get_best_checkpoint() - i));
		}
		std::cout << '\n';

		evaluator_manager.generate(config.evaluation_config.selfplay_options.games_per_iteration);
		std::string to_save;
		for (int i = 0; i < evaluator_manager.numberOfThreads(); i++)
			to_save += evaluator_manager.getGameBuffer(i).generatePGN();
		std::ofstream file(working_dir + "/rating.pgn", std::fstream::app);
		file << to_save;
		file.close();

		if (config.evaluation_config.use_gating)
		{
			GameBufferStats stats;
			for (int i = 0; i < evaluator_manager.numberOfThreads(); i++)
				stats += evaluator_manager.getGameBuffer(i).getStats();
			if (stats.cross_win + 0.5 * stats.draws >= config.evaluation_config.gating_threshold)
				metadata["best_checkpoint"] = get_last_checkpoint() + 1;
		}
		else
			metadata["best_checkpoint"] = get_last_checkpoint() + 1;
		std::cout << "Evaluation finished\n";
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
		int iterations = static_cast<int>(std::max(10.0, 0.999 + last_checkpoint * 0.2));

		std::cout << "Loading buffers from " << std::max(0, last_checkpoint + 1 - iterations) << " to " << last_checkpoint << '\n';
		for (int i = std::max(0, last_checkpoint + 1 - iterations); i <= last_checkpoint; i++)
			result.load(path + "buffer_" + std::to_string(i) + ".bin");
		if (result.isCorrect() == false)
			throw std::runtime_error("loaded buffer is invalid");
	}

	int TrainingManager::get_last_checkpoint() const
	{
		return static_cast<int>(metadata["last_checkpoint"]);
	}
	int TrainingManager::get_best_checkpoint() const
	{
		return static_cast<int>(metadata["best_checkpoint"]);
	}
	float TrainingManager::get_learning_rate() const
	{
		float result = 0.0f;
		for (auto iter = config.training_config.learning_rate_schedule.begin(); iter < config.training_config.learning_rate_schedule.end(); iter++)
			if (get_last_checkpoint() >= iter->first)
				result = iter->second;
		std::cout << "using learning rate = " << result << '\n';
		return result;
	}

} /* namespace ag */

