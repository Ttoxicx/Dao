#pragma once

#include <filesystem>

namespace Dao {

	class ConfigManager {
	public:
		void initialize(const std::filesystem::path& config_file_path);

		const std::filesystem::path& getRootFolder() const { return _root_folder; }
		const std::filesystem::path& getAssetFolder() const { return _asset_folder; }
		const std::filesystem::path& getSchemaFolder() const { return _schema_folder; }
		const std::string& getDefaultWorldUrl() const { return _default_world_url; }
		const std::string& getGlobalRenderingResUrl() const { return _global_rendering_res_url; }
		const std::string& getGlobalParticleResUrl() const { return _global_particle_res_url; }

	private:
		std::filesystem::path _root_folder;
		std::filesystem::path _asset_folder;
		std::filesystem::path _schema_folder;

		std::string _default_world_url;
		std::string _global_rendering_res_url;
		std::string _global_particle_res_url;
	};
}