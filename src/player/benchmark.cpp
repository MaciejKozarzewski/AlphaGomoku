/*
 * benchmark.cpp
 *
 *  Created on: Aug 20, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/ProgramManager.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/networks/AGNetwork.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/version.hpp>

namespace
{
	using namespace ag;

	struct HardwareTestConfig
	{
			ml::Device device;
			int search_threads;
	};

	const std::vector<int>& batch_sizes()
	{
		static const std::vector<int> result = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 256 };
		return result;
	}

	Json test_speed(double max_time, const std::string &path, HardwareTestConfig config)
	{
		const GameConfig cfg(GameRules::STANDARD, 15, 15);
		std::vector<std::thread> threads(config.search_threads);
		std::vector<std::unique_ptr<AGNetwork>> networks(config.search_threads);
		for (size_t i = 0; i < networks.size(); i++)
		{
			networks[i] = loadAGNetwork(path);
			networks[i]->optimize(2);
			networks[i]->setBatchSize(batch_sizes().back());
			networks[i]->moveTo(config.device);
			networks[i]->convertToHalfFloats();
		}

		int total_samples = 0;
		double total_time = 0.0;
		std::mutex result_mutex;
		auto benchmark_function = [&](size_t index, int batch_size)
		{
			ml::Device::cpu().setNumberOfThreads(1);
			try
			{
				networks[index]->forward(batch_size);
				const double start = getTime();
				int processed_samples = 0;
				while (getTime() - start < max_time)
				{
					networks[index]->forward(batch_size);
					processed_samples += batch_size;
				}
				const double stop = getTime();
				std::lock_guard lock(result_mutex);
				total_samples += processed_samples;
				total_time += (stop - start);
			}
			catch (std::exception &e)
			{
			}
		};

		Json result;
		result["device"] = config.device.toString();
		result["search_threads"] = config.search_threads;
		result["batch"] = Json(JsonType::Array);
		result["samples"] = Json(JsonType::Array);
		result["time"] = Json(JsonType::Array);

		for (size_t i = 0; i < batch_sizes().size(); i++)
		{
			total_samples = 0;
			total_time = 0.0;
			for (size_t j = 0; j < threads.size(); j++)
				threads[j] = std::thread(benchmark_function, j, batch_sizes()[i]);

			for (size_t j = 0; j < threads.size(); j++)
				if (threads[j].joinable())
					threads[j].join();

			if (total_samples == 0)
				return Json(); // network inference didn't work for some reason (most likely out of memory)
			result["batch"][i] = batch_sizes()[i];
			result["speed"][i] = total_samples / (total_time / threads.size());
		}
		return result;
	}
}
namespace ag
{
	Json run_benchmark(const std::string &path_to_network, const OutputSender &output_sender)
	{
		const double benchmarking_time = 2.0; /**< how much time is spent on testing speed [s] */

		Json result;
		result["version"] = ProgramInfo::version();
		result["devices"][ml::Device::cpu().toString()] = ml::Device::cpu().info();
		for (int i = 0; i < ml::Device::numberOfCudaDevices(); i++)
			result["devices"][ml::Device::cuda(i).toString()] = ml::Device::cuda(i).info();
		for (int i = 0; i < ml::Device::numberOfOpenCLDevices(); i++)
			result["devices"][ml::Device::opencl(i).toString()] = ml::Device::opencl(i).info();

		std::vector<HardwareTestConfig> configs_to_test;

		const int cpu_cores = ml::Device::numberOfCpuCores();
		/* create list of CPU configurations to test */
		for (int search_threads = 1; search_threads <= cpu_cores; search_threads *= 2)
			configs_to_test.push_back( { ml::Device::cpu(), search_threads });

		/* create list of CUDA configurations to test */
		for (int device_index = 0; device_index < ml::Device::numberOfCudaDevices(); device_index++)
			for (int search_threads = 1; search_threads <= cpu_cores; search_threads *= 2)
				configs_to_test.push_back( { ml::Device::cuda(device_index), search_threads });

		/* create list of OpenCL configurations to test */
		for (int device_index = 0; device_index < ml::Device::numberOfOpenCLDevices(); device_index++)
			for (int search_threads = 1; search_threads <= cpu_cores; search_threads *= 2)
				configs_to_test.push_back({ ml::Device::opencl(device_index), search_threads });

		const int overhead_time = 2; /**< in seconds */
		const int estimated_time = ((batch_sizes().size() * benchmarking_time + overhead_time) * configs_to_test.size() + 59.9) / 60.0;

		/* run benchmarks */
		output_sender.send("Detected following devices:\n" + ml::Device::hardwareInfo());
		output_sender.send("Starting benchmark. This should take about " + std::to_string(estimated_time) + " minutes.");
		for (size_t i = 0; i < configs_to_test.size(); i++)
		{
			int progress = 100 * i / configs_to_test.size();
			output_sender.send(std::to_string(progress) + "% done, currently benchmarking " + configs_to_test[i].device.toString() + " ...");

			result["tests"][i] = test_speed(benchmarking_time, path_to_network, configs_to_test[i]);
		}
		output_sender.send("Benchmark finished.");
		return result;
	}
} /* namespace ag */

