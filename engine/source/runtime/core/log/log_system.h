#pragma once

#include <spdlog/spdlog.h>

#include <cstdint>
#include <stdexcept>

namespace Dao {

	class LogSystem final {
	public:
		enum class LogLevel : uint8_t
		{
			debug,
			info,
			warn,
			error,
			fatal
		};

	public:
		LogSystem();
		~LogSystem();

		template<typename... TARGS>
		void log(LogLevel level, TARGS&... args) {
			switch (level) {
			case LogLevel::debug:
				_logger->debug(std::forward<TARGS>(args)...);
				break;
			case LogLevel::info:
				_logger->info(std::forward<TARGS>(args)...);
				break;
			case LogLevel::warn:
				_logger->warn(std::forward<TARGS>(args)...);
				break;
			case LogLevel::error:
				_logger->error(std::forward<TARGS>(args)...);
				break;
			case LogLevel::fatal:
				_logger->critical(std::forward<TARGS>(args)...);
				fatalCallBack(std::forward<TARGS>(args)...);
				break;
			default:
				break;
			}
		}

		template<typename... TARGS>
		void fatalCallBack(TARGS&&... args) {
			const std::string format_str = fmt::format(std::forward<TARGS>(args)...);
			throw std::runtime_error(format_str);
		}

	private:
		std::shared_ptr<spdlog::logger> _logger;
	};
}