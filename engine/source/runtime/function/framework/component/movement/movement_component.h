#pragma once

#include "runtime/resource/res_type/components/movement.h"
#include "runtime/function/controller/character_controller.h"
#include "runtime/function/framework/component/component.h"

namespace Dao {

	enum class MovementState :unsigned char {
		MOVING,
		JUMPING
	};

	enum class JumpState :unsigned char {
		IDEL,
		RISING,
		FALLING,
	};

	REFLECTION_TYPE(MovementComponent);
	CLASS(MovementComponent:public Component, WhiteListFields, WhiteListMethods)
	{
		REFLECTION_BODY(MovementComponent);
	public:
		MovementComponent() = default;

		~MovementComponent() override;

		void postLoadResource(std::weak_ptr<GObject> parent_object) override;

		void tick(float delta_time) override;
		void tickPlayerMovement(float delta_time);

		const Vector3& getTargetPosition() const { return _target_position; }
		float getSpeedRatio() const { return _move_speed_ratio; }
		bool getIsMoving() const { return _is_moving; }

		META(Enable) void getOffStuckDead();

	private:
		void calculateDesiredHorizontalMoveSpeed(uint64_t commond, float delta_time);
		void calculateDesiredVerticalMoveSpeed(uint64_t commond, float delta_time);
		void calculateDesiredMoveDirection(uint64_t commond, const Quaternion & object_rotation);
		void calculateDesiredDisplacement(float delta_time);
		void calculateTargetPosition(const Vector3 && current_position);

	private:
		META(Enable) MovementComponentRes _movement_res;

		float _move_speed_ratio{ 0.f };
		float _vertical_move_speed{ 0.f };
		float _jump_horizontal_speed_ratio{ 0.f };

		Vector3 _desired_displacement;
		Vector3 _desired_horizontal_move_direction;
		Vector3 _jump_initial_velocity;
		Vector3 _target_position;

		MovementState _motor_state{ MovementState::MOVING };
		JumpState  _jump_state{ JumpState::IDEL };

		ControllerType _controller_type{ ControllerType::NONE };
		Controller* _controller{ nullptr };

		META(Enable) bool _is_moving{ false };
	};
}