#include "runtime/function/input/input_system.h"

#include "runtime/engine.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"
#include "runtime/core/base/macro.h"

#include <GLFW/glfw3.h>

namespace Dao {

	unsigned int k_complement_control_command = 0xFFFFFFFF;

	void InputSystem::initialize() {
		std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
		ASSERT(window_system);
		window_system->registeronKeyFunc(
			std::bind(&InputSystem::onKey, this,
				std::placeholders::_1, std::placeholders::_2,
				std::placeholders::_3, std::placeholders::_4)
		);
		window_system->registeronCursorPosFunc(
			std::bind(&InputSystem::onCursorPos, this, std::placeholders::_1, std::placeholders::_2)
		);
	}

	void InputSystem::tick() {
		calculateCursorDeltaAngles();
		clear();

		std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
		if (window_system->getFocusMode()) {
			_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::INVALID);
		}
		else {
			_input_command |= (unsigned int)InputCommand::INVALID;
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
			switch (key) {
			case GLFW_KEY_W:
				_input_command |= (unsigned int)InputCommand::W;
				break;
			case GLFW_KEY_A:
				_input_command |= (unsigned int)InputCommand::A;
				break;
			case GLFW_KEY_S:
				_input_command |= (unsigned int)InputCommand::S;
				break;
			case GLFW_KEY_D:
				_input_command |= (unsigned int)InputCommand::D;
				break;
			case GLFW_KEY_E:
				_input_command |= (unsigned int)InputCommand::E;
				break;
			case GLFW_KEY_Q:
				_input_command |= (unsigned int)InputCommand::Q;
				break;
			case GLFW_KEY_LEFT_ALT:
				_input_command |= (unsigned int)InputCommand::LEFT_ALT;
				break;
			case GLFW_KEY_SPACE:
				_input_command |= (unsigned int)InputCommand::SPACE;
				break;
			default:
				break;
			}
		}
		else if (action == GLFW_RELEASE) {
			switch (key) {
			case GLFW_KEY_W:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::W);
				break;
			case GLFW_KEY_A:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::A);
				break;
			case GLFW_KEY_S:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::S);
				break;
			case GLFW_KEY_D:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::D);
				break;
			case GLFW_KEY_E:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::E);
				break;
			case GLFW_KEY_Q:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::Q);
				break;
			case GLFW_KEY_LEFT_ALT:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::LEFT_ALT);
				break;
			case GLFW_KEY_SPACE:
				_input_command &= (k_complement_control_command ^ (unsigned int)InputCommand::SPACE);
				break;
			default:
				break;
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
		m_cursor_delta_pitch = (cursor_delta_y / float(window_size[1])) * fov.y;
	}
}