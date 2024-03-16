#pragma once

#include <memory>
#include <string>

namespace Dao {

	class LogSystem;

	class RuntimeGlobalContext {
	public:
		void startSystem();
		void shutdownSystem();

	public:
		std::shared_ptr<LogSystem> m_log_system;
	};
	
	extern RuntimeGlobalContext g_runtime_global_context;
}