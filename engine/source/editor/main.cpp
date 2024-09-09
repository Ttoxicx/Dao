#include <iostream>
#include "runtime/core/base/macro.h"
#include "runtime/engine.h"

#include "runtime/core/meta/test.h"
namespace Dao {
	namespace Test {
		void logTest() {
			LOG_ERROR("engine is still in initial stage")
		}
		void metaTest() {
			Dao::metaTest();
		}
	}
}

#include "runtime/function/input/input_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/render_swap_context.h"

namespace Dao {
	void inputTest() {
		auto window = g_runtime_global_context.m_window_system;
		auto render = g_runtime_global_context.m_render_system;
		window->registeronWindowSizeFunc([=](int x, int y) {
			render->updateEngineContentViewport(0, 0, x, y);
			}
		);
		auto input = g_runtime_global_context.m_input_system;
		window->registeronKeyFunc([=](int key, int scancode, int action, int mods) {
			if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
				window->setFocusMode(!window->getFocusMode());
			}
			input->onKey(key, scancode, action, mods);
			}
		);
	}
}

int main() {
	
	std::filesystem::path config_file_path = "config/DaoEditor.ini";

	Dao::DaoEngine engine;

	engine.startEngine(config_file_path.generic_string());
	engine.initialize();
	Dao::inputTest();
	engine.run();
	engine.clear();
	engine.shutdownEngine();

	return 0;
}