#pragma once

#include "runtime/function/render/render_pass.h"
#include "runtime/function/render/passes/color_grading_pass.h"
#include "runtime/function/render/passes/combine_ui_pass.h"
#include "runtime/function/render/passes/fxaa_pass.h"
#include "runtime/function/render/passes/particle_pass.h"
#include "runtime/function/render/passes/tone_mapping_pass.h"
#include "runtime/function/render/passes/ui_pass.h"

namespace Dao {
	
	class RenderResourceBase;

	struct MainCameraPassInitInfo : RenderPassInitInfo {
		bool enable_fxaa;
	};

	class MainCameraPass :public RenderPass {
	public:
		enum LayoutType :uint8_t {
			layout_type_per_mesh = 0,
			layout_type_mesh_gloabel,
			layout_type_mesh_per_material,
			layout_type_skybox,
			layout_type_axis,
			layout_type_particle,
			layout_type_deferred_lighting,
			layout_type_count
		};

		enum RenderPipelineType :uint8_t {
			render_pipeline_type_mesh_gbuffer = 0,
			render_pipeline_type_deferred_lighting,
			render_pipeline_type_mesh_lighting,
			render_pipeline_type_skybox,
			render_pipeline_type_axis,
			render_pipeline_type_particle,
			render_pipeline_type_count
		};

		void initialize(const RenderPassInitInfo* init_info) override final;
		void preparePassData(std::shared_ptr<RenderResourceBase> render_resource) override final;
		void draw(
			ColorGradingPass& color_grading_pass,
			FXAAPass& fxaa_pass,
			ToneMappingPass& tone_mapping_pass,
			UIPass& ui_pass,
			CombineUIPass& combine_ui_pass,
			ParticlePass& particle_pass,
			uint32_t current_swapchain_image_index
		);
		void drawForward(
			ColorGradingPass& color_grading_pass,
			FXAAPass& fxaa_pass,
			ToneMappingPass& tone_mapping_pass,
			UIPass& ui_pass,
			CombineUIPass& combine_ui_pass,
			ParticlePass& particle_pass,
			uint32_t current_swapchain_image_index
		);

		RHIImageView* m_point_light_shadow_color_image_view;
		RHIImageView* m_directional_light_shadow_color_image_view;

		bool m_is_show_axis{ false };
		bool m_enable_fxaa{ false };
		size_t m_selected_axis{ 3 };
		MeshPerframeStorageBufferObject m_mesh_perframe_storage_buffer_object;
		AxisStorageBufferObject m_axis_storage_buffer_object;

		void updateAfterFramebufferRecreate();
		RHICommandBuffer* getRenderCommandBuffer();
		void setParticlePass(std::shared_ptr<ParticlePass> pass);

	private:
		void setupParticlePass();
		void setupAttachments();
		void setupRenderPass();
		void setupDescriptorSetLayout();
		void setupPipelines();
		void setupDescriptorSet();
		void setupFramebufferDescriptorSet();
		void setupSwapchainFramebuffers();
		
		void setupModelGlobalDescriptorSet();
		void setupSkyboxDescriptorSet();
		void setupAxisDescriptorSet();
		void setupParticleDescriptorSet();
		void setupGbufferLightingDescriptorSet();

		void drawMeshGbuffer();
		void drawDefferredLighting();
		void drawMeshLighting();
		void drawSkybox();
		void drawAxis();

	private:
		std::vector<RHIFramebuffer*> m_swapchain_framebuffers;
		std::vector<ParticlePass> m_particle_pass;
	};
}