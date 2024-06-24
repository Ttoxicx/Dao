#include "runtime/function/framework/object/object_id_allocator.h"

#include "core/base/macro.h"

namespace Dao {

	std::atomic<GObjectID> ObjectIDAllocator::m_next_id{ 0 };

	GObjectID ObjectIDAllocator::allocate() {
		std::atomic<GObjectID> new_object_id = m_next_id.load();
		m_next_id++;
		if (m_next_id >= k_invalid_gobject_id) {
			LOG_FATAL("GObjectID overflow");
		}
		return new_object_id;
	}
}