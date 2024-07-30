#pragma once

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

	class RenderResource;
	class RenderCamera;

	class RenderScene {
	public:
		//light
		AmbientLight		m_ambient_light;
		PDirectionalLight	m_directional_light;
		PointLightList		m_point_light_list;
		//render entities
		std::vector<RenderEntity>	m_render_entities;
		//axis for editor
		std::optional<RenderEntity> m_render_axis;
		//visible objects(update per frame)
		std::vector<RenderMeshNode> m_directional_light_visible_mesh_nodes;
		std::vector<RenderMeshNode> m_point_lights_visible_mesh_nodes;
		std::vector<RenderMeshNode> m_main_camera_visible_mesh_nodes;
		RenderAxisNode				m_axis_node;

		void clear();

		void updateVisibleObjects(
			std::shared_ptr<RenderResource> render_resource,
			std::shared_ptr<RenderCamera> camera
		);

		void setVisibleNodesReference();

		GuidAllocator<GameObjectPartId>& getInstanceIdAllocator();
		GuidAllocator<MeshSourceDesc>& getMeshAssetIdAllocator();
		GuidAllocator<MaterialSourceDesc>& getMaterialAssetIdAllocator();

		void addInstanceIdToMap(uint32_t instance_id, GObjectID go_id);
		GObjectID getGObjectIDByMeshID(uint32_t mesh_id) const;
		void deleteEntityByGObjectID(GObjectID go_id);

		void clearForLevelReloading();

	private:
		GuidAllocator<GameObjectPartId>			_instance_id_allocator;
		GuidAllocator<MeshSourceDesc>			_mesh_asset_id_allocator;
		GuidAllocator<MaterialSourceDesc>		_material_asset_id_allocator;
		std::unordered_map<uint32_t, GObjectID> _mesh_object_id_map;

		void updateVisibleObjectsDirectionalLight(
			std::shared_ptr<RenderResource> render_resource,
			std::shared_ptr<RenderCamera> camera
		);
		void updateVisibleObjectsPointLight(std::shared_ptr<RenderResource> render_resource);
		void updateVisibleObjectsMainCamera(
			std::shared_ptr<RenderResource> render_resource,
			std::shared_ptr<RenderCamera> camera
		);
		void updateVisibleObjectsAxis(std::shared_ptr<RenderResource> render_resource);
		void updateVisibleObjectsParticle(std::shared_ptr<RenderResource> render_resource);
	};
}