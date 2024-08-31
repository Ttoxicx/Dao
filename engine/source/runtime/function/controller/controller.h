#pragma once

#include "runtime/core/math/vector3.h"

namespace Dao {

	enum SweepPass {
		SWEEP_PASS_UP,
		SWEEP_PASS_SIDE,
		SWEEP_PASS_DOWN,
		SWEEP_PASS_SENSOR
	};

	class Controller {
	public:
		virtual ~Controller() = default;

		virtual Vector3 move(const Vector3& current_position, const Vector3& displacement) = 0;
	};
}