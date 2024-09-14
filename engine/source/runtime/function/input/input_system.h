#pragma once

#include "runtime/core/math/math.h"

namespace Dao {

	enum class InputKey :uint64_t {
		KEY_0 = 1ui64,
		KEY_1 = 1ui64 << 1,
		KEY_2 = 1ui64 << 2,
		KEY_3 = 1ui64 << 3,
		KEY_4 = 1ui64 << 4,
		KEY_5 = 1ui64 << 5,
		KEY_6 = 1ui64 << 6,
		KEY_7 = 1ui64 << 7,
		KEY_8 = 1ui64 << 8,
		KEY_9 = 1ui64 << 9,

		KEY_A = 1ui64 << 10,
		KEY_B = 1ui64 << 11,
		KEY_C = 1ui64 << 12,
		KEY_D = 1ui64 << 13,
		KEY_E = 1ui64 << 14,
		KEY_F = 1ui64 << 15,
		KEY_G = 1ui64 << 16,
		KEY_H = 1ui64 << 17,
		KEY_I = 1ui64 << 18,
		KEY_J = 1ui64 << 19,
		KEY_K = 1ui64 << 20,
		KEY_L = 1ui64 << 21,
		KEY_M = 1ui64 << 22,
		KEY_N = 1ui64 << 23,
		KEY_O = 1ui64 << 24,
		KEY_P = 1ui64 << 25,
		KEY_Q = 1ui64 << 26,
		KEY_R = 1ui64 << 27,
		KEY_S = 1ui64 << 28,
		KEY_T = 1ui64 << 29,
		KEY_U = 1ui64 << 30,
		KEY_V = 1ui64 << 31,
		KEY_W = 1ui64 << 32,
		KEY_X = 1ui64 << 33,
		KEY_Y = 1ui64 << 34,
		KEY_Z = 1ui64 << 35,

		KEY_ESCAPE = 1ui64 << 36,
		KEY_ENTER = 1ui64 << 37,
		KEY_TAB = 1ui64 << 38,

		KEY_DELETE = 1ui64 << 39,
		KEY_RIGHT = 1ui64 << 40,
		KEY_LEFT = 1ui64 << 41,
		KEY_DOWN = 1ui64 << 42,
		KEY_UP = 1ui64 << 43,

		KEY_LEFT_SHIFT = 1ui64 << 44,
		KEY_LEFT_CONTROL = 1ui64 << 45,
		KEY_LEFT_ALT = 1ui64 << 46,
		KEY_LEFT_SUPER = 1ui64 << 47,
		KEY_RIGHT_SHIFT = 1ui64 << 48,
		KEY_RIGHT_CONTROL = 1ui64 << 49,
		KEY_RIGHT_ALT = 1ui64 << 50,
		KEY_RIGHT_SUPER = 1ui64 << 51,

		MOUSE_BUTTON_LEFT = 1ui64 << 52,
		MOUSE_BUTTON_RIGHT = 1ui64 << 53,
		MOUSE_BUTTON_MIDDLE = 1ui64 << 54,
		KEY_SPACE = 1ui64 << 55,
		INVALID = 1ui64 << 63
	};

	extern uint64_t k_complement_control_command;

	class InputSystem {
	public:
		void initialize();
		void tick();
		void clear();

		void onKey(int key, int scancode, int action, int modes);
		void onCursorPos(double current_cursor_x, double current_cursor_y);

		int m_cursor_delta_x{ 0 };
		int m_cursor_delta_y{ 0 };

		Radian m_cursor_delta_yaw{ 0 };
		Radian m_cursor_delta_pitch{ 0 };

		void resetInputCommand() { _input_command = 0; }
		uint64_t getInputCommand() const { return _input_command; }

	private:
		void onKeyInGameMode(int key, int sancode, int action, int modes);
		void calculateCursorDeltaAngles();

		int _last_cursor_x{ 0 };
		int _last_cursor_y{ 0 };

		uint64_t _input_command{ 0 };
	};
}