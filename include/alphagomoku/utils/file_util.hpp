/*
 * FileLoader.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_UTILS_FILE_UTIL_HPP_
#define ALPHAGOMOKU_UTILS_FILE_UTIL_HPP_

#include <libml/utils/json.hpp>
#include <libml/utils/serialization.hpp>
#include <stddef.h>
#include <fstream>
#include <string>

namespace ag
{
	class FileSaver
	{
			std::string path;
			std::ofstream stream;
		public:
			FileSaver(const std::string &path);
			std::string getPath() const;
			void save(const Json &json, const SerializedObject &binary_data);
			void close();
	};

	class FileLoader
	{
			Json json;
			SerializedObject binary_data;
			std::string loaded_string;
			size_t split_point;
		public:
			FileLoader(const std::string &path);
			const Json& getJson() const noexcept;
			Json& getJson() noexcept;
			const SerializedObject& getBinaryData() const noexcept;
			SerializedObject& getBinaryData() noexcept;
		private:
			void load_all_data();
			size_t find_split_point() const noexcept;
	};
}

#endif /* ALPHAGOMOKU_UTILS_FILE_UTIL_HPP_ */
