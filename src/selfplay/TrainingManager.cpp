/*
 * TrainingManager.cpp
 *
 *  Created on: Mar 27, 2021
 *      Author: maciek
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
			config(FileLoader(workingDirectory + "config.json").getJson()),
			metadata( { { "last_checkpoint", 0 }, { "best_checkpoint", 0 }, { "learning_steps", 0 } }),
			working_dir(workingDirectory),
			generator(config),
			evaluator(config),
			supervised_learning(config)
	{
		config["working_directory"] = workingDirectory;
		supervised_learning = SupervisedLearning(config);
		if (loadMetadata())
			std::cout << "Loading existing training run\n" << metadata.dump(2) << '\n';
		else
		{
			std::cout << "Initializing new training run\n";
			saveMetadata();
			initFolderTree();
			initModel();
			supervised_learning.saveTrainingHistory();
		}
	}

	void TrainingManager::runIterationRL()
	{
		generateGames();

		train();
		validate();
		supervised_learning.saveTrainingHistory();

		evaluate();
		metadata["last_checkpoint"] = get_last_checkpoint() + 1;
		saveMetadata();
	}
	void TrainingManager::runIterationSL(const std::string &buffer_src)
	{
	}
	//private
	void TrainingManager::initFolderTree()
	{
		std::filesystem::create_directory(working_dir + "/checkpoint/");	// create folder for networks
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
		AGNetwork model(config);
		model.saveToFile(working_dir + "/checkpoint/network_0.bin");
		model.optimize();
		model.saveToFile(working_dir + "/checkpoint/network_0_opt.bin");
	}
	void TrainingManager::generateGames()
	{
		std::string path_to_best_network = working_dir + "/checkpoint/network_" + std::to_string(static_cast<int>(metadata["best_checkpoint"]))
				+ "_opt.bin";
		std::cout << "Loading " << path_to_best_network << '\n';
		int training_games = static_cast<int>(config["selfplay_options"]["games_per_iteration"]);
		int validation_games = static_cast<int>(config["selfplay_options"]["games_per_iteration"])
				* static_cast<int>(config["training_options"]["validation_percent"]) / 100;
		std::cout << "Generating " << (training_games + validation_games) << " games\n";

		generator.getGameBuffer().clear();
		generator.generate(path_to_best_network, training_games + validation_games);
		if (generator.getGameBuffer().isCorrect() == false)
			throw std::runtime_error("generated buffer is invalid");
		std::cout << "Finished generating games\n";

		splitBuffer(generator.getGameBuffer(), training_games, validation_games);
	}

	void TrainingManager::train()
	{
		GameBuffer buffer;
		loadBuffer(buffer, working_dir + "/train_buffer/");

		AGNetwork model;
		model.loadFromFile(working_dir + "/checkpoint/network_" + std::to_string(get_last_checkpoint()) + ".bin");
		model.setBatchSize(static_cast<int>(config["training_options"]["batch_size"]));
		model.getGraph().moveTo(ml::Device::fromString(config["training_options"]["device"]));
		model.changeLearningRate(get_learning_rate());

		supervised_learning.loadProgress(metadata);
		supervised_learning.train(model, buffer, static_cast<int>(config["training_options"]["steps_per_iteration"]));
		metadata["learning_steps"] = supervised_learning.saveProgress()["learning_steps"];

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
		model.setBatchSize(static_cast<int>(config["training_options"]["batch_size"]));
		model.getGraph().moveTo(ml::Device::fromString(config["training_options"]["device"]));

		supervised_learning.validate(model, buffer);
		std::cout << "Validation finished\n";
	}
	void TrainingManager::evaluate()
	{
		if (static_cast<bool>(config["evaluation_options"]["use_evaluation"]) == false)
		{
			metadata["best_network"] = get_last_checkpoint() + 1;
			return;
		}
		std::string path_to_networks = working_dir + "/checkpoint/network_";

		evaluator.setFirstPlayer(config["evaluation_options"], path_to_networks + std::to_string(get_last_checkpoint() + 1) + "_opt.bin",
				get_name(get_last_checkpoint() + 1));

		std::cout << "Evaluating network " << get_last_checkpoint() + 1 << " vs";
		for (int i = 0; i < evaluator.numberOfThreads(); i++)
		{
			evaluator.setSecondPlayer(i, config["evaluation_options"],
					path_to_networks + std::to_string(std::max(0, get_best_checkpoint() - i)) + "_opt.bin",
					get_name(std::max(0, get_best_checkpoint() - i)));
			std::cout << ' ' << std::to_string(std::max(0, get_best_checkpoint() - i)) << ',';
		}
		std::cout << '\n';

		evaluator.generate(static_cast<int>(config["evaluation_options"]["games_per_iteration"]));
		std::string to_save;
		for (int i = 0; i < evaluator.numberOfThreads(); i++)
			to_save += evaluator.getGameBuffer(i).generatePGN();
		std::ofstream file(working_dir + "/rating.pgn", std::fstream::app);
		file << to_save;
		file.close();

		if (config["evaluation_options"]["gating"]["use_gating"])
		{
			GameBufferStats stats;
			for (int i = 0; i < evaluator.numberOfThreads(); i++)
				stats += evaluator.getGameBuffer(i).getStats();
			if (stats.cross_win + 0.5 * stats.draws >= static_cast<float>(config["evaluation_options"]["gating"]["treshold"]))
				metadata["best_network"] = get_last_checkpoint() + 1;
		}
		else
			metadata["best_checkpoint"] = get_last_checkpoint() + 1;
		std::cout << "Evaluation finished\n";
	}
	void TrainingManager::splitBuffer(const GameBuffer &buffer, int training_games, int validation_games)
	{
		std::cout << "Saving validation buffer\n";
		std::vector<int> ordering = permutation(training_games + validation_games);
		GameBuffer tmp;
		for (int i = 0; i < validation_games; i++)
			tmp.addToBuffer(buffer.getFromBuffer(ordering[i]));
		tmp.save(working_dir + "/valid_buffer/buffer_" + std::to_string(static_cast<int>(metadata["last_checkpoint"])) + ".bin");

		std::cout << "Saving validation buffer\n";
		tmp.clear();
		for (int i = 0; i < training_games; i++)
			tmp.addToBuffer(buffer.getFromBuffer(ordering[validation_games + i]));
		tmp.save(working_dir + "/train_buffer/buffer_" + std::to_string(static_cast<int>(metadata["last_checkpoint"])) + ".bin");
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
		Json schedule = config["training_options"]["learning_rate_schedule"];
		for (int i = 0; i < schedule.size(); i++)
			if (get_last_checkpoint() >= static_cast<float>(schedule[i]["from_epoch"]))
				result = schedule[i]["value"];
		std::cout << "using learning rate = " << result << '\n';
		return result;
	}

} /* namespace ag */

