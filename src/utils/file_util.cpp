/*
 * FileLoader.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/file_util.hpp>

#include <minml/utils/ZipWrapper.hpp>

#include <fstream>
#include <iterator>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace ag
{
	bool pathExists(const std::string &path)
	{
		return std::filesystem::exists(path);
	}
	bool createDirectory(const std::string &path)
	{
		return std::filesystem::create_directory(path);
	}
	bool removeFile(const std::string &path)
	{
		return std::filesystem::remove(path);
	}

	FileSaver::FileSaver(const std::string &path) :
			path(path),
			stream(path, std::ofstream::out)
	{
	}
	std::string FileSaver::getPath() const
	{
		return path;
	}
	void FileSaver::save(const Json &json, const SerializedObject &binary_data, int indent, bool compress)
	{
		std::string json_string = json.dump(indent);

		std::vector<char> to_save(json_string.begin(), json_string.end());
		to_save.push_back('\n');
		to_save.insert(to_save.end(), binary_data.data(), binary_data.data() + binary_data.size());
		if (compress == true)
			to_save = ZipWrapper::compress(to_save);
		stream.write(to_save.data(), to_save.size());
	}

	FileLoader::FileLoader(const std::string &path, bool uncompress)
	{
		if (std::filesystem::exists(path) == false)
			throw std::runtime_error("File '" + path + "' does not exist");

		uintmax_t filesize = std::filesystem::file_size(path);
		loaded_data.assign(filesize, '\0');
		std::fstream file(path, std::fstream::in);
		if (file.good() == false)
			throw std::runtime_error("File '" + path + "' could not be opened");
		file.read(loaded_data.data(), filesize);
		file.close();

		if (uncompress == true)
			loaded_data = ZipWrapper::uncompress(loaded_data);
		split_point = std::min(loaded_data.size(), find_split_point());
		load_all_data();
	}
	const Json& FileLoader::getJson() const noexcept
	{
		return json;
	}
	Json& FileLoader::getJson() noexcept
	{
		return json;
	}
	const SerializedObject& FileLoader::getBinaryData() const noexcept
	{
		return binary_data;
	}
	SerializedObject& FileLoader::getBinaryData() noexcept
	{
		return binary_data;
	}
	void FileLoader::load_all_data()
	{
		std::string str(loaded_data.begin(), loaded_data.begin() + split_point);
		json = Json::load(str);
		binary_data.save(loaded_data.data() + split_point, loaded_data.size() - split_point);
	}
	size_t FileLoader::find_split_point() const noexcept
	{
		if (loaded_data[0] == 'n' and loaded_data[1] == 'u' and loaded_data[2] == 'l' and loaded_data[3] == 'l')
			return 5;

		int opened_braces = 0;
		for (size_t i = 0; i < loaded_data.size(); i++)
		{
			if (loaded_data[i] == '{' || loaded_data[i] == '[')
				opened_braces++;
			if (loaded_data[i] == '}' || loaded_data[i] == ']')
				opened_braces--;
			if (opened_braces == 0)
				return i + 2; // +1 for }, another +1 for \n
		}
		return loaded_data.size();
	}
}

