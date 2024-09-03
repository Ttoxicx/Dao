#pragma once

#include "runtime/core/math/transform.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/component/camera/camera_component.h"

#include <vector>

namespace Dao {

	class Character {
		inline static const float s_camera_blend_time{ 0.3f };

	public:
		Character(std::shared_ptr<GObject> charactor_object);

		void tick(float delta_time);

		GObjectID getObjectID() const;
		void setObject(std::shared_ptr<GObject> gobject);
		std::weak_ptr<GObject> getObject() const { return m_character_object; }

		void setPosition(const Vector3& position) { m_position = position; }
		void setRotation(const Quaternion& rotation) { m_rotation = rotation; }

		const Vector3& getPosition() const { return m_position; }
		const Quaternion& getRotation() const { return m_rotation; }

	private:
		void toggleFreeCamera();

	private:
		Vector3 m_position;
		Quaternion m_rotation;

		std::shared_ptr<GObject> m_character_object;

		Quaternion m_rotation_buffer;
		bool m_rotation_dirty{ false };

		CameraMode m_original_camera_mode;
		bool m_is_free_camera{ false };
	};
}