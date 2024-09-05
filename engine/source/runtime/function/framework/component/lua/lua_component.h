#pragma once

#include "runtime/function/framework/component/component.h"

#include "runtime/core/base/macro.h"

#include <sol/sol.hpp>

namespace Dao {

	REFLECTION_TYPE(LuaComponent);
	CLASS(LuaComponent:public Component, WhiteListFields)
	{
		REFLECTION_BODY(LuaComponent);
	public:
		LuaComponent() = default;

		void postLoadResource(std::weak_ptr<GObject> parent_object) override;

		void tick(float delta_time) override;

		template<typename T>
		static void set(std::weak_ptr<GObject> game_object, const char* name, T value) {
			LOG_INFO(name);
			Reflection::FieldAccessor field_accessor;
			void* target_instance;
			if (findComponentField(game_object, name, field_accessor, target_instance)) {
				field_accessor.set(target_instance, &value);
			}
			else {
				LOG_ERROR("Can't find target field.");
			}
		}

		template<typename T>
		static T get(std::weak_ptr<GObject> game_object, const char* name) {
			LOG_INFO(name);
			Reflection::FieldAccessor field_accessor;
			void* target_instance;
			if (findComponentField(game_object, name, field_accessor, target_instance)) {
				return *(T*)field_accessor.get(target_instance);
			}
			else {
				LOG_ERROR("Can't find target field.");
			}
		}

		static void invoke(std::weak_ptr<GObject> game_object, const char* name);

		static bool findComponentField(std::weak_ptr<GObject> game_object, const char* field_name, Reflection::FieldAccessor& field_accessor, void*& target_instance);

	protected:
		sol::state m_lua_state;
		META(Enable) std::string m_lua_script;
	};
}