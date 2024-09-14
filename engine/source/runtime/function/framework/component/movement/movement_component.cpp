#include "runtime/function/framework/component/movement/movement_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/character/character.h"
#include "runtime/function/controller/character_controller.h"
#include "runtime/function/framework/component/camera/camera_component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/physics/physics_scene.h"

namespace Dao {
    MovementComponent::~MovementComponent() {
        if (_controller_type == ControllerType::PHYSICS) {
            delete _controller;
            _controller = nullptr;
        }
    }

    void MovementComponent::postLoadResource(std::weak_ptr<GObject> parent_object) {
        m_parent_object = parent_object;
        if (_movement_res.m_controller_config.getTypeName() == "PhysicsControllerConfig") {
            _controller_type = ControllerType::PHYSICS;
            PhysicsControllerConfig* controller_config = static_cast<PhysicsControllerConfig*>(_movement_res.m_controller_config);
            _controller = new CharacterController(controller_config->m_capsule_shape);
        }
        else if (_movement_res.m_controller_config != nullptr) {
            _controller_type = ControllerType::INVALID;
            LOG_ERROR("invalid controller type,not able to move");
        }
        const TransformComponent* transform_component = parent_object.lock()->tryGetComponentConst(TransformComponent);
        _target_position = transform_component->getPosition();
    }

    void MovementComponent::getOffStuckDead() {

    }

    void MovementComponent::tick(float delta_time) {
        tickPlayerMovement(delta_time);
    }

    void MovementComponent::tickPlayerMovement(float delta_time) {
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
        TransformComponent* transform_component = m_parent_object.lock()->tryGetComponent(TransformComponent);
        Radian turn_angle_yaw = g_runtime_global_context.m_input_system->m_cursor_delta_yaw;
        uint64_t command = g_runtime_global_context.m_input_system->getInputCommand();
        if (command >= (uint64_t)InputKey::INVALID) {
            return;
        }

        calculateDesiredHorizontalMoveSpeed(command, delta_time);
        calculateDesiredVerticalMoveSpeed(command, delta_time);
        calculateDesiredMoveDirection(command, transform_component->getRotation());
        calculateDesiredDisplacement(delta_time);
        calculateTargetPosition(transform_component->getPosition());

        transform_component->setPosition(_target_position);
    }

    void MovementComponent::calculateDesiredHorizontalMoveSpeed(uint64_t command, float delta_time) {
        bool has_move_command = ((uint64_t)InputKey::KEY_W | (uint64_t)InputKey::KEY_A | (uint64_t)InputKey::KEY_S | (uint64_t)InputKey::KEY_D) & command;
        bool has_sprint_command = (uint64_t)InputKey::KEY_LEFT_SHIFT & command;

        bool  is_acceleration = false;
        float final_acceleration = _movement_res.m_move_acceleration;
        float min_speed_ratio = 0.f;
        float max_speed_ratio = 0.f;
        if (has_move_command) {
            is_acceleration = true;
            max_speed_ratio = _movement_res.m_max_move_speed_ratio;
            if (_move_speed_ratio >= _movement_res.m_max_move_speed_ratio) {
                final_acceleration = _movement_res.m_sprint_acceleration;
                is_acceleration = has_sprint_command;
                min_speed_ratio = _movement_res.m_max_move_speed_ratio;
                max_speed_ratio = _movement_res.m_max_sprint_speed_ratio;
            }
        }
        else {
            is_acceleration = false;
            min_speed_ratio = 0.f;
            max_speed_ratio = _movement_res.m_max_sprint_speed_ratio;
        }

        _move_speed_ratio += (is_acceleration ? 1.0f : -1.0f) * final_acceleration * delta_time;
        _move_speed_ratio = std::clamp(_move_speed_ratio, min_speed_ratio, max_speed_ratio);
    }

    void MovementComponent::calculateDesiredVerticalMoveSpeed(uint64_t command, float delta_time) {
        std::shared_ptr<PhysicsScene> physics_scene = g_runtime_global_context.m_world_manager->getCurrentActivePhysicsScene().lock();
        ASSERT(physics_scene);

        if (_movement_res.m_jump_height == 0.f) {
            return;
        }

        const float gravity = physics_scene->getGravity().length();

        if (_jump_state == JumpState::IDEL) {
            if ((uint64_t)InputKey::KEY_SPACE & command) {
                _jump_state = JumpState::RISING;
                _vertical_move_speed = Math::sqrt(_movement_res.m_jump_height * 2 * gravity);
                _jump_horizontal_speed_ratio = _move_speed_ratio;
            }
            else {
                _vertical_move_speed = 0.f;
            }
        }
        else if (_jump_state == JumpState::RISING || _jump_state == JumpState::FALLING) {
            _vertical_move_speed -= gravity * delta_time;
            if (_vertical_move_speed <= 0.f) {
                _jump_state = JumpState::FALLING;
            }
        }
    }

    void MovementComponent::calculateDesiredMoveDirection(uint64_t command, const Quaternion& object_rotation) {
        if (_jump_state == JumpState::IDEL) {
            Vector3 forward_dir = object_rotation * Vector3::NEGATIVE_UNIT_Y;
            Vector3 left_dir = object_rotation * Vector3::UNIT_X;

            if (command > 0) {
                _desired_horizontal_move_direction = Vector3::ZERO;
            }

            if ((uint64_t)InputKey::KEY_W & command) {
                _desired_horizontal_move_direction += forward_dir;
            }

            if ((uint64_t)InputKey::KEY_S & command) {
                _desired_horizontal_move_direction -= forward_dir;
            }

            if ((uint64_t)InputKey::KEY_A & command) {
                _desired_horizontal_move_direction += left_dir;
            }

            if ((uint64_t)InputKey::KEY_D & command) {
                _desired_horizontal_move_direction -= left_dir;
            }

            _desired_horizontal_move_direction.normalise();
        }
    }

    void MovementComponent::calculateDesiredDisplacement(float delta_time) {
        float horizontal_speed_ratio = _jump_state == JumpState::IDEL ? _move_speed_ratio : _jump_horizontal_speed_ratio;
        _desired_displacement = _desired_horizontal_move_direction * _movement_res.m_move_speed * horizontal_speed_ratio * delta_time + Vector3::UNIT_Z * _vertical_move_speed * delta_time;
    }

    void MovementComponent::calculateTargetPosition(const Vector3&& current_position) {
        Vector3 final_position;

        switch (_controller_type)
        {
        case ControllerType::NONE:
            final_position = current_position + _desired_displacement;
            break;
        case ControllerType::PHYSICS:
            final_position = _controller->move(current_position, _desired_displacement);
            break;
        default:
            final_position = current_position;
            break;
        }

        // hack: motor level simulating jump, character always above z-plane
        if (_jump_state == JumpState::FALLING && final_position.z + _desired_displacement.z <= 0.f) {
            final_position.z = 0.f;
            _jump_state = JumpState::IDEL;
        }

        _is_moving = (final_position - current_position).squaredLength() > 0.f;
        _target_position = final_position;
    }
}