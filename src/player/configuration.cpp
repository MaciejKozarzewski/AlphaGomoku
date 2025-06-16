/*
 * configuration.cpp
 *
 *  Created on: Jun 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/version.hpp>

#include <minml/core/Device.hpp>

#include <filesystem>

namespace
{
	Json create_base_config()
	{
		Json result;
		result["version"] = ag::ProgramInfo::version();
		result["protocol"] = "gomocup";
		result["use_logging"] = false;
		result["always_ponder"] = false;
		result["swap2_openings_file"] = "swap2_openings.json";

		result["conv_networks"]["freestyle"] = "freestyle_conv_8x128.bin";
		result["conv_networks"]["freestyle_15"] = "freestyle_conv_8x128_15x15.bin";
		result["conv_networks"]["standard"] = "standard_conv_8x128.bin";
		result["conv_networks"]["renju"] = "renju_conv_8x128.bin";
		result["conv_networks"]["caro5"] = "caro5_conv_8x128.bin";
		result["conv_networks"]["caro6"] = "caro6_conv_8x128.bin";

		result["nnue_networks"]["freestyle"] = "";
		result["nnue_networks"]["freestyle_15"] = "";
		result["nnue_networks"]["standard"] = "";
		result["nnue_networks"]["renju"] = "";
		result["nnue_networks"]["caro5"] = "";
		result["nnue_networks"]["caro6"] = "";

		result["use_symmetries"] = true;
		result["search_threads"] = 1;
		result["devices"][0] = ag::DeviceConfig().toJson();
		result["search_config"] = ag::SearchConfig().toJson();
		return result;
	}
	struct HardwareConfiguration
	{
			ml::Device device = ml::Device::cpu();
			int search_threads = 0;
			std::vector<int> batch_size;
			std::vector<float> speed; // [samples / second]
			HardwareConfiguration() = default;
			HardwareConfiguration(const Json &json) :
					device(ml::Device::fromString(json["device"])),
					search_threads(json["search_threads"].getInt())
			{
				for (int i = 0; i < json["batch"].size(); i++)
				{
					batch_size.push_back(json["batch"][i].getInt());
					if (json.hasKey("speed"))
						speed.push_back(json["speed"][i].getDouble()); // new format from version 5.8.0
					else
						speed.push_back(json["samples"][i].getDouble() / json["time"][i].getDouble()); // old format used for versions before 5.8.0
				}
			}
			void print() const
			{
				std::cout << device.toString() << " " << search_threads << '\n';
				for (size_t i = 0; i < batch_size.size(); i++)
					std::cout << "--" << batch_size[i] << " = " << speed[i] << " n/s\n";
				std::cout << '\n';
			}
			std::pair<int, float> getOptimalParams() const noexcept
			{
				float max_speed = std::numeric_limits<float>::lowest();
				std::pair<int, float> result( { 0, 0.0f });
				for (size_t i = 0; i < speed.size(); i++)
					if (speed[i] > max_speed)
					{
						result = std::pair<int, float>( { batch_size[i], speed[i] });
						max_speed = speed[i];
					}
				return result;
			}
	};

	HardwareConfiguration filter_configs(const std::vector<HardwareConfiguration> &configs, ml::Device device, int maxThreads)
	{
		int index = -1;
		std::pair<int, float> best_param( { 0, 0.0f });
		for (size_t i = 0; i < configs.size(); i++)
			if (configs[i].device == device and configs[i].search_threads <= maxThreads)
			{
				std::pair<int, float> tmp = configs[i].getOptimalParams();
				if (tmp.second > best_param.second)
				{
					index = i;
					best_param = tmp;
				}
			}
		if (index == -1)
			return HardwareConfiguration();
		else
			return configs[index];
	}

	struct BestConfig
	{
			std::vector<ag::DeviceConfig> device_configs;
			float speed = 0.0;
			int max_batch_size = 0;
			int search_threads = 0;
	};

	BestConfig pick_config(const BestConfig &cpu, const BestConfig &cuda, const BestConfig &opencl) noexcept
	{
		if (cpu.speed >= cuda.speed and cpu.speed >= opencl.speed)
			return cpu;
		if (cuda.speed >= cpu.speed and cuda.speed >= opencl.speed)
			return cuda;
		if (opencl.speed >= cpu.speed and opencl.speed >= cuda.speed)
			return opencl;
		return BestConfig();
	}
	void update_config(BestConfig &cfg, const std::vector<HardwareConfiguration> &configs, ml::Device device, int num_threads)
	{
		const HardwareConfiguration best_config = filter_configs(configs, device, num_threads);
		cfg.search_threads += best_config.search_threads;
		cfg.speed += best_config.getOptimalParams().second;
		const int batch_size = best_config.getOptimalParams().first;
		for (int j = 0; j < best_config.search_threads; j++)
		{
			ag::DeviceConfig tmp;
			tmp.batch_size = batch_size;
			tmp.device = device;
			cfg.device_configs.push_back(tmp);
		}
		cfg.max_batch_size = std::max(cfg.max_batch_size, batch_size);
	}
}

namespace ag
{
	Json createDefaultConfig()
	{
		return create_base_config();
	}

	Json createConfig(const Json &benchmarkResults)
	{
		std::vector<HardwareConfiguration> configs;
		for (int i = 0; i < benchmarkResults["tests"].size(); i++)
			if (benchmarkResults["tests"][i].isNull() == false)
				configs.push_back(HardwareConfiguration(benchmarkResults["tests"][i]));

		const int num_cpu_cores = ml::Device::numberOfCpuCores();

		BestConfig best_cpu_config;
		update_config(best_cpu_config, configs, ml::Device::cpu(), num_cpu_cores);

		BestConfig best_cuda_config;
		const int num_cuda_devices = ml::Device::numberOfCudaDevices();
		for (int i = 0; i < num_cuda_devices; i++)
		{
			const int max_threads = std::max(1, (num_cpu_cores - best_cuda_config.search_threads) / (num_cuda_devices - i));
			update_config(best_cuda_config, configs, ml::Device::cuda(i), max_threads);
		}

		BestConfig best_opencl_config;
		const int num_opencl_devices = ml::Device::numberOfOpenCLDevices();
		for (int i = 0; i < num_opencl_devices; i++)
		{
			const int max_threads = std::max(1, (num_cpu_cores - best_opencl_config.search_threads) / (num_opencl_devices - i));
			update_config(best_opencl_config, configs, ml::Device::opencl(i), max_threads);
		}

		const BestConfig final_config = pick_config(best_cpu_config, best_cuda_config, best_opencl_config);

		Json result = create_base_config();

		result["search_threads"] = final_config.search_threads;
		result["devices"] = Json(JsonType::Array);
		for (size_t i = 0; i < final_config.device_configs.size(); i++)
			result["devices"][i] = final_config.device_configs[i].toJson();

		SearchConfig search_config;
		search_config.max_batch_size = final_config.max_batch_size;
		search_config.mcts_config.max_children = 32;
		search_config.mcts_config.edge_selector_config.exploration_constant = 0.5;
		search_config.mcts_config.edge_selector_config.init_to = "q_head";
		search_config.mcts_config.edge_selector_config.policy = "puct";
		search_config.tss_config.mode = 2;
		search_config.tss_config.hash_table_size = 4 * 1024 * 1024;

		result["search_config"] = search_config.toJson();

		return result;
	}

} /* namespace ag */

