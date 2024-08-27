#include "runtime/platform/path/path.h"

namespace Dao {
	const std::filesystem::path Path::getRelativePath(const std::filesystem::path& directory, const std::filesystem::path& file_path) {
		return file_path.lexically_relative(directory);
	}

	const std::vector<std::string> Path::getPathSegments(const std::filesystem::path& file_path) {
		std::vector<std::string> segments;
		for (auto itr = file_path.begin(); itr != file_path.end(); ++itr) {
			segments.emplace_back(itr->generic_string());
		}
		return segments;
	}


	const std::tuple<std::string, std::string, std::string> Path::getFileExtensions(const std::filesystem::path& file_path) {
		return std::make_tuple(
			file_path.extension().generic_string(),
			file_path.stem().extension().generic_string(),
			file_path.stem().stem().extension().generic_string()
		);
	}


	const std::string Path::getFileNameWithoutExtension(const std::string file_full_name) {
		std::string file_name = file_full_name;
		auto pos = file_full_name.find_first_of(".");
		if (pos != std::string::npos) {
			file_name = file_full_name.substr(0, pos);
		}
		return file_name;
	}
}