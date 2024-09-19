#include "runtime/function/input/input_system.h"

#include "runtime/engine.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/core/base/macro.h"

#include <GLFW/glfw3.h>

namespace Dao {

	uint64_t k_complement_control_command = UINT64_MAX;

	void InputSystem::initialize() {
		std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
		ASSERT(window_system);
		window_system->registerOnKeyFunc(
			std::bind(&InputSystem::onKey, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4)
		);
		window_system->registerOnCursorPosFunc(
			std::bind(&InputSystem::onCursorPos, this, std::placeholders::_1, std::placeholders::_2)
		);
	}

	void InputSystem::tick() {
		calculateCursorDeltaAngles();
		clear();

		std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
		if (window_system->getFocusMode()) {
			_input_command &= (k_complement_control_command ^ (uint64_t)InputKey::INVALID);
		}
		else {
			_input_command |= (uint64_t)InputKey::INVALID;
		}
	}

	void InputSystem::clear() {
		m_cursor_delta_x = 0;
		m_cursor_delta_y = 0;
	}

	void InputSystem::onKey(int key, int sancode, int action, int mods) {
		if (!g_is_editor_mode) {
			onKeyInGameMode(key, sancode, action, mods);
		}
	}

	void InputSystem::onKeyInGameMode(int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
				_input_command |= (1ui64 << (key - GLFW_KEY_0));
			}
			else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
				_input_command |= (1ui64 << (key - GLFW_KEY_A + 10));
			}
			else if (key >= GLFW_KEY_ESCAPE && key <= GLFW_KEY_TAB) {
				_input_command |= (1ui64 << (key - GLFW_KEY_ESCAPE + 36));
			}
			else if (key >= GLFW_KEY_DELETE && key <= GLFW_KEY_UP) {
				_input_command |= (1ui64 << (key - GLFW_KEY_DELETE + 39));
			}
			else if (key >= GLFW_KEY_LEFT_SHIFT && key <= GLFW_KEY_RIGHT_SUPER) {
				_input_command |= (1ui64 << (key - GLFW_KEY_LEFT_SHIFT + 44));
			}
			else if (key >= GLFW_MOUSE_BUTTON_LEFT && key <= GLFW_MOUSE_BUTTON_MIDDLE) {
				_input_command |= (1ui64 << (key - GLFW_MOUSE_BUTTON_LEFT + 52));
			}
			else if (key == GLFW_KEY_SPACE) {
				_input_command |= (uint64_t)InputKey::KEY_SPACE;
			}
		}
		else if (action == GLFW_RELEASE) {
			if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
				_input_command &= (k_complement_control_command ^ (1ui64 << (key - GLFW_KEY_0)));
			}
			else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
				_input_command &= (k_complement_control_command ^ (1ui64 << (key - GLFW_KEY_A + 10)));
			}
			else if (key >= GLFW_KEY_ESCAPE && key <= GLFW_KEY_TAB) {
				_input_command &= (k_complement_control_command ^ (1ui64 << (key - GLFW_KEY_ESCAPE + 36)));
			}
			else if (key >= GLFW_KEY_DELETE && key <= GLFW_KEY_UP) {
				_input_command &= (k_complement_control_command ^ (1ui64 << (key - GLFW_KEY_DELETE + 39)));
			}
			else if (key >= GLFW_KEY_LEFT_SHIFT && key <= GLFW_KEY_RIGHT_SUPER) {
				_input_command &= (k_complement_control_command ^ (1ui64 << (key - GLFW_KEY_LEFT_SHIFT + 44)));
			}
			else if (key >= GLFW_MOUSE_BUTTON_LEFT && key <= GLFW_MOUSE_BUTTON_MIDDLE) {
				_input_command &= (k_complement_control_command ^ (1ui64 << (key - GLFW_MOUSE_BUTTON_LEFT + 52)));
			}
			else if (key == GLFW_KEY_SPACE) {
				_input_command &= (k_complement_control_command ^ (uint64_t)InputKey::KEY_SPACE);
			}
		}
	}

	void InputSystem::onCursorPos(double cursor_current_x, double cursor_current_y) {
		if (g_runtime_global_context.m_window_system->getFocusMode()) {
			m_cursor_delta_x = _last_cursor_x - cursor_current_x;
			m_cursor_delta_y = _last_cursor_y - cursor_current_y;
		}
		_last_cursor_x = cursor_current_x;
		_last_cursor_y = cursor_current_y;
	}

	void InputSystem::calculateCursorDeltaAngles() {
		std::array<int, 2> window_size = g_runtime_global_context.m_window_system->getWindowSize();
		if (window_size[0] < 1 || window_size[1] < 1) {
			return;
		}
		std::shared_ptr<RenderCamera> render_camera = g_runtime_global_context.m_render_system->getRenderCamera();
		const Vector2 fov = render_camera->getFOV();
		Radian cursor_delta_x(Math::degreesToRadians(m_cursor_delta_x));
		Radian cursor_delta_y(Math::degreesToRadians(m_cursor_delta_y));
		m_cursor_delta_yaw = (cursor_delta_x / float(window_size[0])) * fov.x;
		m_cursor_delta_pitch = -(cursor_delta_y / float(window_size[1])) * fov.y;
	}
}