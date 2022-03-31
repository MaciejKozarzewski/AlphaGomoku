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

#include <libml/hardware/Device.hpp>

#include <filesystem>
#include <iostream>

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
		result["networks"]["freestyle"] = "freestyle_10x128.bin";
		result["networks"]["standard"] = "standard_10x128.bin";
		result["networks"]["renju"] = "";
		result["networks"]["caro"] = "";
		result["use_symmetries"] = true;
		result["search_threads"] = 1;
		result["devices"][0] = ag::DeviceConfig().toJson();
		result["search_options"] = ag::SearchConfig().toJson();
		result["tree_options"] = ag::TreeConfig().toJson();
		return result;
	}
	struct HardwareConfiguration
	{
			ml::Device device;
			int search_threads;
			int omp_threads;
			std::vector<int> batch_size;
			std::vector<float> speed; // [samples / second]
			HardwareConfiguration(const Json &json) :
					device(ml::Device::fromString(json["device"])),
					search_threads(static_cast<int>(json["search_threads"])),
					omp_threads(static_cast<int>(json["omp_threads"]))
			{
				for (int i = 0; i < json["batch"].size(); i++)
				{
					batch_size.push_back(static_cast<int>(json["batch"][i]));
					speed.push_back(static_cast<double>(json["samples"][i]) / static_cast<double>(json["time"][i]));
				}
			}
			void print() const
			{
				std::cout << device.toString() << " " << search_threads << ":" << omp_threads << '\n';
				for (size_t i = 0; i < batch_size.size(); i++)
					std::cout << "--" << batch_size[i] << " = " << speed[i] << " n/s\n";
				std::cout << '\n';
			}
			std::pair<int, float> getOptimalParams() const noexcept
			{
				float max_speed = *(std::max_element(speed.begin(), speed.end()));
				for (size_t i = 0; i < speed.size(); i++)
					if (speed[i] >= 0.95 * max_speed)
						return std::pair<int, float>( { batch_size[i], speed[i] });
				return std::pair<int, float>( { 0, 0.0f }); // this should never happen
			}
	};

	HardwareConfiguration filter_configs(const std::vector<HardwareConfiguration> &configs, ml::Device device)
	{
		size_t index = 0;
		std::pair<int, float> best_param( { 0, 0.0f });
		for (size_t i = 0; i < configs.size(); i++)
			if (configs[i].device == device)
			{
				std::pair<int, float> tmp = configs[i].getOptimalParams();
				if (tmp.second > best_param.second)
				{
					index = i;
					best_param = tmp;
				}
			}
		return configs[index];
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

		Json result = create_base_config();
		int search_threads = 0;
		int max_batch_size = 0;
		std::vector<DeviceConfig> device_configs;
		if (ml::Device::numberOfCudaDevices() > 0)
		{
			for (int i = 0; i < ml::Device::numberOfCudaDevices(); i++)
			{
				HardwareConfiguration best_config = filter_configs(configs, ml::Device::cuda(i));
				search_threads += best_config.search_threads;
				for (int j = 0; j < best_config.search_threads; j++)
				{
					DeviceConfig tmp;
					tmp.batch_size = best_config.getOptimalParams().first;
					tmp.device = ml::Device::cuda(i);
					device_configs.push_back(tmp);
					max_batch_size = std::max(max_batch_size, tmp.batch_size);
				}
			}
			result["search_options"]["vcf_solver_max_positions"] = 200;
			result["tree_options"]["bucket_size"] = 1000000;
		}
		else
		{
			HardwareConfiguration best_config = filter_configs(configs, ml::Device::cpu());
			search_threads = best_config.search_threads;
			DeviceConfig tmp;
			tmp.batch_size = best_config.getOptimalParams().first;
			tmp.device = ml::Device::cpu();
			device_configs.push_back(tmp);

			max_batch_size = tmp.batch_size;
			result["search_options"]["vcf_solver_max_positions"] = 1600;
		}

		result["search_threads"] = search_threads;
		result["devices"] = Json(JsonType::Array);
		for (size_t i = 0; i < device_configs.size(); i++)
			result["devices"][i] = device_configs[i].toJson();

		result["search_options"]["max_batch_size"] = max_batch_size;
		result["search_options"]["expansion_prior_treshold"] = 1.0e-4f;
		result["search_options"]["max_children"] = 30;
		result["search_options"]["vcf_solver_level"] = 2;

		return result;
	}

} /* namespace ag */

