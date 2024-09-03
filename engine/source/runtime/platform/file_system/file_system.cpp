#include "runtime/platform/file_system/file_system.h"

namespace Dao {
	std::vector<std::filesystem::path> FileSystem::getFiles(const std::filesystem::path& directory) {
		std::vector<std::filesystem::path> files;
		for (auto const& directory_entry : std::filesystem::recursive_directory_iterator{ directory }) {
			if (directory_entry.is_regular_file()) {
				files.push_back(directory_entry);
			}
		}
		return files;
	}
}