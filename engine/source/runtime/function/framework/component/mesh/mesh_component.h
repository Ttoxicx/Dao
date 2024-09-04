#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/resource/res_type/components/mesh.h"
#include "runtime/function/render/render_object.h"

#include <vector>

namespace Dao {
	
	class RenderSwapContext;
	
	REFLECTION_TYPE(MeshComponent);
	CLASS(MeshComponent:public Component, WhiteListFields)
	{
		REFLECTION_BODY(MeshComponent);
	public:
		MeshComponent() = default;

		void postLoadResource(std::weak_ptr<GObject> parent_object) override;

		void tick(float delta_time) override;

		const std::vector<GameObjectPartDesc>& getRawMeshes() const { return _raw_meshes; }

	private:
		META(Enable) MeshComponentRes _mesh_res;
		std::vector<GameObjectPartDesc> _raw_meshes;
	};
}