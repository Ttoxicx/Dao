#include "runtime/function/render/passes/color_grading_pass.h"

#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"
#include "runtime/core/base/macro.h"

#include <color_grading_frag.h>
#include <post_process_vert.h>

namespace Dao {
	void ColorGradingPass::initialize(const RenderPassInitInfo* init_info) {
		RenderPass::initialize(init_info);
		const ColorGradingPassInitInfo* info = static_cast<const ColorGradingPassInitInfo*>(init_info);
		m_framebuffer.render_pass = info->render_pass;

		setupDescriptorSetLayout();
		setupPipelines();
		setupDescriptorSet();
		updateAfterFramebufferRecreate(info->input_attachment);
	}

	void ColorGradingPass::setupDescriptorSetLayout() {
		m_descriptor_infos.resize(1);
		RHIDescriptorSetLayoutBinding post_process_global_layout_bindings[2] = {};

		RHIDescriptorSetLayoutBinding& post_process_global_layout_input_attachment_binding = post_process_global_layout_bindings[0];
		post_process_global_layout_input_attachment_binding.binding = 0;
		post_process_global_layout_input_attachment_binding.descriptorType = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		post_process_global_layout_input_attachment_binding.descriptorCount = 1;
		post_process_global_layout_input_attachment_binding.stageFlags = RHI_SHADER_STAGE_FRAGMENT_BIT;

		RHIDescriptorSetLayoutBinding& post_process_global_layout_LUT_binding = post_process_global_layout_bindings[1];
		post_process_global_layout_LUT_binding.binding = 1;
		post_process_global_layout_LUT_binding.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		post_process_global_layout_LUT_binding.descriptorCount = 1;
		post_process_global_layout_LUT_binding.stageFlags = RHI_SHADER_STAGE_FRAGMENT_BIT;

		RHIDescriptorSetLayoutCreateInfo post_process_global_layout_create_info;
		post_process_global_layout_create_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		post_process_global_layout_create_info.pNext = nullptr;
		post_process_global_layout_create_info.flags = 0;
		post_process_global_layout_create_info.bindingCount = sizeof(post_process_global_layout_bindings) / sizeof(post_process_global_layout_bindings[0]);
		post_process_global_layout_create_info.pBindings = post_process_global_layout_bindings;

		if (m_rhi->createDescriptorSetLayout(&post_process_global_layout_create_info, m_descriptor_infos[0].layout) != RHI_SUCCESS) {
			LOG_FATAL("failed to create post process global layout");
		}
	}

