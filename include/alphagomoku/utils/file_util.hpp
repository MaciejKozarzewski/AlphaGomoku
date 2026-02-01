/*
 * FileLoader.hpp
 *
 *  Created on: Mar 6, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_UTILS_FILE_UTIL_HPP_
#define ALPHAGOMOKU_UTILS_FILE_UTIL_HPP_

#include <minml/utils/json.hpp>
#include <minml/utils/serialization.hpp>

#include <vector>
#include <cstddef>
#include <fstream>
#include <string>

namespace ag
{
	bool pathExists(const std::string &path);
	bool createDirectory(const std::string &path);
	bool removeFile(const std::string &path);

	template<typename T>
	void serializeVector(const std::vector<T> &result, SerializedObject &binary_data)
	{
		binary_data.save<uint32_t>(static_cast<uint32_t>(result.size()));
		binary_data.save(result.data(), sizeof(T) * result.size());
	}
	template<typename T>
	void unserializeVector(std::vector<T> &result, const SerializedObject &binary_data, size_t &offset)
	{
		const uint32_t size = binary_data.load<uint32_t>(offset);
		offset += sizeof(uint32_t);
		result.resize(size);

		const uint64_t size_in_bytes = sizeof(T) * size;
		binary_data.load(result.data(), offset, size_in_bytes);
		offset += size_in_bytes;
	}

	class FileSaver
	{
			std::string path;
			std::ofstream stream;
		public:
			FileSaver(const std::string &path);
			std::string getPath() const;
			void save(const Json &json, const SerializedObject &binary_data, int indent = -1, bool compress = false);
	};

	class FileLoader
	{
			Json json;
			SerializedObject binary_data;
			std::vector<char> loaded_data;
			size_t split_point;
		public:
			FileLoader(const std::string &path, bool uncompress = false);
			const Json& getJson() const noexcept;
			Json& getJson() noexcept;
			const SerializedObject& getBinaryData() const noexcept;
			SerializedObject& getBinaryData() noexcept;
		private:
			void load_all_data();
			size_t find_split_point() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_UTILS_FILE_UTIL_HPP_ */
