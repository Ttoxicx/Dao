#pragma once

#include "runtime/core/math/vector2.h"

#include <memory>

namespace Dao {

    class EditorUI;
    class DaoEngine;

    class DaoEditor {

        friend class EditorUI;

    public:
        DaoEditor();
        virtual ~DaoEditor();

        void initialize(DaoEngine* engine_runtime);
        void clear();

        void run();

    protected:
        std::shared_ptr<EditorUI> _editor_ui;
        DaoEngine* _engine_runtime{ nullptr };
    };
}
