#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_pass_base.h"
#include "runtime/function/render/render_resource.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Dao {

	class VulkanRHI;

	enum {
		main_camera_pass_gbuffer_a = 0,
		main_camera_pass_gbuffer_b = 1,
		main_camera_pass_gbuffer_c = 2,
		main_camera_pass_backup_buffer_odd = 3,
		main_camera_pass_backup_buffer_even = 4,
		main_camera_pass_post_process_buffer_odd = 5,
		main_camera_pass_post_process_buffer_even = 6,
		main_camera_pass_depth = 7,
		main_camera_pass_swap_chain_image = 8,
		main_camera_pass_custom_attachment_count = 5,
		main_camera_pass_post_process_attachment_count = 2,
		main_camera_pass_attachment_count = 9
	};

	enum {
		main_camera_subpass_basepass = 0,
		main_camera_subpass_deferred_lighting,
		main_camera_subpass_forward_lighting,
		main_camera_subpass_tone_mapping,
		main_camera_subpass_color_grading,
		main_camera_subpass_fxaa,
		main_camera_subpass_ui,
		main_camera_subpass_combine_ui,
		main_camera_subpass_count
	};

	struct VisibleNodes {
		std::vector<RenderMeshNode>* p_directional_light_visible_mesh_nodes{ nullptr };
		std::vector<RenderMeshNode>* p_point_lights_visible_mesh_nodes{ nullptr };
		std::vector<RenderMeshNode>* p_main_camera_visible_mesh_nodes{ nullptr };
		RenderAxisNode* p_axis_node{ nullptr };
	};

	class RenderPass :public RenderPassBase {
	public:
		struct FrameBufferAttachment {
			RHIImage*			image;
			RHIDeviceMemory*	memory;
			RHIImageView*		view;
			RHIFormat			format;
		};

		struct FrameBuffer {
			int				width;
			int				height;
			RHIFramebuffer* framebuffer;
			RHIRenderPass* render_pass;
			std::vector<FrameBufferAttachment> attachments;
		};

		struct Descriptor {
			RHIDescriptorSetLayout* layout;
			RHIDescriptorSet* descriptor_set;
		};

		struct RenderPipelineBase {
			RHIPipelineLayout* layout;
			RHIPipeline* pipeline;
		};

		GlobalRenderResource*			m_global_render_resource{ nullptr };

		std::vector<Descriptor>			m_descriptor_infos;
		std::vector<RenderPipelineBase>	m_render_pipelines;
		FrameBuffer						m_framebuffer;

		void initialize(const RenderPassInitInfo* init_info) override;
		void postInitialize() override;

		virtual void draw();

		virtual RHIRenderPass* getRenderPass() const;
		virtual std::vector<RHIImageView*>	getFrameBufferImageViews() const;
		virtual std::vector<RHIDescriptorSetLayout*> getDescriptorSetLayouts() const;

		static VisibleNodes m_visible_nodes;
	};

}