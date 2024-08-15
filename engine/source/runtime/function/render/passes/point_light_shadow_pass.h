#pragma once

#include "runtime/function/render/render_pass.h"

namespace Dao {
	
	class RenderResourceBase;

	class PointLightShadowPass :public RenderPass {
	public:
		void initialize(const RenderPassInitInfo* init_info) override final;
		void postInitialize() override final;
		void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
		void draw() override final;

		void setPerMeshLayout(RHIDescriptorSetLayout* layout) { _per_mesh_layout = layout; }

	private:
		void setupAttachment();
		void setupRenderPass();
		void setupFramebuffer();
		void setupDescriptorSetLayout();
		void setupPipelines();
		void setupDescriptorSet();
		void drawModel();

	private:
		RHIDescriptorSetLayout* _per_mesh_layout;
		MeshPointLightShadowPerframeStorageBufferObject _mesh_point_light_shadow_perframe_storage_buffer_object;
	};
}