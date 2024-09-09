#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <functional>
#include <vector>

namespace Dao {

	struct WindowCreateInfo {
		int width{ 1280 };
		int height{ 720 };
		const char* title{ "Dao" };
		bool fullscreen{ false };
	};

	class WindowSystem {
	public:
		WindowSystem() = default;
		~WindowSystem();
		void initialize(WindowCreateInfo create_info);
		void pollEvents() const;
		bool shouldClose() const;
		void setTitle(const char* title);
		GLFWwindow* getWindow() const;
		std::array<int, 2> getWindowSize() const;

		typedef std::function<void()>					onResetFunc;
		typedef std::function<void(int, int, int, int)> onKeyFunc;
		typedef std::function<void(unsigned int)>		onCharFunc;
		typedef std::function<void(int, unsigned int)>	onCharModsFunc;
		typedef std::function<void(int, int, int)>		onMouseButtonFunc;
		typedef std::function<void(double, double)>		onCursorPosFunc;
		typedef std::function<void(int)>				onCursorEnterFunc;
		typedef std::function<void(double, double)>		onScrollFunc;
		typedef std::function<void(int, const char**)>	onDropFunc;
		typedef std::function<void(int, int)>			onWindowSizeFunc;
		typedef std::function<void()>					onWindowCloseFunc;

		void registeronResetFunc(onResetFunc func) { _onResetFunc.push_back(func); }
		void registeronKeyFunc(onKeyFunc func) { _onKeyFunc.push_back(func); }
		void registeronCharFunc(onCharFunc func) { _onCharFunc.push_back(func); }
		void registeronCharModsFunc(onCharModsFunc func) { _onCharModsFunc.push_back(func); }
		void registeronMouseButtonFunc(onMouseButtonFunc func) { _onMouseButtonFunc.push_back(func); }
		void registeronCursorPosFunc(onCursorPosFunc func) { _onCursorPosFunc.push_back(func); }
		void registeronCursorEnterFunc(onCursorEnterFunc func) { _onCursorEnterFunc.push_back(func); }
		void registeronScrollFunc(onScrollFunc func) { _onScrollFunc.push_back(func); }
		void registeronDropFunc(onDropFunc func) { _onDropFunc.push_back(func); }
		void registeronWindowSizeFunc(onWindowSizeFunc func) { _onWindowSizeFunc.push_back(func); }
		void registeronWindowCloseFunc(onWindowCloseFunc func) { _onWindowCloseFunc.push_back(func); }

		bool isMouseButtonDown(int button) const {
			if (button<GLFW_MOUSE_BUTTON_1 || button>GLFW_MOUSE_BUTTON_LAST) {
				return false;
			}
			return glfwGetMouseButton(_window, button) == GLFW_PRESS;
		}
		bool getFocusMode() const { return _is_focus_mode; }
		void setFocusMode(bool mode);
	protected:

		void onReset() {
			for (auto& func : _onResetFunc) {
				func();
			}
		}

		void onKey(int key, int scancode, int action, int mods) {
			for (auto& func : _onKeyFunc) {
				func(key, scancode, action, mods);
			}
		}

		void onChar(unsigned int code_point) {
			for (auto& func : _onCharFunc) {
				func(code_point);
			}
		}

		void onCharMods(int code_point, unsigned int mods) {
			for (auto& func : _onCharModsFunc) {
				func(code_point, mods);
			}
		}

		void onMouseButton(int button, int action, int mods) {
			for (auto& func : _onMouseButtonFunc) {
				func(button, action, mods);
			}
		}

		void onCursorPos(double xpos, double ypos) {
			for (auto& func : _onCursorPosFunc) {
				func(xpos, ypos);
			}
		}

		void onCursorEnter(int entered) {
			for (auto& func : _onCursorEnterFunc) {
				func(entered);
			}
		}

		void onScroll(double xoffset, double yoffset) {
			for (auto& func : _onScrollFunc) {
				func(xoffset, yoffset);
			}
		}

		void onDrop(int count, const char** paths) {
			for (auto& func : _onDropFunc) {
				func(count, paths);
			}
		}

		void onWindowSize(int width, int height) {
			for (auto& func : _onWindowSizeFunc) {
				func(width, height);
			}
		}

		void onWindowClose() {
			for (auto& func : _onWindowCloseFunc) {
				func();
			}
		}

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onKey(key, scancode, action, mods);
			}
		}

		static void charCallBack(GLFWwindow* window, unsigned int code_point) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onChar(code_point);
			}
		}

		static void charModesCallBack(GLFWwindow* window, unsigned int code_point, int mods) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onCharMods(code_point, mods);
			}
		}

		static void mouseButtonCallBack(GLFWwindow* window, int button, int action, int mods) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onMouseButton(button, action, mods);
			}
		}

		static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onCursorPos(xpos, ypos);
			}
		}

		static void cursorEnterCallback(GLFWwindow* window, int entered) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onCursorEnter(entered);
			}
		}

		static void scrollCallBack(GLFWwindow* window, double xoffset, double yoffset) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onScroll(xoffset, yoffset);
			}
		}

		static void dropCallBack(GLFWwindow* window, int count, const char** paths) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->onDrop(count, paths);
			}
		}

		static void windowSizeCallBack(GLFWwindow* window, int width, int height) {
			WindowSystem* app_window = (WindowSystem*)glfwGetWindowUserPointer(window);
			if (app_window) {
				app_window->_width = width;
				app_window->_height = height;
				app_window->onWindowSize(width, height);
			}
		}

		static void windowCloseCallback(GLFWwindow* window) {
			glfwSetWindowShouldClose(window, true);
		}

	private:
		GLFWwindow* _window{ nullptr };
		int _width{ 0 };
		int _height{ 0 };
		bool _is_focus_mode{ false };

		std::vector<onResetFunc>			_onResetFunc;
		std::vector<onKeyFunc>				_onKeyFunc;
		std::vector<onCharFunc>				_onCharFunc;
		std::vector<onCharModsFunc>			_onCharModsFunc;
		std::vector<onMouseButtonFunc>		_onMouseButtonFunc;
		std::vector<onCursorPosFunc>		_onCursorPosFunc;
		std::vector<onCursorEnterFunc>		_onCursorEnterFunc;
		std::vector<onScrollFunc>			_onScrollFunc;
		std::vector<onDropFunc>				_onDropFunc;
		std::vector<onWindowSizeFunc>		_onWindowSizeFunc;
		std::vector<onWindowCloseFunc>		_onWindowCloseFunc;
	};
}