#pragma once

#include "runtime/function/render/render_resource_base.h"
#include "runtime/function/render/render_type.h"
#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/render_common.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <cmath>

namespace Dao {

	class RHI;
	class RenderPassBase;
	class RenderCamera;

	struct IBLResource {
		RHIImage* m_brdfLUT_texture_image;
		RHIImageView* m_brdfLUT_texture_image_view;
		RHISampler* m_brdfLUT_texture_sampler;
		VmaAllocation m_brdfLUT_texture_image_allocation;

		RHIImage* m_irradiance_texture_image;
		RHIImageView* m_irradiance_texture_image_view;
		RHISampler* m_irradiance_texture_sampler;
		VmaAllocation m_irradiance_texture_image_allocation;

		RHIImage* m_specular_texture_image;
		RHIImageView* m_specular_texture_image_view;
		RHISampler* m_specular_texture_sampler;
		VmaAllocation m_specular_texture_image_allocation;
	};

	struct IBLResourceData {
		void* m_brdfLUT_texture_image_pixels;
		uint32_t m_brdfLUT_texture_image_width;
		uint32_t m_brdfLUT_texture_image_height;
		RHIFormat m_brdfLUT_texture_image_format;

		std::array<void*, 6> m_irradiance_texture_image_pxiels;
		uint32_t m_irradiance_texture_image_width;
		uint32_t m_irradiance_texture_image_hieght;
		RHIFormat m_irradiance_texture_image_format;

		std::array<void*, 6> m_specular_texture_image_pxiels;
		uint32_t m_specular_texture_image_width;
		uint32_t m_specular_texture_image_hieght;
		RHIFormat m_specular_texture_image_format;
	};

	struct ColorGradingResource {
		RHIImage* m_color_grading_LUT_texture_image;
		RHIImageView* m_color_grading_LUT_texture_image_view;
		VmaAllocation m_color_grading_LUT_texture_image_allocation;
	};

	struct ColorGradingResourceData {
		void* m_color_grading_LUT_texture_image_pixels;
		uint32_t m_color_grading_LUT_texture_image_width;
		uint32_t m_color_grading_LUT_texture_image_height;
		RHIFormat m_color_grading_LUT_texture_image_format;
	};

	struct StorageBuffer {
		//limits
		uint32_t m_min_uniform_buffer_offset_aligment{ 256 };
		uint32_t m_min_storage_buffer_offset_aligment{ 256 };
		uint32_t m_max_storage_buffer_rage{ 1 << 27 };
		uint32_t m_non_coherent_atom_size{ 256 };

		RHIBuffer* m_global_upload_ringbuffer;
		RHIDeviceMemory* m_global_upload_ringbuffer_memory;
		void* m_global_upload_ringbuffer_memory_pointer;
		std::vector<uint32_t> m_global_upload_ringbuffers_begin;
		std::vector<uint32_t> m_global_upload_ringbuffers_end;
		std::vector<uint32_t> m_global_upload_ringbuffers_size;

		RHIBuffer* m_global_null_descriptor_storage_buffer;
		RHIDeviceMemory* m_global_null_descriptor_storage_buffer_memory;

		//axis
		RHIBuffer* m_axis_inefficient_storage_buffer;
		RHIDeviceMemory* m_axis_inefficient_storage_buffer_memory;
		void* m_axis_inefficient_storage_buffer_memory_pointer;
	};

	struct GlobalRenderResource {
		IBLResource m_ibl_resource;
		ColorGradingResource m_color_grading_resource;
		StorageBuffer m_storage_buffer;
	};

	class RenderResource :public RenderResourceBase {
	public:
		void clear() override final;

		virtual void uploadGloabalRenderResource(
			std::shared_ptr<RHI> rhi,
			LevelResourceDesc level_resource_desc
		) override final;

		virtual void uploadGameObjectRenderResource(
			std::shared_ptr<RHI> rhi,
			RenderEntity render_entity,
			RenderMeshData mesh_data,
			RenderMaterialData material_data
		) override final;

