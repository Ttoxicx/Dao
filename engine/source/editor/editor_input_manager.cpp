#include "editor/editor_input_manager.h"

#include "editor/editor.h"
#include "editor/editor_global_context.h"
#include "editor/editor_scene_manager.h"

#include "runtime/engine.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

namespace Dao {
    void EditorInputManager::initialize() { registerInput(); }

    void EditorInputManager::tick(float delta_time) { processEditorCommand(); }

    void EditorInputManager::registerInput() {
        g_editor_global_context.m_window_system->registerOnResetFunc(std::bind(&EditorInputManager::onReset, this));
        g_editor_global_context.m_window_system->registerOnCursorPosFunc(
            std::bind(&EditorInputManager::onCursorPos, this, std::placeholders::_1, std::placeholders::_2));
        g_editor_global_context.m_window_system->registerOnCursorEnterFunc(
            std::bind(&EditorInputManager::onCursorEnter, this, std::placeholders::_1));
        g_editor_global_context.m_window_system->registerOnScrollFunc(
            std::bind(&EditorInputManager::onScroll, this, std::placeholders::_1, std::placeholders::_2));
        g_editor_global_context.m_window_system->registerOnMouseButtonFunc(
            std::bind(&EditorInputManager::onMouseButtonClicked, this, std::placeholders::_1, std::placeholders::_2));
        g_editor_global_context.m_window_system->registerOnWindowCloseFunc(
            std::bind(&EditorInputManager::onWindowClosed, this));
        g_editor_global_context.m_window_system->registerOnKeyFunc(
            std::bind(&EditorInputManager::onKey, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4)
        );
    }

    void EditorInputManager::updateCursorOnAxis(Vector2 cursor_uv) {
        if (g_editor_global_context.m_scene_manager->getEditorCamera()) {
            Vector2 window_size(_engine_window_size.x, _engine_window_size.y);
            _cursor_on_axis = g_editor_global_context.m_scene_manager->updateCursorOnAxis(cursor_uv, window_size);
        }
    }

    void EditorInputManager::processEditorCommand() {
        float           camera_speed = _camera_speed;
        std::shared_ptr editor_camera = g_editor_global_context.m_scene_manager->getEditorCamera();
        Quaternion      camera_rotate = editor_camera->rotation().inverse();
        Vector3         camera_relative_pos(0, 0, 0);

        if ((unsigned int)EditorCommand::camera_foward & _editor_command) {
            camera_relative_pos += camera_rotate * Vector3{ 0, camera_speed, 0 };
        }
        if ((unsigned int)EditorCommand::camera_back & _editor_command) {
            camera_relative_pos += camera_rotate * Vector3{ 0, -camera_speed, 0 };
        }
        if ((unsigned int)EditorCommand::camera_left & _editor_command) {
            camera_relative_pos += camera_rotate * Vector3{ -camera_speed, 0, 0 };
        }
        if ((unsigned int)EditorCommand::camera_right & _editor_command) {
            camera_relative_pos += camera_rotate * Vector3{ camera_speed, 0, 0 };
        }
        if ((unsigned int)EditorCommand::camera_up & _editor_command) {
            camera_relative_pos += Vector3{ 0, 0, camera_speed };
        }
        if ((unsigned int)EditorCommand::camera_down & _editor_command) {
            camera_relative_pos += Vector3{ 0, 0, -camera_speed };
        }
        if ((unsigned int)EditorCommand::delete_object & _editor_command) {
            g_editor_global_context.m_scene_manager->onDeleteSelectedGObject();
        }

        editor_camera->move(camera_relative_pos);
    }

