#pragma once

#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/function/render/render_light.h"
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_entity.h"
#include "runtime/function/render/render_object.h"
#include "runtime/function/render/render_light.h"
#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/framework/object/object_id_allocator.h"

#include <optional>
#include <vector>

namespace Dao {

	class RenderScene {
	public:
		//light
		AmbientLight		m_ambient_light;
		PDirectionalLight	m_directional_light;
		PointLight			m_point_light_list;
		//render entities
		std::vector<RenderEntity>	m_render_entities;
	};
}