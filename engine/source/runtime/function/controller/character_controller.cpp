#include "runtime/function/controller/character_controller.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/physics/physics_scene.h"

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
		std::shared_ptr<PhysicsScene> physics_scene = g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
		ASSERT(physics_scene);
		Vector3 final_position = current_position + displacement;
		Transform final_transform = Transform(final_position, Quaternion::IDENTITY, Vector3::UNIT_SCALE);
		if (physics_scene->isOverlap(_rigidbody_shape, final_transform.getMatrix())) {
			final_position = current_position;
		}
		return final_position;
	}
}