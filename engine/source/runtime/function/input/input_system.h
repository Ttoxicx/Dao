#pragma once

#include "runtime/core/math/math.h"

namespace Dao {

	enum class InputCommand :unsigned int {
		W = 1 << 0,
		A = 1 << 1,
		S = 1 << 2,
		D = 1 << 3,
		E = 1 << 4,
		Q = 1 << 5,
		LEFT_ALT = 1 << 6,
		SPACE = 1 << 7,
		INVALID = (unsigned int)(1 << 31)
	};

	extern unsigned int k_complement_control_command;

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

		void resetGameCommand() { _input_command = 0; }
		unsigned int getGameCommand() const { return _input_command; }

	private:
		void onKeyInGameMode(int key, int sancode, int action, int modes);
		void calculateCursorDeltaAngles();

		int _last_cursor_x{ 0 };
		int _last_cursor_y{ 0 };

		unsigned int _input_command{ 0 };
	};
}