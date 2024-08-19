#pragma once

#include <memory>
#include <string>

namespace Dao {

	class LogSystem;
	class AssetManager;
	class ConfigManager;
	class RenderSystem;

	class RuntimeGlobalContext {
	public:
		void startSystem();
		void shutdownSystem();

	public:
		std::shared_ptr<LogSystem>		m_log_system;
		std::shared_ptr<AssetManager>	m_asset_manager;
		std::shared_ptr<ConfigManager>	m_config_manager;
		std::shared_ptr<RenderSystem>	m_render_system;
	};
	
	extern RuntimeGlobalContext g_runtime_global_context;
}