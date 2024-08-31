#pragma once

#include "runtime/function/controller/controller.h"
#include "runtime/resource/res_type/components/rigid_body.h"
#include "runtime/resource/res_type/data/basic_shape.h"

namespace Dao {
	class CharacterController :public Controller {
	public:
		CharacterController(const Capsule& capsule);
		~CharacterController() = default;

		Vector3 move(const Vector3& current_position, const Vector3& displacement) override;

	private:
		Capsule _capsule;
		RigidBodyShape _rigidbody_shape;
	};
}