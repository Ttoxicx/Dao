#pragma once

#include "editor/editor_axis.h"

#include "runtime/function/framework/object/object.h"
#include "runtime/function/render/render_object.h"

#include <memory>

namespace Dao {

    class RenderCamera;
    class RenderEntity;

    enum class EditorAxisMode : int {
        TranslateMode = 0,
        RotateMode = 1,
        ScaleMode = 2,
        Default = 3
    };

    class EditorSceneManager {
    public:
        void initialize();
        void tick(float delta_time);

    public:
        size_t updateCursorOnAxis(Vector2 cursor_uv, Vector2 game_engine_window_size);
        void drawSelectedEntityAxis();
        std::weak_ptr<GObject> getSelectedGObject() const;
        RenderEntity* getAxisMeshByType(EditorAxisMode axis_mode);
        void onGObjectSelected(GObjectID selected_gobject_id);
        void onDeleteSelectedGObject();
        void moveEntity(
            float new_mouse_pos_x,
            float new_mouse_pos_y,
            float last_mouse_pos_x,
            float last_mouse_pos_y,
            Vector2 engine_window_pos,
            Vector2 engine_window_size,
            size_t cursor_on_axis,
            Matrix4x4 model_matrix
        );

        void setEditorCamera(std::shared_ptr<RenderCamera> camera) { _camera = camera; }
        void uploadAxisResource();
        size_t getGuidOfPickedMesh(const Vector2& picked_uv) const;

    public:
        std::shared_ptr<RenderCamera> getEditorCamera() { return _camera; };

        GObjectID getSelectedObjectID() { return _selected_gobject_id; };
        Matrix4x4 getSelectedObjectMatrix() { return _selected_object_matrix; }
        EditorAxisMode getEditorAxisMode() { return _axis_mode; }

        void setSelectedObjectID(GObjectID selected_gobject_id) { _selected_gobject_id = selected_gobject_id; };
        void setSelectedObjectMatrix(Matrix4x4 new_object_matrix) { _selected_object_matrix = new_object_matrix; }
        void setEditorAxisMode(EditorAxisMode new_axis_mode) { _axis_mode = new_axis_mode; }
    private:
        EditorTranslationAxis _translation_axis;
        EditorRotationAxis    _rotation_axis;
        EditorScaleAxis       _scale_aixs;

        GObjectID _selected_gobject_id{ k_invalid_gobject_id };
        Matrix4x4 _selected_object_matrix{ Matrix4x4::IDENTITY };

        EditorAxisMode _axis_mode{ EditorAxisMode::TranslateMode };
        std::shared_ptr<RenderCamera> _camera;

        size_t _selected_axis{ 3 };
        bool   _is_show_axis = true;
    };
}