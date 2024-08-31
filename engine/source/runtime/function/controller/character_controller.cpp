#include "runtime/function/controller/character_controller.h"

#include "runtime/core/base/macro.h"

namespace Dao {
	CharacterController::CharacterController(const Capsule& capsule) :_capsule(capsule) {
		_rigidbody_shape = RigidBodyShape();
		_rigidbody_shape.m_geometry = DAO_REFLECTION_NEW(Capsule);
		*static_cast<Capsule*>(_rigidbody_shape.m_geometry) = _capsule;
		_rigidbody_shape.m_type = RigidBodyShapeType::capsule;

		Quaternion orientation;
		orientation.fromAngleAxis(Radian(Degree(90.f)), Vector3::UNIT_X);
		_rigidbody_shape.m_local_transform = Transform(Vector3(0, 0, capsule.m_half_height + capsule.m_radius), orientation, Vector3::UNIT_SCALE);
	}

	Vector3 CharacterController::move(const Vector3& current_position, const Vector3& displacement) {
		//TODO(complete)
		return Vector3();
	}
}