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
	void testInput() {
		auto window = g_runtime_global_context.m_window_system;
		auto render = g_runtime_global_context.m_render_system;
		window->registeronWindowSizeFunc([=](int x, int y) {
			render->updateEngineContentViewport(0, 0, x, y);
			}
		);
		auto input = g_runtime_global_context.m_input_system;
		window->registeronKeyFunc(
			[=](int key, int scancode, int action, int mods) {
				if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
					window->setFocusMode(false);
				}
			}
		);
	}
}

#include "runtime/function/ui/window_ui.h"
#include <imgui.h>
#include <imgui_internal.h>
namespace Dao {

	class UITest :public WindowUI {
	public:
		virtual void initialize(WindowUIInitInfo init_info) {
			//create imgui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			//set ui content scale
			float x_scale, y_scale;
			glfwGetWindowContentScale(init_info.window_system->getWindow(), &x_scale, &y_scale);

			init_info.render_system->initializeUIRenderBackend(this);
		}

		virtual void preRender() {
			GameWindowUI();
		}

	private:
		bool _show_ui = true;
		void GameWindowUI() {

			ImVec2 windowSize = ImGui::GetIO().DisplaySize;
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(windowSize);

			// Create an invisible window that covers the entire screen
			ImGui::Begin("UITest", &_show_ui, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground);

			// Set the cursor position to the top-right corner
			ImGui::SetCursorPos(ImVec2(windowSize.x - 100, 0)); // Adjust the x offset as needed

			// Focus mode
			bool focus_mode = g_runtime_global_context.m_window_system->getFocusMode();
			if (focus_mode) {
				if (ImGui::Button("Focus Mode")) {
					g_runtime_global_context.m_window_system->setFocusMode(!focus_mode);
				}
			}
			else {
				if (ImGui::Button("UnFocus Mode")) {
					g_runtime_global_context.m_window_system->setFocusMode(!focus_mode);
				}
			}

			ImGui::SetCursorPos(ImVec2(10, 0));
			if (focus_mode) {
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Press Left Alt key to display the mouse cursor!");
			}
			else {

			}

			ImGui::End();
		}
	};
	std::shared_ptr<UITest> ui;
	void testUI() {
		auto window = g_runtime_global_context.m_window_system;
		auto render = g_runtime_global_context.m_render_system;
		ui = std::make_shared<UITest>();
		ui->initialize({ window,render });
	}
}

int main() {
	
	std::filesystem::path config_file_path = "config/DaoEditor.ini";

	Dao::DaoEngine engine;

	engine.startEngine(config_file_path.generic_string());
	engine.initialize();
	Dao::testInput();
	Dao::testUI();
	engine.run();
	engine.clear();
	engine.shutdownEngine();

	return 0;
}