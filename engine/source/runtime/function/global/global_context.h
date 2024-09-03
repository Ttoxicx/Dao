#pragma once

#include <memory>
#include <string>

namespace Dao {

	class LogSystem;
	class FileSystem;
	class AssetManager;
	class ConfigManager;
	class RenderSystem;
	class InputSystem;
	class WindowSystem;
	class WorldManager;
	class ParticleManager;
	class PhysicsManager;

	class RuntimeGlobalContext {
	public:
		void startSystems(const std::string& config_file_path);
		void shutdownSystems();

	public:
		std::shared_ptr<LogSystem>			m_log_system;
		std::shared_ptr<FileSystem>			m_file_system;
		std::shared_ptr<AssetManager>		m_asset_manager;
		std::shared_ptr<ConfigManager>		m_config_manager;
		std::shared_ptr<RenderSystem>		m_render_system;
		std::shared_ptr<InputSystem>        m_input_system;
		std::shared_ptr<WindowSystem>       m_window_system;
		std::shared_ptr<WorldManager>       m_world_manager;
		std::shared_ptr<ParticleManager>	m_particle_manager;
		std::shared_ptr<PhysicsManager>     m_physics_manager;
	};
	
	extern RuntimeGlobalContext g_runtime_global_context;
}