#include "runtime/function/character/character.h"

#include "runtime/engine.h"
#include "runtime/function/framework/component/movement/movement_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"

namespace Dao {
	Character::Character(std::shared_ptr<GObject> character_object) {
		setObject(character_object);
	}

	GObjectID Character::getObjectID() const {
		if (m_character_object) {
			return m_character_object->getID();
		}
		return k_invalid_gobject_id;
	}

	void Character::setObject(std::shared_ptr<GObject> gobject) {
		m_character_object = gobject;
		if (m_character_object) {
			const TransformComponent* tranform_component = m_character_object->tryGetComponentConst(TransformComponent);
			const Transform& transform = tranform_component->getTransformConst();
			m_position = transform.m_position;
			m_rotation = transform.m_rotation;
		}
		else {
			m_position = Vector3::ZERO;
			m_rotation = Quaternion::IDENTITY;
		}
	}

	void Character::tick(float delta_time) {
		if (m_character_object == nullptr) {
			return;
		}

		uint64_t command = g_runtime_global_context.m_input_system->getInputCommand();
		if (command < (uint64_t)InputKey::INVALID) {
			if (((command & (uint64_t)InputKey::KEY_F) > 0) != m_is_free_camera) {
				toggleFreeCamera();
			}
		}

		TransformComponent* transform_component = m_character_object->tryGetComponent(TransformComponent);

		if (m_rotation_dirty) {
			transform_component->setRotation(m_rotation_buffer);
			m_rotation_dirty = false;
		}

		const MovementComponent* movement_component = m_character_object->tryGetComponentConst(MovementComponent);
		if (movement_component == nullptr) {
			return;
		}

		if (movement_component->getIsMoving()) {
			m_rotation_buffer = m_rotation;
			transform_component->setRotation(m_rotation_buffer);
			m_rotation_dirty = true;
		}

		const Vector3& new_position = movement_component->getTargetPosition();
		m_position = new_position;
	}

	void Character::toggleFreeCamera() {
		CameraComponent* camera_component = m_character_object->tryGetComponent(CameraComponent);
		if (camera_component == nullptr) {
			return;
		}

		m_is_free_camera = !m_is_free_camera;

		if (m_is_free_camera) {
			m_original_camera_mode = camera_component->getCameraMode();
			camera_component->setCameraMode(CameraMode::FREE);
		}
		else {
			camera_component->setCameraMode(m_original_camera_mode);
		}
	}
}