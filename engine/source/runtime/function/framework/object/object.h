#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/function/framework/component/component.h"
#include "runtime/resource/res_type/common/object.h"

#include <memory>

namespace Dao {

	class GObject:public std::enable_shared_from_this<GObject> {

	public:
		GObject(GObjectID id) :m_id(id) {}
		virtual ~GObject();

	protected:
		GObjectID m_id{ k_invalid_gobject_id };

	};
}