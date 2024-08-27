#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Dao {
	class DaoEngine {
		static const float s_fps_alpha;
	public:
		void startEngine();
		void shutdownEngine();
		
		void initialize();
		void clear();

		void run();
		bool tickOneFrame(float delta_time);

		bool isQuit() const {
			return m_is_quit;
		}

		int getFPS() const {
			return m_fps;
		}
	
	protected:
		void logicalTick(float delta_time);
		void rendererTick(float delta_time);

		void calculateFPS(float delta_time);
		float calculateDeltaTime();

	protected:
		std::chrono::steady_clock::time_point m_last_tick_time_point{ std::chrono::steady_clock::now() };

		bool m_is_quit{ false };
		float m_average_duration{ 0.f };
		int m_frame_count{ 0 };
		int m_fps{ 0 };
	};
}