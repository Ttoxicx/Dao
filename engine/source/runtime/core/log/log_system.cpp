#include "log_system.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Dao {
	LogSystem::LogSystem() {
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::trace);
		console_sink->set_pattern("[%^%T%$] %v");
		const spdlog::sinks_init_list sink_list = { console_sink };
		spdlog::init_thread_pool(8192, 1);
		_logger = std::make_shared<spdlog::async_logger>(
			"dao_logger",
			sink_list.begin(),
			sink_list.end(),
			spdlog::thread_pool(),
			spdlog::async_overflow_policy::block
			);
		_logger->set_level(spdlog::level::trace);
		spdlog::register_logger(_logger);
	}

	LogSystem::~LogSystem() {
		_logger->flush();
		spdlog::drop_all();
	}
}