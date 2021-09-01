/*
 * configuration.cpp
 *
 *  Created on: Jun 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include "engine_manager.hpp"

#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <libml/hardware/Device.hpp>

#ifdef USE_TEXT_UI
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#endif

#include <filesystem>
#include <iostream>

namespace
{
	Json create_base_config()
	{
		Json result;
		result["protocol"] = "gomocup";
		result["use_logging"] = false;
		result["always_ponder"] = false;
		result["swap2_openings_file"] = "swap2_openings.json";
		result["networks"]["freestyle"] = "freestyle_10x128.bin";
		result["networks"]["standard"] = "standard_10x128.bin";
		result["networks"]["renju"] = "";
		result["networks"]["caro"] = "";
		result["threads"] = Json(JsonType::Array);
		result["search_options"]["max_batch_size"] = 1;
		result["search_options"]["exploration_constant"] = 1.25;
		result["search_options"]["expansion_prior_treshold"] = 1.0e-4;
		result["search_options"]["max_children"] = 30;
		result["search_options"]["use_vcf_solver"] = true;
		result["search_options"]["use_symmetries"] = true;
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
	};
}

namespace ag
{
	void createConfig(const Json &benchmarkResults)
	{
		Json result = create_base_config();
		if (ml::Device::numberOfCudaDevices() > 0)
		{

		}
		else
		{
		}

//		std::vector<HardwareConfiguration> configs;
//		for (int i = 0; i < benchmarkResults["tests"].size(); i++)
//			configs.push_back(HardwareConfiguration(benchmarkResults["tests"][i]));

//		std::vector<std::pair<bool, ml::Device>> list_of_devices;
//		list_of_devices.push_back( { false, ml::Device::cpu() });
//		for (int i = 0; i < ml::Device::numberOfCudaDevices(); i++)
//			list_of_devices.push_back( { false, ml::Device::cuda(i) });

#ifdef USE_TEXT_UI
		auto screen = ftxui::ScreenInteractive::FitComponent();

		std::wstring number_of_threads;
		auto input = ftxui::Input(&number_of_threads, L"placeholder");

		auto checkboxes = ftxui::Container::Vertical( { });
		for (size_t i = 0; i < list_of_devices.size(); ++i)
		{
			std::string description = list_of_devices[i].second.toString() + " : " + list_of_devices[i].second.info();
			checkboxes->Add(ftxui::Checkbox(std::wstring(description.begin(), description.end()), &list_of_devices[i].first));
		}
		auto button = ftxui::Button(L"Save", screen.ExitLoopClosure());

		auto layout = ftxui::Container::Vertical( { input, checkboxes, button });
		auto component = ftxui::Renderer(layout, [&]
		{
			return ftxui::vbox(
					{	input->Render(), ftxui::separator(), checkboxes->Render(), ftxui::separator(), button->Render()}) |
			ftxui::xflex | size(ftxui::WIDTH, ftxui::GREATER_THAN, 40) | ftxui::border;
		});

		screen.Loop(component);
#endif
	}

} /* namespace ag */

