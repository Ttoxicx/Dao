#include "global_context.h"

#include "core/log/log_system.h"
#include "resource/asset_manager/asset_manager.h"
#include "resource/config_manager/config_manager.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/particle/particle_manager.h"

namespace Dao {
	RuntimeGlobalContext g_runtime_global_context;

	void RuntimeGlobalContext::startSystem() {
		m_log_system = std::make_shared<LogSystem>();
		m_asset_manager = std::make_shared<AssetManager>();
		m_config_manager = std::make_shared<ConfigManager>();
		m_render_system = std::make_shared<RenderSystem>();
		m_particle_manager = std::make_shared<ParticleManager>();

		m_log_system->log(LogSystem::LogLevel::warn, "Global context is not complete");
	}

	void RuntimeGlobalContext::shutdownSystem() {
		m_log_system.reset();
	}
}