	void ColorGradingPass::setupPipelines() {
		m_render_pipelines.resize(1);

		RHIDescriptorSetLayout* descriptor_layouts[1] = { m_descriptor_infos[0].layout };
		RHIPipelineLayoutCreateInfo pipeline_layout_create_info{};
		pipeline_layout_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = descriptor_layouts;

		if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[0].layout) != RHI_SUCCESS) {
			LOG_FATAL("failed to create post process pipeline layout");
		}

		RHIShader* vert_shader_module = m_rhi->createShaderModule(POST_PROCESS_VERT);
		RHIShader* frag_shader_module = m_rhi->createShaderModule(COLOR_GRADING_FRAG);

		RHIPipelineShaderStageCreateInfo vert_pipeline_shader_stage_create_info{};
		vert_pipeline_shader_stage_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_pipeline_shader_stage_create_info.stage = RHI_SHADER_STAGE_VERTEX_BIT;
		vert_pipeline_shader_stage_create_info.module = vert_shader_module;
		vert_pipeline_shader_stage_create_info.pName = "main";

		RHIPipelineShaderStageCreateInfo frag_pipeline_shader_stage_create_info{};
		frag_pipeline_shader_stage_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_pipeline_shader_stage_create_info.stage = RHI_SHADER_STAGE_FRAGMENT_BIT;
		frag_pipeline_shader_stage_create_info.module = frag_shader_module;
		frag_pipeline_shader_stage_create_info.pName = "main";

		RHIPipelineShaderStageCreateInfo shader_stages[] = { vert_pipeline_shader_stage_create_info,frag_pipeline_shader_stage_create_info };
		
		RHIPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
		vertex_input_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
		vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
		vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
		vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;

		RHIPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
		input_assembly_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_create_info.topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		input_assembly_create_info.primitiveRestartEnable = RHI_FALSE;

		RHIPipelineViewportStateCreateInfo viewport_state_create_info{};
		viewport_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state_create_info.viewportCount = 1;
		viewport_state_create_info.pViewports = m_rhi->getSwapChainInfo().viewport;
		viewport_state_create_info.scissorCount = 1;
		viewport_state_create_info.pScissors = m_rhi->getSwapChainInfo().scissor;

		RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
		rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_state_create_info.depthClampEnable = RHI_FALSE;
		rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
		rasterization_state_create_info.polygonMode = RHI_POLYGON_MODE_FILL;
		rasterization_state_create_info.lineWidth = 1.0f;
		rasterization_state_create_info.cullMode = RHI_CULL_MODE_BACK_BIT;
		rasterization_state_create_info.frontFace = RHI_FRONT_FACE_CLOCKWISE;
		rasterization_state_create_info.depthBiasEnable = RHI_FALSE;
		rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_state_create_info.depthBiasClamp = 0.0f;
		rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

		RHIPipelineMultisampleStateCreateInfo multisample_state_create_info{};
		multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_state_create_info.sampleShadingEnable = RHI_FALSE;
		multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

		RHIPipelineColorBlendAttachmentState color_blend_attachment_state{};
		color_blend_attachment_state.colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT | RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
		color_blend_attachment_state.blendEnable = RHI_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
		color_blend_attachment_state.dstColorBlendFactor = RHI_BLEND_FACTOR_ZERO;
		color_blend_attachment_state.colorBlendOp = RHI_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
		color_blend_attachment_state.dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
		color_blend_attachment_state.alphaBlendOp = RHI_BLEND_OP_ADD;

		RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
		color_blend_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_state_create_info.logicOpEnable = RHI_FALSE;
		color_blend_state_create_info.logicOp = RHI_LOGIC_OP_COPY;
		color_blend_state_create_info.attachmentCount = 1;
		color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_state_create_info.blendConstants[0] = 0.0f;
		color_blend_state_create_info.blendConstants[1] = 0.0f;
		color_blend_state_create_info.blendConstants[2] = 0.0f;
		color_blend_state_create_info.blendConstants[3] = 0.0f;

		RHIPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
		depth_stencil_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_state_create_info.depthTestEnable = RHI_TRUE;
		depth_stencil_state_create_info.depthWriteEnable = RHI_TRUE;
		depth_stencil_state_create_info.depthCompareOp = RHI_COMPARE_OP_LESS;
		depth_stencil_state_create_info.depthBoundsTestEnable = RHI_FALSE;
		depth_stencil_state_create_info.stencilTestEnable = RHI_FALSE;

		RHIDynamicState dynamic_states[] = { RHI_DYNAMIC_STATE_VIEWPORT,RHI_DYNAMIC_STATE_SCISSOR };

		RHIPipelineDynamicStateCreateInfo dynamic_state_create_info{};
		dynamic_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_create_info.dynamicStateCount = 2;
		dynamic_state_create_info.pDynamicStates = dynamic_states;

		RHIGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = RHI_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_state_create_info;
		pipeline_info.pInputAssemblyState = &input_assembly_create_info;
		pipeline_info.pViewportState = &viewport_state_create_info;
		pipeline_info.pRasterizationState = &rasterization_state_create_info;
		pipeline_info.pMultisampleState = &multisample_state_create_info;
		pipeline_info.pColorBlendState = &color_blend_state_create_info;
		pipeline_info.pDepthStencilState = &depth_stencil_state_create_info;
		pipeline_info.layout = m_render_pipelines[0].layout;
		pipeline_info.renderPass = m_framebuffer.render_pass;
		pipeline_info.subpass = main_camera_subpass_color_grading;
		pipeline_info.basePipelineHandle = RHI_NULL_HANDLE;
		pipeline_info.pDynamicState = &dynamic_state_create_info;

		if (m_rhi->createGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipeline_info, m_render_pipelines[0].pipeline) != RHI_SUCCESS) {
			LOG_FATAL("failed to create post process graphics pipeline");
		}

		m_rhi->destroyShaderModule(vert_shader_module);
		m_rhi->destroyShaderModule(frag_shader_module);
	}

	void ColorGradingPass::setupDescriptorSet() {
		RHIDescriptorSetAllocateInfo post_process_global_descriptor_set_alloc_info;
		post_process_global_descriptor_set_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		post_process_global_descriptor_set_alloc_info.pNext = nullptr;
		post_process_global_descriptor_set_alloc_info.descriptorPool = m_rhi->getDescriptorPool();
		post_process_global_descriptor_set_alloc_info.descriptorSetCount = 1;
		post_process_global_descriptor_set_alloc_info.pSetLayouts = &m_descriptor_infos[0].layout;

		if (m_rhi->allocateDescriptorSets(&post_process_global_descriptor_set_alloc_info, m_descriptor_infos[0].descriptor_set) != RHI_SUCCESS) {
			LOG_FATAL("failed to allocate post process global descriptor set");
		}
	}

	void ColorGradingPass::updateAfterFramebufferRecreate(RHIImageView* input_attachment) {
		RHIDescriptorImageInfo post_process_per_frame_input_attachment_info = {};
		post_process_per_frame_input_attachment_info.sampler = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
		post_process_per_frame_input_attachment_info.imageView = input_attachment;
		post_process_per_frame_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		RHIDescriptorImageInfo color_grading_LUT_image_info = {};
		color_grading_LUT_image_info.sampler = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Linear);
		color_grading_LUT_image_info.imageView = m_global_render_resource->m_color_grading_resource.m_color_grading_LUT_texture_image_view;
		color_grading_LUT_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		RHIWriteDescriptorSet post_process_descriptor_write_info[2];

		RHIWriteDescriptorSet& post_process_descriptor_input_attachment_write_info = post_process_descriptor_write_info[0];
		post_process_descriptor_input_attachment_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		post_process_descriptor_input_attachment_write_info.pNext = nullptr;
		post_process_descriptor_input_attachment_write_info.dstSet = m_descriptor_infos[0].descriptor_set;
		post_process_descriptor_input_attachment_write_info.dstBinding = 0;
		post_process_descriptor_input_attachment_write_info.dstArrayElement = 0;
		post_process_descriptor_input_attachment_write_info.descriptorType = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		post_process_descriptor_input_attachment_write_info.descriptorCount = 1;
		post_process_descriptor_input_attachment_write_info.pImageInfo = &post_process_per_frame_input_attachment_info;

		RHIWriteDescriptorSet& post_process_descriptor_LUT_write_info = post_process_descriptor_write_info[1];
		post_process_descriptor_LUT_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		post_process_descriptor_LUT_write_info.pNext = nullptr;
		post_process_descriptor_LUT_write_info.dstSet = m_descriptor_infos[0].descriptor_set;
		post_process_descriptor_LUT_write_info.dstBinding = 1;
		post_process_descriptor_LUT_write_info.dstArrayElement = 0;
		post_process_descriptor_LUT_write_info.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		post_process_descriptor_LUT_write_info.descriptorCount = 1;
		post_process_descriptor_LUT_write_info.pImageInfo = &color_grading_LUT_image_info;

		m_rhi->updateDescriptorSets(
			sizeof(post_process_descriptor_write_info) / sizeof(post_process_descriptor_write_info[0]),
			post_process_descriptor_write_info, 0, nullptr
		);
	}

	void ColorGradingPass::draw() {
		float color[4] = { 1.0f,1.0f,1.0f,1.0f };
		m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "Color Grading", color);
		m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, m_render_pipelines[0].pipeline);
		m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapChainInfo().viewport);
		m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapChainInfo().scissor);
		m_rhi->cmdBindDescriptorSetsPFN(
			m_rhi->getCurrentCommandBuffer(),
			RHI_PIPELINE_BIND_POINT_GRAPHICS,
			m_render_pipelines[0].layout,
			0, 1,
			&m_descriptor_infos[0].descriptor_set,
			0, nullptr
		);
		m_rhi->cmdDraw(m_rhi->getCurrentCommandBuffer(), 3, 1, 0, 0);
		m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
	}
}