    void EditorInputManager::onKeyInEditorMode(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            switch (key)
            {
            case GLFW_KEY_A:
                _editor_command |= (unsigned int)EditorCommand::camera_left;
                break;
            case GLFW_KEY_S:
                _editor_command |= (unsigned int)EditorCommand::camera_back;
                break;
            case GLFW_KEY_W:
                _editor_command |= (unsigned int)EditorCommand::camera_foward;
                break;
            case GLFW_KEY_D:
                _editor_command |= (unsigned int)EditorCommand::camera_right;
                break;
            case GLFW_KEY_Q:
                _editor_command |= (unsigned int)EditorCommand::camera_up;
                break;
            case GLFW_KEY_E:
                _editor_command |= (unsigned int)EditorCommand::camera_down;
                break;
            case GLFW_KEY_T:
                _editor_command |= (unsigned int)EditorCommand::translation_mode;
                break;
            case GLFW_KEY_R:
                _editor_command |= (unsigned int)EditorCommand::rotation_mode;
                break;
            case GLFW_KEY_C:
                _editor_command |= (unsigned int)EditorCommand::scale_mode;
                break;
            case GLFW_KEY_DELETE:
                _editor_command |= (unsigned int)EditorCommand::delete_object;
                break;
            default:
                break;
            }
        }
        else if (action == GLFW_RELEASE) {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::exit);
                break;
            case GLFW_KEY_A:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_left);
                break;
            case GLFW_KEY_S:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_back);
                break;
            case GLFW_KEY_W:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_foward);
                break;
            case GLFW_KEY_D:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_right);
                break;
            case GLFW_KEY_Q:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_up);
                break;
            case GLFW_KEY_E:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_down);
                break;
            case GLFW_KEY_T:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::translation_mode);
                break;
            case GLFW_KEY_R:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::rotation_mode);
                break;
            case GLFW_KEY_C:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::scale_mode);
                break;
            case GLFW_KEY_DELETE:
                _editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::delete_object);
                break;
            default:
                break;
            }
        }
    }

    void EditorInputManager::onKey(int key, int scancode, int action, int mods) {
        if (g_is_editor_mode) {
            onKeyInEditorMode(key, scancode, action, mods);
        }
    }

    void EditorInputManager::onReset() {
        // to do
    }

    void EditorInputManager::onCursorPos(double xpos, double ypos) {
        if (!g_is_editor_mode) {
            return;
        }

        float angularVelocity = 180.0f / Math::max(_engine_window_size.x, _engine_window_size.y); // 180 degrees while moving full screen
        if (_mouse_x >= 0.0f && _mouse_y >= 0.0f) {
            if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                glfwSetInputMode(g_editor_global_context.m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                g_editor_global_context.m_scene_manager->getEditorCamera()->rotate(Vector2(ypos - _mouse_y, xpos - _mouse_x) * angularVelocity);
            }
            else if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
                g_editor_global_context.m_scene_manager->moveEntity(
                    xpos, ypos, _mouse_x, _mouse_y,
                    _engine_window_pos, _engine_window_size, _cursor_on_axis,
                    g_editor_global_context.m_scene_manager->getSelectedObjectMatrix()
                );
                glfwSetInputMode(g_editor_global_context.m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else {
                glfwSetInputMode(g_editor_global_context.m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                if (isCursorInRect(_engine_window_pos, _engine_window_size)) {
                    Vector2 cursor_uv = Vector2((_mouse_x - _engine_window_pos.x) / _engine_window_size.x, (_mouse_y - _engine_window_pos.y) / _engine_window_size.y);
                    updateCursorOnAxis(cursor_uv);
                }
            }
        }
        _mouse_x = xpos;
        _mouse_y = ypos;
    }

    void EditorInputManager::onCursorEnter(int entered) {
        if (!entered) {
            _mouse_x = _mouse_y = -1.0f;
        }
    }

    void EditorInputManager::onScroll(double xoffset, double yoffset) {
        if (!g_is_editor_mode) {
            return;
        }

        if (isCursorInRect(_engine_window_pos, _engine_window_size)) {
            if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                if (yoffset > 0) {
                    _camera_speed *= 1.2f;
                }
                else {
                    _camera_speed *= 0.8f;
                }
            }
            else {
                g_editor_global_context.m_scene_manager->getEditorCamera()->zoom(
                    (float)yoffset * 2.0f); // wheel scrolled up = zoom in by 2 extra degrees
            }
        }
    }

    void EditorInputManager::onMouseButtonClicked(int key, int action) {
        if (!g_is_editor_mode) {
            return;
        }
        if (_cursor_on_axis != 3) {
            return;
        }

        std::shared_ptr<Level> current_active_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        if (current_active_level == nullptr) {
            return;
        }

        if (isCursorInRect(_engine_window_pos, _engine_window_size)) {
            if (key == GLFW_MOUSE_BUTTON_LEFT) {
                Vector2 picked_uv((_mouse_x - _engine_window_pos.x) / _engine_window_size.x, (_mouse_y - _engine_window_pos.y) / _engine_window_size.y);
                size_t  select_mesh_id = g_editor_global_context.m_scene_manager->getGuidOfPickedMesh(picked_uv);
                size_t gobject_id = g_editor_global_context.m_render_system->getGObjectIDByMeshID(select_mesh_id);
                g_editor_global_context.m_scene_manager->onGObjectSelected(gobject_id);
            }
        }
    }

    void EditorInputManager::onWindowClosed() { g_editor_global_context.m_engine_runtime->shutdownEngine(); }

    bool EditorInputManager::isCursorInRect(Vector2 pos, Vector2 size) const {
        return pos.x <= _mouse_x && _mouse_x <= pos.x + size.x && pos.y <= _mouse_y && _mouse_y <= pos.y + size.y;
    }
}