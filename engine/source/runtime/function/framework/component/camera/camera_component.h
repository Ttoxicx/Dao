#pragma once

#include "runtime/core/math/vector3.h"
#include "runtime/resource/res_type/components/camera.h"
#include "runtime/function/framework/component/component.h"

namespace Dao {

	class RenderCamera;

	enum class CameraMode :unsigned char {
		FIRST_PERSON,
		THIRD_PERSON,
		FREE,
		INVALID,
	};

	REFLECTION_TYPE(CameraComponent);
	CLASS(CameraComponent:public Component, WhiteListFields)
	{
		REFLECTION_BODY(CameraComponent);
	public:
		CameraComponent() = default;

		void postLoadResource(std::weak_ptr<GObject> parent_object) override;

		void tick(float delta_time) override;

		CameraMode getCameraMode() const { return _camera_mode; }
		void setCameraMode(CameraMode mode) { _camera_mode = mode; }

		Vector3 getPosition() const { return _position; }
		Vector3 getForward() const { return _forward; }

	private:
		void tickFirstPersonCamera(float delta_time);
		void tickThirdPersonCamera(float delta_time);
		void tickFreeCamera(float delta_time);

	private:
		META(Enable) CameraComponentRes _camera_res;
		CameraMode _camera_mode{ CameraMode::INVALID };
		Vector3 _position;
		Vector3 _forward{ Vector3::NEGATIVE_UNIT_Y };
		Vector3 _up{ Vector3::UNIT_Z };
		Vector3 _left{ Vector3::UNIT_X };
	};
}