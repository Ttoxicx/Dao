#include "runtime/resource/config_manager/config_manager.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace Dao {

    void ConfigManager::initialize(const std::filesystem::path& config_file_path) {
        std::ifstream config_file(config_file_path);
        std::string config_line;
        while (std::getline(config_file, config_line)) {
            size_t seperate_pos = config_line.find_first_of('=');
            if (seperate_pos > 0 && seperate_pos < (config_line.length() - 1)) {
                std::string name = config_line.substr(0, seperate_pos);
                std::string value = config_line.substr(seperate_pos + 1, config_line.length() - seperate_pos - 1);
                if (name == "BinaryRootFolder") {
                    _root_folder = config_file_path.parent_path() / value;
                }
                else if (name == "AssetFolder") {
                    _asset_folder = _root_folder / value;
                }
                else if (name == "SchemaFolder") {
                    _schema_folder = _root_folder / value;
                }
                else if (name == "DefaultWorld") {
                    _default_world_url = value;
                }
                else if (name == "GlobalRenderingRes") {
                    _global_rendering_res_url = value;
                }
                else if (name == "GlobalParticleRes") {
                    _global_particle_res_url = value;
                }
            }
        }
    }
}