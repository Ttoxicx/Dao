#include "global_context.h"

#include "core/log/log_system.h"

namespace Dao {
	RuntimeGlobalContext g_runtime_global_context;

	void RuntimeGlobalContext::startSystem() {
		m_log_system = std::make_shared<LogSystem>();
		m_log_system->log(LogSystem::LogLevel::warn, "Global context is not complete");
	}

	void RuntimeGlobalContext::shutdownSystem() {
		m_log_system.reset();
	}
}