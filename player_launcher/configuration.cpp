/*
 * configuration.cpp
 *
 *  Created on: Jun 11, 2021
 *      Author: Maciej Kozarzewski
 */

#include "configuration.hpp"

#include <alphagomoku/utils/file_util.hpp>

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
		result["use_logging"] = false;
		result["always_ponder"] = false;
		result["swap2_openings_file"] = "swap2_openings.json";
		result["networks"]["freestyle"] = "freestyle_10x128.bin";
		result["networks"]["standard"] = "standard_10x128.bin";
		result["networks"]["renju"] = "";
		result["networks"]["caro"] = "";
		result["use_symmetries"] = true;
		result["threads"][0] = Json();
		result["search_options"]["batch_size"] = 1;
		result["search_options"]["exploration_constant"] = 1.25;
		result["search_options"]["expansion_prior_treshold"] = 1.0e-4;
		result["search_options"]["max_children"] = 30;
		result["search_options"]["noise_weight"] = 0.0;
		result["search_options"]["use_endgame_solver"] = true;
		result["search_options"]["use_vcf_solver"] = true;
		result["tree_options"]["max_number_of_nodes"] = 500000000;
		result["tree_options"]["bucket_size"] = 100000;
		result["cache_options"]["min_cache_size"] = 1048576;
		result["cache_options"]["max_cache_size"] = 1048576;
		result["cache_options"]["update_from_search"] = false;
		result["cache_options"]["update_visit_treshold"] = 1000;
		return result;
	}

}

namespace ag
{
	Json loadConfig(const std::string &localLaunchPath)
	{
		if (std::filesystem::exists(localLaunchPath + "config.json"))
		{
			try
			{
				FileLoader fl(localLaunchPath + "config.json");
				return fl.getJson();
			} catch (std::exception &e)
			{
				std::cout << "The configuration file is invalid for some reason. Try deleting it and creating a new one." << std::endl;
				return Json();
			}
		}
		else
		{
			std::cout << "Could not load configuration file.\n"
					<< "You can generate new one by launching AlphaGomoku from command line with parameter 'configure'\n"
					<< "If you are sure that file 'config.json' exists, it means that launch path might not have been parsed correctly due to some special characters in it."
					<< std::endl;
			return Json();
		}
	}

	void createConfig(const std::string &localLaunchPath)
	{
#ifdef USE_TEXT_UI
		std::vector<std::pair<bool, ml::Device>> list_of_devices;
		list_of_devices.push_back( { false, ml::Device::cpu() });
		for (int i = 0; i < ml::Device::numberOfCudaDevices(); i++)
			list_of_devices.push_back( { false, ml::Device::cuda(i) });

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

