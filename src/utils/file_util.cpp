/*
 * FileLoader.cpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#include <alphagomoku/utils/file_util.hpp>
#include <fstream>
#include <iterator>
#include <iostream>

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
	void FileSaver::save(const Json &json, const SerializedObject &binary_data)
	{
		std::string to_save = json.dump(2);
		stream.write(to_save.data(), to_save.size());
		stream.put('\n');
		stream.write(binary_data.data(), binary_data.size());
	}
	void FileSaver::close()
	{
		stream.close();
	}

	FileLoader::FileLoader(const std::string &path)
	{
		std::fstream file(path, std::fstream::in);
		loaded_string = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
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
			if (loaded_string[i] == '{')
				opened_braces++;
			if (loaded_string[i] == '}')
				opened_braces--;
			if (opened_braces == 0)
				return i + 2; // +1 for }, another +1 for \n
		}
		return loaded_string.size();
	}
}