		virtual void uploadGameObjectRenderResource(
			std::shared_ptr<RHI> rhi,
			RenderEntity render_entity,
			RenderMeshData mesh_data
		) override final;

		virtual void uploadGameObjectRenderResource(
			std::shared_ptr<RHI> rhi,
			RenderEntity render_entity,
			RenderMaterialData material_data
		) override final;

		virtual void updatePerFrameBuffer(
			std::shared_ptr<RenderScene> render_scene,
			std::shared_ptr<RenderCamera> camera
		) override final;

		VulkanMesh& getEntityMesh(RenderEntity entity);

		VulkanPBRMaterial& getEntityMaterial(RenderEntity entity);

		void resetRingBufferOffset(uint8_t current_frame_index);

		GlobalRenderResource									m_global_render_resource;

		//storage buffer objects
		MeshPerframeStorageBufferObject							m_mesh_perframe_storage_buffer_object;
		MeshPointLightShadowPerframeStorageBufferObject			m_mesh_point_light_shadow_perframe_storage_buffer_object;
		MeshDirectionalLightShadowPerframeStorageBufferObject	m_mesh_directional_light_shadow_perframe_storage_buffer_object;
		AxisStorageBufferObject									m_axis_storage_buffer_object;
		MeshInefficientPickPerframeStorageBufferObject			m_mesh_inefficient_pick_perframe_storage_buffer_object;
		ParticleBillboardPerframeStorageBufferObject			m_particle_billboard_perframe_storage_buffer_object;
		ParticleCollisionPerframeStorageBufferObject			m_particle_collision_perframe_storage_buffer_object;
		//cache mesh and material
		std::map<size_t, VulkanMesh>		m_vulkan_meshs;
		std::map<size_t, VulkanPBRMaterial> m_vulkan_pbr_material;

		RHIDescriptorSetLayout* const* m_mesh_descriptor_set_layout{ nullptr };
		RHIDescriptorSetLayout* const* m_material_descriptor_set_layout{ nullptr };

	private:
		void createAndMapStorageBuffer(std::shared_ptr<RHI> rhi);
		void createIBLSamplers(std::shared_ptr<RHI> rhi);
		void createIBLTextures(
			std::shared_ptr<RHI> rhi,
			std::array<std::shared_ptr<TextureData>, 6> irradiance_maps,
			std::array<std::shared_ptr<TextureData>, 6> specular_maps
		);
		VulkanMesh& getOrCreateVulkanMesh(
			std::shared_ptr<RHI> rhi, 
			RenderEntity entity, 
			RenderMeshData mesh_data
		);
		VulkanPBRMaterial& getOrCreateVulkanMaterial(
			std::shared_ptr<RHI> rhi, 
			RenderEntity entity, 
			RenderMaterialData material_data
		);
		void updateMeshData(
			std::shared_ptr<RHI> rhi,
			bool enable_vertex_blending,
			uint32_t index_buffer_size,
			void* index_buffer_data,
			uint32_t vertex_buffer_size,
			MeshVertexDataDefinition const* vertex_buffer_data,
			uint32_t joint_binding_buffer_size,
			MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
			VulkanMesh& mesh
		);
		void updateVertexBuffer(
			std::shared_ptr<RHI> rhi,
			bool enable_vertex_blending,
			uint32_t vertex_buffer_size,
			MeshVertexDataDefinition const* vertex_buffer_data,
			uint32_t joint_binding_buffer_size,
			MeshVertexBindingDataDefinition const* joint_binding_buffer_data,
			uint32_t index_buffer_size,
			uint16_t* index_buffer_data,
			VulkanMesh& mesh
		);
		void updateIndexBuffer(
			std::shared_ptr<RHI> rhi,
			uint32_t index_buffer_size,
			void* index_buffer_data,
			VulkanMesh& mesh
		);
		void updateTextureImageData(
			std::shared_ptr<RHI> rhi,
			const TextureDataToUpdate& texture_data
		);
	};
}