/*
 * FileLoader.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/file_util.hpp>
#include <libml/utils/ZipWrapper.hpp>
#include <fstream>
#include <iterator>
#include <iostream>
#include <filesystem>

namespace ag
{

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
		std::string to_save = json.dump(indent);
		to_save += '\n';
		to_save.append(binary_data.data(), binary_data.size());
		if (compress == true)
			to_save = ZipWrapper::compress(to_save);
		stream.write(to_save.data(), to_save.size());
	}
	void FileSaver::close()
	{
		stream.close();
	}

	FileLoader::FileLoader(const std::string &path, bool uncompress)
	{
		if (not std::filesystem::exists(path))
			throw std::runtime_error("File '" + path + "' does not exist");

		std::fstream file(path, std::fstream::in);
		loaded_string = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		if (uncompress == true)
			loaded_string = ZipWrapper::uncompress(loaded_string);
		split_point = find_split_point();
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
		json = Json::load(loaded_string);
		binary_data.save(loaded_string.data() + split_point, loaded_string.size() - split_point);
	}
	size_t FileLoader::find_split_point() const noexcept
	{
		int opened_braces = 0;
		for (size_t i = 0; i < loaded_string.size(); i++)
		{
			if (loaded_string[i] == '{' || loaded_string[i] == '[')
				opened_braces++;
			if (loaded_string[i] == '}' || loaded_string[i] == ']')
				opened_braces--;
			if (opened_braces == 0)
				return i + 2; // +1 for }, another +1 for \n
		}
		return loaded_string.size();
	}
}

