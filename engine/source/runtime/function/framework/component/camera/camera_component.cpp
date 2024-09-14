#include "runtime/function/framework/component/camera/camera_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/math/math_headers.h"
#include "runtime/function/character/character.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"

namespace Dao {
	void CameraComponent::postLoadResource(std::weak_ptr<GObject> parent_object) {
		m_parent_object = parent_object;
		const std::string& camera_type_name = _camera_res.m_parameter.getTypeName();
		if (camera_type_name == "FirstPersonCameraParameter") {
			_camera_mode = CameraMode::FIRST_PERSON;
		}
		else if (camera_type_name == "ThirdPersonCameraParameter") {
			_camera_mode = CameraMode::THIRD_PERSON;
		}
		else if (camera_type_name == "FreeCameraParameter") {
			_camera_mode = CameraMode::FREE;
		}
		else {
			LOG_ERROR("invalid camera type");
		}

		RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
		CameraSwapData camera_swap_data;
		camera_swap_data.m_fov_x = _camera_res.m_parameter->m_fov;
		swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;
	}

	void CameraComponent::tick(float delta_time) {
		if (!m_parent_object.lock()) {
			return;
		}
		std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
		std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
		if (current_character == nullptr) {
			return;
		}
		if (current_character->getObjectID() != m_parent_object.lock()->getID()) {
			return;
		}

		switch (_camera_mode)
		{
		case Dao::CameraMode::FIRST_PERSON:
			tickFirstPersonCamera(delta_time);
			break;
		case Dao::CameraMode::THIRD_PERSON:
			tickThirdPersonCamera(delta_time);
			break;
		case Dao::CameraMode::FREE:
			tickFreeCamera(delta_time);
			break;
		case Dao::CameraMode::INVALID:
			break;
		default:
			break;
		}
	}

	void CameraComponent::tickFirstPersonCamera(float delta_time) {
		std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
		std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
		if (current_character == nullptr) {
			return;
		}

		Quaternion q_yaw, q_pitch;
		q_yaw.fromAngleAxis(g_runtime_global_context.m_input_system->m_cursor_delta_yaw, Vector3::UNIT_Z);
		q_pitch.fromAngleAxis(g_runtime_global_context.m_input_system->m_cursor_delta_pitch, _left);

		const float offset = static_cast<FirstPersonCameraParameter*>(_camera_res.m_parameter)->m_vertical_offset;
		_position = current_character->getPosition() + offset * Vector3::UNIT_Z;
		_forward = q_yaw * q_pitch * _forward;
		_left = q_yaw * q_pitch * _left;
		_up = _forward.crossProduct(_left);

		Matrix4x4 desired_mat = Math::makeLookAtMatrix(_position, _position + _forward, _up);

		RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
		CameraSwapData camera_swap_data;
		camera_swap_data.m_camera_type = RenderCameraType::MOTOR;
		camera_swap_data.m_view_matrix = desired_mat;
		swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;

		Vector3 object_facing = _forward - _forward.dotProduct(Vector3::UNIT_Z) * Vector3::UNIT_Z;
		Vector3 object_left = Vector3::UNIT_Z.crossProduct(object_facing);
		Quaternion object_rotation;
		object_rotation.fromAxes(object_left, -object_facing, Vector3::UNIT_Z);
		current_character->setRotation(object_rotation);
	}

	void CameraComponent::tickThirdPersonCamera(float delta_time) {
		std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
		std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
		if (current_character == nullptr) {
			return;
		}

		ThirdPersonCameraParameter* param = static_cast<ThirdPersonCameraParameter*>(_camera_res.m_parameter);

		Quaternion q_yaw, q_pitch;
		q_yaw.fromAngleAxis(g_runtime_global_context.m_input_system->m_cursor_delta_yaw, Vector3::UNIT_Z);
		q_pitch.fromAngleAxis(g_runtime_global_context.m_input_system->m_cursor_delta_pitch, Vector3::UNIT_X);

		param->m_cursor_pitch = q_pitch * param->m_cursor_pitch;
		
		const float vertical_offset = param->m_vertical_offset;
		const float horizontal_offset = param->m_horizontal_offset;
		Vector3 offset = Vector3(0, horizontal_offset, vertical_offset);
		Vector3 center_pos = current_character->getPosition() + Vector3::UNIT_Z * vertical_offset;
		_position = current_character->getRotation() * param->m_cursor_pitch * offset + current_character->getPosition();
		_forward = center_pos - _position;
		_up = current_character->getRotation() * param->m_cursor_pitch * Vector3::UNIT_Z;

		current_character->setRotation(q_yaw * current_character->getRotation());

		Matrix4x4 desired_mat = Math::makeLookAtMatrix(_position, _position + _forward, _up);
		RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
		CameraSwapData camera_swap_data;
		camera_swap_data.m_camera_type = RenderCameraType::MOTOR;
		camera_swap_data.m_view_matrix = desired_mat;
		swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;
	}

	void CameraComponent::tickFreeCamera(float delta_time) {
		uint64_t command = g_runtime_global_context.m_input_system->getInputCommand();
		if (command >= (uint64_t)InputKey::INVALID) {
			return;
		}
		std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
		std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
		if (current_character == nullptr) {
			return;
		}

		Quaternion q_yaw, q_pitch;
		q_yaw.fromAngleAxis(g_runtime_global_context.m_input_system->m_cursor_delta_yaw, Vector3::UNIT_Z);
		q_pitch.fromAngleAxis(g_runtime_global_context.m_input_system->m_cursor_delta_pitch, _left);

		_forward = q_yaw * q_pitch * _forward;
		_left = q_yaw * q_pitch * _left;
		_up = _forward.crossProduct(_left);


		bool has_move_command = ((uint64_t)InputKey::KEY_W | (uint64_t)InputKey::KEY_A | (uint64_t)InputKey::KEY_S | (uint64_t)InputKey::KEY_D) & command;

		if (has_move_command) {
			Vector3 move_direction = Vector3::ZERO;
			if ((uint64_t)InputKey::KEY_W & command) {
				move_direction += _forward;
			}
			if ((uint64_t)InputKey::KEY_S & command) {
				move_direction -= _forward;
			}
			if ((uint64_t)InputKey::KEY_A & command) {
				move_direction += _left;
			}
			if ((uint64_t)InputKey::KEY_D & command) {
				move_direction -= _left;
			}
			_position += move_direction * 2.0f * delta_time;
		}

		Matrix4x4 desired_mat = Math::makeLookAtMatrix(_position, _position + _forward, _up);

		RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
		CameraSwapData camera_swap_data;
		camera_swap_data.m_camera_type = RenderCameraType::MOTOR;
		camera_swap_data.m_view_matrix = desired_mat;
		swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;
	}
}