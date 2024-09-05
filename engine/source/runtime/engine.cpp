#include "runtime/engine.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/core/meta/reflection/reflection_register.h"

namespace Dao {
	bool g_is_editor_mode{ false };
	std::unordered_set<std::string> g_editor_tick_component_types{};

	void DaoEngine::startEngine(const std::string& config_file_path) {
		Reflection::TypeMetaRegister::metaRegister();
		g_runtime_global_context.startSystems(config_file_path);

		LOG_INFO("engine start");
	}

	void DaoEngine::shutdownEngine() {
		LOG_INFO("engine shutdown");

		g_runtime_global_context.shutdownSystems();
		Reflection::TypeMetaRegister::metaUnregister();
	}

	void DaoEngine::initialize() {}

	void DaoEngine::clear() {}

	void DaoEngine::run() {
		std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
		ASSERT(window_system);
		
		while (!window_system->shouldClose()){
			const float delta_time = calculateDeltaTime();
			tickOneFrame(delta_time);
		}
	}

	float DaoEngine::calculateDeltaTime() {
		float delta_time;
		using namespace std::chrono;
		steady_clock::time_point tick_time_point = steady_clock::now();
		duration<float> time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);
		delta_time = time_span.count();
		m_last_tick_time_point = tick_time_point;
		return delta_time;
	}

	bool DaoEngine::tickOneFrame(float delta_time) {
		logicalTick(delta_time);
		calculateFPS(delta_time);

		g_runtime_global_context.m_render_system->swapLogicRenderData();
		rendererTick(delta_time);

		g_runtime_global_context.m_window_system->setTitle(std::string("Dao - " + std::to_string(getFPS()) + " FPS").c_str());

		const bool should_not_window_close = !(g_runtime_global_context.m_window_system->shouldClose());

		return should_not_window_close;
	}

	void DaoEngine::logicalTick(float delta_time) {
		g_runtime_global_context.m_world_manager->tick(delta_time);
		g_runtime_global_context.m_input_system->tick();
	}

	void DaoEngine::rendererTick(float delta_time) {
		g_runtime_global_context.m_render_system->tick(delta_time);
	}

	const float DaoEngine::s_fps_alpha = 1.0f / 100;
	void DaoEngine::calculateFPS(float delta_time) {
		m_frame_count++;
		if (m_frame_count == 1) {
			m_average_duration = delta_time;
		}
		else {
			m_average_duration = m_average_duration * (1 - s_fps_alpha) + delta_time * s_fps_alpha;
		}
		m_fps = static_cast<int>(1.0f / m_average_duration);
	}
}