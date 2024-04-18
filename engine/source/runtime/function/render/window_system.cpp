#include "runtime/function/render/window_system.h"

#include "runtime/core/base/macro.h"

namespace Dao {

	WindowSystem::~WindowSystem() {
		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	void WindowSystem::initialize(WindowCreateInfo create_info) {
		if (!glfwInit()) {
			LOG_FATAL("failed to initialize GLFW");
			return;
		}
		_width = create_info.width;
		_height = create_info.height;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_window = glfwCreateWindow(create_info.width, create_info.height, create_info.title, nullptr, nullptr);
		if (!_window) {
			LOG_FATAL("failed to create window");
			glfwTerminate();
			return;
		}
		glfwSetWindowUserPointer(_window, this);
		glfwSetKeyCallback(_window, keyCallback);
		glfwSetCharCallback(_window, charCallBack);
		glfwSetCharModsCallback(_window, charModesCallBack);
		glfwSetMouseButtonCallback(_window, mouseButtonCallBack);
		glfwSetCursorPosCallback(_window, cursorPosCallback);
		glfwSetCursorEnterCallback(_window, cursorEnterCallback);
		glfwSetScrollCallback(_window, scrollCallBack);
		glfwSetDropCallback(_window, dropCallBack);
		glfwSetWindowSizeCallback(_window, windowSizeCallBack);
		glfwSetWindowCloseCallback(_window, windowCloseCallback);
		glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
	}

	void WindowSystem::pollEvents() const { 
		glfwPollEvents();
	}

	bool WindowSystem::shouldClose() const {
		return glfwWindowShouldClose(_window);
	}

	void WindowSystem::setTitle(const char* title) {
		glfwSetWindowTitle(_window, title);
	}

	GLFWwindow* WindowSystem::getWindow() const {
		return _window;
	}

	std::array<int, 2> WindowSystem::getWindowSize() const {
		return std::array<int, 2>({ _width,_height });
	}

	void WindowSystem::setFocusMode(bool mode) {
		_is_focus_mode = mode;
		glfwSetInputMode(
			_window, GLFW_CURSOR, 
			_is_focus_mode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
		);
	}
}