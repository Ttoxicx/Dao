#pragma once

#include <filesystem>
#include <vector>

namespace Dao {

	class FileSystem {
	public:
		std::vector<std::filesystem::path> getFiles(const std::filesystem::path& directory);
	};
}