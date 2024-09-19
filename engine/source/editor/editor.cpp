#include "editor/editor.h"


#include "editor/editor_global_context.h"
#include "editor/editor_input_manager.h"
#include "editor/editor_scene_manager.h"
#include "editor/editor_ui.h"

#include "runtime/engine.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"

namespace Dao
{
    void registerEdtorTickComponent(std::string component_type_name) {
        g_editor_tick_component_types.insert(component_type_name);
    }

    DaoEditor::DaoEditor() {
        registerEdtorTickComponent("TransformComponent");
        registerEdtorTickComponent("MeshComponent");
    }

    DaoEditor::~DaoEditor() {}

    void DaoEditor::initialize(DaoEngine* engine_runtime) {
        ASSERT(engine_runtime);

        g_is_editor_mode = true;
        _engine_runtime = engine_runtime;

        EditorGlobalContextInitInfo init_info = {
            g_runtime_global_context.m_window_system.get(),
            g_runtime_global_context.m_render_system.get(),
            engine_runtime
        };
        g_editor_global_context.initialize(init_info);
        g_editor_global_context.m_scene_manager->setEditorCamera(g_runtime_global_context.m_render_system->getRenderCamera());
        g_editor_global_context.m_scene_manager->uploadAxisResource();

        _editor_ui = std::make_shared<EditorUI>();
        WindowUIInitInfo ui_init_info = {
            g_runtime_global_context.m_window_system,
            g_runtime_global_context.m_render_system
        };
        _editor_ui->initialize(ui_init_info);
    }

    void DaoEditor::clear() { g_editor_global_context.clear(); }

    void DaoEditor::run() {
        ASSERT(_engine_runtime);
        ASSERT(_editor_ui);
        float delta_time;
        while (true) {
            delta_time = _engine_runtime->calculateDeltaTime();
            g_editor_global_context.m_scene_manager->tick(delta_time);
            g_editor_global_context.m_input_manager->tick(delta_time);
            if (!_engine_runtime->tickOneFrame(delta_time)) {
                return;
            }
        }
    }
}
