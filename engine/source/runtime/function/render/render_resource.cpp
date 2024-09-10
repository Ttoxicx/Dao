#include "runtime/function/render/render_resource.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_helper.h"
#include "runtime/function/render/render_mesh.h"
#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"
#include "runtime/core/base/macro.h"

#include <stdexcept>

namespace Dao {

	void RenderResource::clear() {}

	void RenderResource::uploadGloabalRenderResource(std::shared_ptr<RHI> rhi, LevelResourceDesc level_resource_desc) {
		// create and map global storage buffer
		createAndMapStorageBuffer(rhi);

		//sky box irradiance
		SkyBoxIrradianceMap skybox_irradiance_map = level_resource_desc.m_ibl_resource_desc.m_skybox_irradiance_map;
		std::shared_ptr<TextureData> irradiance_pos_x_map = loadTextureHDR(skybox_irradiance_map.m_positive_x_map);
		std::shared_ptr<TextureData> irradiance_neg_x_map = loadTextureHDR(skybox_irradiance_map.m_negative_x_map);
		std::shared_ptr<TextureData> irradiance_pos_y_map = loadTextureHDR(skybox_irradiance_map.m_positive_y_map);
		std::shared_ptr<TextureData> irradiance_neg_y_map = loadTextureHDR(skybox_irradiance_map.m_negative_y_map);
		std::shared_ptr<TextureData> irradiance_pos_z_map = loadTextureHDR(skybox_irradiance_map.m_positive_z_map);
		std::shared_ptr<TextureData> irradiance_neg_z_map = loadTextureHDR(skybox_irradiance_map.m_negative_z_map);

		//sky box specular
		SkyBoxSpecularMap sky_box_specular_map = level_resource_desc.m_ibl_resource_desc.m_skybox_specular_map;
		std::shared_ptr<TextureData> specular_pos_x_map = loadTextureHDR(sky_box_specular_map.m_positive_x_map);
		std::shared_ptr<TextureData> specular_neg_x_map = loadTextureHDR(sky_box_specular_map.m_negative_x_map);
		std::shared_ptr<TextureData> specular_pos_y_map = loadTextureHDR(sky_box_specular_map.m_positive_y_map);
		std::shared_ptr<TextureData> specular_neg_y_map = loadTextureHDR(sky_box_specular_map.m_negative_y_map);
		std::shared_ptr<TextureData> specular_pos_z_map = loadTextureHDR(sky_box_specular_map.m_positive_z_map);
		std::shared_ptr<TextureData> specular_neg_z_map = loadTextureHDR(sky_box_specular_map.m_negative_z_map);

		//brdf
		std::shared_ptr<TextureData> brdf_map = loadTextureHDR(level_resource_desc.m_ibl_resource_desc.m_brdf_map);

		//create IBL samplers
		createIBLSamplers(rhi);

		//create IBL textures, order required
		std::array<std::shared_ptr<TextureData>, 6> irradiance_maps = {
			irradiance_pos_x_map,
			irradiance_neg_x_map,
			irradiance_pos_z_map,
			irradiance_neg_z_map,
			irradiance_pos_y_map,
			irradiance_neg_y_map
		};
		std::array<std::shared_ptr<TextureData>, 6> specular_maps = {
			specular_pos_x_map,
			specular_neg_x_map,
			specular_pos_z_map,
			specular_neg_z_map,
			specular_pos_y_map,
			specular_neg_y_map
		};
		createIBLTextures(rhi, irradiance_maps, specular_maps);

		//create brdf lut texture
		rhi->createGlobalImage(
			m_global_render_resource.m_ibl_resource.m_brdfLUT_texture_image,
			m_global_render_resource.m_ibl_resource.m_brdfLUT_texture_image_view,
			m_global_render_resource.m_ibl_resource.m_brdfLUT_texture_image_allocation,
			brdf_map->m_width, brdf_map->m_height, brdf_map->m_pixels, brdf_map->m_format
		);

		//color grading
		std::shared_ptr<TextureData> color_grading_map = loadTexture(level_resource_desc.m_color_grading_resource_desc.m_color_grading_map);

		//create color grading texture
		rhi->createGlobalImage(
			m_global_render_resource.m_color_grading_resource.m_color_grading_LUT_texture_image,
			m_global_render_resource.m_color_grading_resource.m_color_grading_LUT_texture_image_view,
			m_global_render_resource.m_color_grading_resource.m_color_grading_LUT_texture_image_allocation,
			color_grading_map->m_width, color_grading_map->m_height, color_grading_map->m_pixels, color_grading_map->m_format
		);
	}

	void RenderResource::uploadGameObjectRenderResource(std::shared_ptr<RHI> rhi, RenderEntity render_entity, RenderMeshData mesh_data, RenderMaterialData material_data) {
		getOrCreateVulkanMesh(rhi, render_entity, mesh_data);
		getOrCreateVulkanMaterial(rhi, render_entity, material_data);
	}

	void RenderResource::uploadGameObjectRenderResource(std::shared_ptr<RHI> rhi, RenderEntity render_entity, RenderMeshData mesh_data) {
		getOrCreateVulkanMesh(rhi, render_entity, mesh_data);
	}

	void RenderResource::uploadGameObjectRenderResource(std::shared_ptr<RHI> rhi, RenderEntity render_entity, RenderMaterialData material_data) {
		getOrCreateVulkanMaterial(rhi, render_entity, material_data);
	}

	void RenderResource::updatePerFrameBuffer(std::shared_ptr<RenderScene> render_scene, std::shared_ptr<RenderCamera> camera) {
		Matrix4x4 view_matrix = camera->getViewMatrix();
		Matrix4x4 proj_matrix = camera->getPersProjMatrix();
		Vector3	camera_position = camera->position();
		Matrix4x4 proj_view_matrix = proj_matrix * view_matrix;

		//ambient light
		Vector3 ambient_light = render_scene->m_ambient_light.m_irradiance;
		uint32_t point_light_num = static_cast<uint32_t>(render_scene->m_point_light_list.m_lights.size());

		//set ubo data
		m_particle_collision_perframe_storage_buffer_object.view_matrix = view_matrix;
		m_particle_collision_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
		m_particle_collision_perframe_storage_buffer_object.proj_inv_matrix = proj_matrix.inverse();
		m_mesh_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
		m_mesh_perframe_storage_buffer_object.camera_position = camera_position;
		m_mesh_perframe_storage_buffer_object.ambient_light = ambient_light;
		m_mesh_perframe_storage_buffer_object.point_light_num = point_light_num;
		m_mesh_point_light_shadow_perframe_storage_buffer_object.point_light_num = point_light_num;
		//point lights
		for (uint32_t i = 0; i < point_light_num; ++i) {
			Vector3 point_light_position = render_scene->m_point_light_list.m_lights[i].m_position;
			Vector3 point_light_intensity = render_scene->m_point_light_list.m_lights[i].m_flux / (4.0f * Math_PI);
			float radius = render_scene->m_point_light_list.m_lights[i].calculateRadius();
			m_mesh_perframe_storage_buffer_object.scene_point_lights[i].position = point_light_position;
			m_mesh_perframe_storage_buffer_object.scene_point_lights[i].radius = radius;
			m_mesh_perframe_storage_buffer_object.scene_point_lights[i].intensity = point_light_intensity;
			m_mesh_point_light_shadow_perframe_storage_buffer_object.point_lights_position_and_radius[i] = Vector4(point_light_position, radius);
		}
		//directional light
		m_mesh_perframe_storage_buffer_object.scene_direction_light.direction = render_scene->m_directional_light.m_direction.normalisedCopy();
		m_mesh_perframe_storage_buffer_object.scene_direction_light.color = render_scene->m_directional_light.m_color;
		//pick pass view projection matrix
		m_mesh_inefficient_pick_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
		m_particle_billboard_perframe_storage_buffer_object.proj_view_matrix = proj_view_matrix;
		m_particle_billboard_perframe_storage_buffer_object.right_direction = camera->right();
		m_particle_billboard_perframe_storage_buffer_object.forward_direction = camera->forward();
		m_particle_billboard_perframe_storage_buffer_object.up_direction = camera->up();
	}

	void RenderResource::createIBLSamplers(std::shared_ptr<RHI> rhi) {
		VulkanRHI* raw_rhi = static_cast<VulkanRHI*>(rhi.get());
		RHIPhysicalDeviceProperties physical_device_properties{};
		rhi->getPhysicalDeviceProperties(&physical_device_properties);

		RHISamplerCreateInfo sampler_info{};
		sampler_info.sType = RHI_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = RHI_FILTER_LINEAR;
		sampler_info.minFilter = RHI_FILTER_LINEAR;
		sampler_info.addressModeU = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeV = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeW = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.anisotropyEnable = RHI_TRUE;
		sampler_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
		sampler_info.borderColor = RHI_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = RHI_FALSE;
		sampler_info.compareEnable = RHI_FALSE;
		sampler_info.compareOp = RHI_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = RHI_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.maxLod = 0.0f;

		if (m_global_render_resource.m_ibl_resource.m_brdfLUT_texture_sampler != RHI_NULL_HANDLE) {
			rhi->destroySampler(m_global_render_resource.m_ibl_resource.m_brdfLUT_texture_sampler);
		}
		if (rhi->createSampler(&sampler_info, m_global_render_resource.m_ibl_resource.m_brdfLUT_texture_sampler) != RHI_SUCCESS) {
			LOG_FATAL("create ibl brdflut sampler failed");
		}

		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = 8.0f;// TODO: irradiance_texture_miplevels
		sampler_info.mipLodBias = 0.0f;

		if (m_global_render_resource.m_ibl_resource.m_irradiance_texture_sampler != RHI_NULL_HANDLE) {
			rhi->destroySampler(m_global_render_resource.m_ibl_resource.m_irradiance_texture_sampler);
		}
		if (rhi->createSampler(&sampler_info, m_global_render_resource.m_ibl_resource.m_irradiance_texture_sampler) != RHI_SUCCESS) {
			LOG_FATAL("create ibl irradiance sampler failed");
		}

		if (m_global_render_resource.m_ibl_resource.m_specular_texture_sampler != RHI_NULL_HANDLE) {
			rhi->destroySampler(m_global_render_resource.m_ibl_resource.m_specular_texture_sampler);
		}
		if (rhi->createSampler(&sampler_info, m_global_render_resource.m_ibl_resource.m_specular_texture_sampler) != RHI_SUCCESS) {
			LOG_FATAL("create ibl specular sampler failed");
		}
	}

	void RenderResource::createIBLTextures(std::shared_ptr<RHI> rhi, std::array<std::shared_ptr<TextureData>, 6> irradiance_maps, std::array<std::shared_ptr<TextureData>, 6> specular_maps) {
		//assume all textures have same width, height and format
		uint32_t irradiance_cubemap_miplevels = static_cast<uint32_t>(std::floor(log2(std::max(irradiance_maps[0]->m_width, irradiance_maps[0]->m_height)))) + 1;
		rhi->createCubeMap(
			m_global_render_resource.m_ibl_resource.m_irradiance_texture_image,
			m_global_render_resource.m_ibl_resource.m_irradiance_texture_image_view,
			m_global_render_resource.m_ibl_resource.m_irradiance_texture_image_allocation,
			irradiance_maps[0]->m_width,
			irradiance_maps[0]->m_height,
			{ irradiance_maps[0]->m_pixels,
			 irradiance_maps[1]->m_pixels,
			 irradiance_maps[2]->m_pixels,
			 irradiance_maps[3]->m_pixels,
			 irradiance_maps[4]->m_pixels,
			 irradiance_maps[5]->m_pixels },
			irradiance_maps[0]->m_format,
			irradiance_cubemap_miplevels
		);
		uint32_t specular_cubemap_miplevels = static_cast<uint32_t>(std::floor(log2(std::max(specular_maps[0]->m_width, specular_maps[0]->m_height)))) + 1;
		rhi->createCubeMap(
			m_global_render_resource.m_ibl_resource.m_specular_texture_image,
			m_global_render_resource.m_ibl_resource.m_specular_texture_image_view,
			m_global_render_resource.m_ibl_resource.m_specular_texture_image_allocation,
			specular_maps[0]->m_width,
			specular_maps[0]->m_height,
			{ specular_maps[0]->m_pixels,
			  specular_maps[1]->m_pixels,
			  specular_maps[2]->m_pixels,
			  specular_maps[3]->m_pixels,
			  specular_maps[4]->m_pixels,
			  specular_maps[5]->m_pixels },
			specular_maps[0]->m_format,
			specular_cubemap_miplevels
		);
	}

	VulkanMesh& RenderResource::getOrCreateVulkanMesh(std::shared_ptr<RHI> rhi, RenderEntity entity, RenderMeshData mesh_data) {
		size_t assetid = entity.m_mesh_asset_id;
		auto it = m_vulkan_meshs.find(assetid);
		if (it != m_vulkan_meshs.end()) {
			return it->second;
		}
		else {
			VulkanMesh temp;
			auto res = m_vulkan_meshs.insert(std::make_pair(assetid, std::move(temp)));
			ASSERT(res.second);

			uint32_t index_buffer_size = static_cast<uint32_t>(mesh_data.m_static_mesh_data.m_index_buffer->m_size);
			void* index_buffer_data = mesh_data.m_static_mesh_data.m_index_buffer->m_data;

			uint32_t vertex_buffer_size = static_cast<uint32_t>(mesh_data.m_static_mesh_data.m_vertex_buffer->m_size);
			MeshVertexDataDefinition* vertex_buffer_data = reinterpret_cast<MeshVertexDataDefinition*>(mesh_data.m_static_mesh_data.m_vertex_buffer->m_data);

			VulkanMesh& mesh = res.first->second;
			if (mesh_data.m_skeleton_binding_buffer) {
				uint32_t joint_binding_buffer_size = (uint32_t)mesh_data.m_skeleton_binding_buffer->m_size;
				MeshVertexBindingDataDefinition* joint_binding_buffer_data = reinterpret_cast<MeshVertexBindingDataDefinition*>(mesh_data.m_skeleton_binding_buffer->m_data);
				updateMeshData(rhi, true, index_buffer_size, index_buffer_data, vertex_buffer_size, vertex_buffer_data, joint_binding_buffer_size, joint_binding_buffer_data, mesh);
			}
			else {
				updateMeshData(rhi, false, index_buffer_size, index_buffer_data, vertex_buffer_size, vertex_buffer_data, 0, nullptr, mesh);
			}
			return mesh;
		}
	}

	VulkanPBRMaterial& RenderResource::getOrCreateVulkanMaterial(std::shared_ptr<RHI> rhi, RenderEntity entity, RenderMaterialData material_data) {
		VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());
		size_t assetid = entity.m_material_asset_id;

		auto it = m_vulkan_pbr_material.find(assetid);
		if (it != m_vulkan_pbr_material.end()) {
			return it->second;
		}
		else {
			VulkanPBRMaterial temp;
			auto res = m_vulkan_pbr_material.insert(std::make_pair(assetid, std::move(temp)));
			ASSERT(res.second);

			float empty_image[] = { 0.5f,0.5f,0.5f,0.5f };

			void* base_color_image_pixels = empty_image;
			uint32_t base_color_image_width = 1;
			uint32_t base_color_image_height = 1;
			RHIFormat base_color_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_SRGB;
			if (material_data.m_base_color_texture) {
				base_color_image_pixels = material_data.m_base_color_texture->m_pixels;
				base_color_image_width = static_cast<uint32_t>(material_data.m_base_color_texture->m_width);
				base_color_image_height = static_cast<uint32_t>(material_data.m_base_color_texture->m_height);
				base_color_image_format = material_data.m_base_color_texture->m_format;
			}

			void* metallic_roughness_image_pixels = empty_image;
			uint32_t metallic_roughness_image_width = 1;
			uint32_t metallic_roughness_image_height = 1;
			RHIFormat metallic_roughness_image_format = RHI_FORMAT_R8G8B8A8_UNORM;
			if (material_data.m_metallic_roughness_texture) {
				metallic_roughness_image_pixels = material_data.m_metallic_roughness_texture->m_pixels;
				metallic_roughness_image_width = material_data.m_metallic_roughness_texture->m_width;
				metallic_roughness_image_height = material_data.m_metallic_roughness_texture->m_height;
				metallic_roughness_image_format = material_data.m_metallic_roughness_texture->m_format;
			}

			void* normal_image_pixels = empty_image;
			uint32_t normal_image_width = 1;
			uint32_t normal_image_height = 1;
			RHIFormat normal_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
			if (material_data.m_normal_texture) {
				normal_image_pixels = material_data.m_normal_texture->m_pixels;
				normal_image_width = static_cast<uint32_t>(material_data.m_normal_texture->m_width);
				normal_image_height = static_cast<uint32_t>(material_data.m_normal_texture->m_height);
				normal_image_format = material_data.m_normal_texture->m_format;
			}

			void* occlusion_image_pixels = empty_image;
			uint32_t occlusion_image_width = 1;
			uint32_t occlusion_image_height = 1;
			RHIFormat occlusion_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
			if (material_data.m_occlusion_texture) {
				occlusion_image_pixels = material_data.m_occlusion_texture->m_pixels;
				occlusion_image_width = static_cast<uint32_t>(material_data.m_occlusion_texture->m_width);
				occlusion_image_height = static_cast<uint32_t>(material_data.m_occlusion_texture->m_height);
				occlusion_image_format = material_data.m_occlusion_texture->m_format;
			}

			void* emissive_image_pixels = empty_image;
			uint32_t emissive_image_width = 1;
			uint32_t emissive_image_height = 1;
			RHIFormat emissive_image_format = RHIFormat::RHI_FORMAT_R8G8B8A8_UNORM;
			if (material_data.m_emissive_texture) {
				emissive_image_pixels = material_data.m_emissive_texture->m_pixels;
				emissive_image_width = static_cast<uint32_t>(material_data.m_emissive_texture->m_width);
				emissive_image_height = static_cast<uint32_t>(material_data.m_emissive_texture->m_height);
				emissive_image_format = material_data.m_emissive_texture->m_format;
			}

			VulkanPBRMaterial& material = res.first->second;
			//similiarly to the vertex/index buffer, we should allocate the uniform
			//buffer in DEVICE_LOCAL memory and use the temp stage buffer to copy the
			//data
			{
				RHIDeviceSize buffer_size = sizeof(MeshPerMaterialUniformBufferObject);
				RHIBuffer* inefficient_staging_buffer = RHI_NULL_HANDLE;
				RHIDeviceMemory* inefficient_staging_buffer_memory = RHI_NULL_HANDLE;
				rhi->createBuffer(
					buffer_size, RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
					RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					inefficient_staging_buffer, inefficient_staging_buffer_memory
				);

				void* staging_buffer_data = nullptr;
				rhi->mapMemory(inefficient_staging_buffer_memory, 0, buffer_size, 0, &staging_buffer_data);
				MeshPerMaterialUniformBufferObject& material_uniform_buffer_info = (*static_cast<MeshPerMaterialUniformBufferObject*>(staging_buffer_data));
				material_uniform_buffer_info.is_blend = entity.m_blend;
				material_uniform_buffer_info.is_double_sided = entity.m_double_sided;
				material_uniform_buffer_info.baseColorFractor = entity.m_base_color_factor;
				material_uniform_buffer_info.metallicFactor = entity.m_metallic_factor;
				material_uniform_buffer_info.roughnessFactor = entity.m_roughness_factor;
				material_uniform_buffer_info.normalScale = entity.m_normal_scale;
				material_uniform_buffer_info.occlusionStrength = entity.m_occlusion_strength;
				material_uniform_buffer_info.emessiveFactor = entity.m_emissive_factor;
				rhi->unmapMemory(inefficient_staging_buffer_memory);

				RHIBufferCreateInfo buffer_info = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				buffer_info.size = buffer_size;
				buffer_info.usage = RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
				VmaAllocationCreateInfo alloc_info = {};
				alloc_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
				rhi->createBufferWithAlignmentVMA(
					vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
					m_global_render_resource.m_storage_buffer.m_min_uniform_buffer_offset_alignment,
					material.material_uniform_buffer, &material.material_uniform_buffer_allocation, nullptr
				);

				rhi->copyBuffer(inefficient_staging_buffer, material.material_uniform_buffer, 0, 0, buffer_size);
				rhi->destroyBuffer(inefficient_staging_buffer);
				rhi->freeMemory(inefficient_staging_buffer_memory);
			}

			TextureDataToUpdate update_texture_data;

			update_texture_data.base_color_image_pixels = base_color_image_pixels;
			update_texture_data.base_color_image_width = base_color_image_width;
			update_texture_data.base_color_image_height = base_color_image_height;
			update_texture_data.base_color_image_format = base_color_image_format;

			update_texture_data.metallic_roughness_image_pixels = metallic_roughness_image_pixels;
			update_texture_data.metallic_roughness_image_width = metallic_roughness_image_width;
			update_texture_data.metallic_roughness_image_height = metallic_roughness_image_height;
			update_texture_data.metallic_roughness_image_format = metallic_roughness_image_format;

			update_texture_data.normal_image_pixels = normal_image_pixels;
			update_texture_data.normal_image_width = normal_image_width;
			update_texture_data.normal_image_height = normal_image_height;
			update_texture_data.normal_image_format = normal_image_format;

			update_texture_data.occlusion_image_pixels = occlusion_image_pixels;
			update_texture_data.occlusion_image_width = occlusion_image_width;
			update_texture_data.occlusion_image_height = occlusion_image_height;
			update_texture_data.occlusion_image_format = occlusion_image_format;

			update_texture_data.emissive_image_pixels = emissive_image_pixels;
			update_texture_data.emissive_image_width = emissive_image_width;
			update_texture_data.emissive_image_height = emissive_image_height;
			update_texture_data.emissive_image_format = emissive_image_format;

			update_texture_data.now_material = &material;

			updateTextureImageData(rhi, update_texture_data);
			RHIDescriptorSetAllocateInfo material_descriptor_set_alloc_info;
			material_descriptor_set_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			material_descriptor_set_alloc_info.pNext = nullptr;
			material_descriptor_set_alloc_info.descriptorPool = vulkan_context->m_descriptor_pool;
			material_descriptor_set_alloc_info.descriptorSetCount = 1;
			material_descriptor_set_alloc_info.pSetLayouts = m_material_descriptor_set_layout;

			bool success = rhi->allocateDescriptorSets(&material_descriptor_set_alloc_info, material.material_descriptor_set) == RHI_SUCCESS;
			if (!success) {
				LOG_FATAL("allocate material descriptor set failed");
			}

			RHIDescriptorBufferInfo material_uniform_buffer_info = {};
			material_uniform_buffer_info.offset = 0;
			material_uniform_buffer_info.range = sizeof(MeshPerMaterialUniformBufferObject);
			material_uniform_buffer_info.buffer = material.material_uniform_buffer;

			RHIDescriptorImageInfo base_color_image_info = {};
			base_color_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			base_color_image_info.imageView = material.base_color_image_view;
			base_color_image_info.sampler = rhi->getOrCreateMipmapSampler(base_color_image_width, base_color_image_height);

			RHIDescriptorImageInfo metallic_roughness_image_info = {};
			metallic_roughness_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			metallic_roughness_image_info.imageView = material.metallic_roughness_image_view;
			metallic_roughness_image_info.sampler = rhi->getOrCreateMipmapSampler(metallic_roughness_image_width, metallic_roughness_image_height);

			RHIDescriptorImageInfo normal_image_info = {};
			normal_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normal_image_info.imageView = material.normal_image_view;
			normal_image_info.sampler = rhi->getOrCreateMipmapSampler(normal_image_width, normal_image_height);

			RHIDescriptorImageInfo occlusion_image_info = {};
			occlusion_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			occlusion_image_info.imageView = material.occlusion_image_view;
			occlusion_image_info.sampler = rhi->getOrCreateMipmapSampler(occlusion_image_width, occlusion_image_height);

			RHIDescriptorImageInfo emissive_image_info = {};
			emissive_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			emissive_image_info.imageView = material.emissive_image_view;
			emissive_image_info.sampler = rhi->getOrCreateMipmapSampler(emissive_image_width, emissive_image_height);

			RHIWriteDescriptorSet mesh_descriptor_writes_info[6];

			mesh_descriptor_writes_info[0].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			mesh_descriptor_writes_info[0].pNext = nullptr;
			mesh_descriptor_writes_info[0].dstSet = material.material_descriptor_set;
			mesh_descriptor_writes_info[0].dstBinding = 0;
			mesh_descriptor_writes_info[0].dstArrayElement = 0;
			mesh_descriptor_writes_info[0].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			mesh_descriptor_writes_info[0].descriptorCount = 1;
			mesh_descriptor_writes_info[0].pBufferInfo = &material_uniform_buffer_info;

			mesh_descriptor_writes_info[1].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			mesh_descriptor_writes_info[1].pNext = nullptr;
			mesh_descriptor_writes_info[1].dstSet = material.material_descriptor_set;
			mesh_descriptor_writes_info[1].dstBinding = 1;
			mesh_descriptor_writes_info[1].dstArrayElement = 0;
			mesh_descriptor_writes_info[1].descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			mesh_descriptor_writes_info[1].descriptorCount = 1;
			mesh_descriptor_writes_info[1].pImageInfo = &base_color_image_info;

			mesh_descriptor_writes_info[2] = mesh_descriptor_writes_info[1];
			mesh_descriptor_writes_info[2].dstBinding = 2;
			mesh_descriptor_writes_info[2].pImageInfo = &metallic_roughness_image_info;

			mesh_descriptor_writes_info[3] = mesh_descriptor_writes_info[1];
			mesh_descriptor_writes_info[3].dstBinding = 3;
			mesh_descriptor_writes_info[3].pImageInfo = &normal_image_info;

			mesh_descriptor_writes_info[4] = mesh_descriptor_writes_info[1];
			mesh_descriptor_writes_info[4].dstBinding = 4;
			mesh_descriptor_writes_info[5].pImageInfo = &occlusion_image_info;

			mesh_descriptor_writes_info[5] = mesh_descriptor_writes_info[1];
			mesh_descriptor_writes_info[5].dstBinding = 5;
			mesh_descriptor_writes_info[5].pImageInfo = &emissive_image_info;

			rhi->updateDescriptorSets(6, mesh_descriptor_writes_info, 0, nullptr);

			return material;
		}
	}

	void RenderResource::updateMeshData(std::shared_ptr<RHI> rhi, bool enable_vertex_blending, uint32_t index_buffer_size, void* index_buffer_data, uint32_t vertex_buffer_size, MeshVertexDataDefinition const* vertex_buffer_data, uint32_t joint_binding_buffer_size, MeshVertexBindingDataDefinition const* joint_binding_buffer_data, VulkanMesh& mesh) {
		mesh.enable_vertex_blending = enable_vertex_blending;
		ASSERT((vertex_buffer_size % sizeof(MeshVertexDataDefinition)) == 0);
		mesh.mesh_vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
		updateVertexBuffer(
			rhi, enable_vertex_blending, vertex_buffer_size, vertex_buffer_data,
			joint_binding_buffer_size, joint_binding_buffer_data, index_buffer_size,
			reinterpret_cast<uint16_t*>(index_buffer_data), mesh
		);
		ASSERT((index_buffer_size % sizeof(uint16_t)) == 0);
		mesh.mesh_index_count = index_buffer_size / sizeof(uint16_t);
		updateIndexBuffer(rhi, index_buffer_size, index_buffer_data, mesh);
	}

	void RenderResource::updateVertexBuffer(std::shared_ptr<RHI> rhi, bool enable_vertex_blending, uint32_t vertex_buffer_size, MeshVertexDataDefinition const* vertex_buffer_data, uint32_t joint_binding_buffer_size, MeshVertexBindingDataDefinition const* joint_binding_buffer_data, uint32_t index_buffer_size, uint16_t* index_buffer_data, VulkanMesh& mesh) {
		VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());
		if (enable_vertex_blending) {
			ASSERT((vertex_buffer_size % sizeof(MeshVertexDataDefinition)) == 0);
			uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);
			ASSERT((index_buffer_size % sizeof(uint16_t)) == 0);
			uint32_t index_count = index_buffer_size / sizeof(uint16_t);

			RHIDeviceSize vertex_position_buffer_size = sizeof(MeshVertex::VulkanMeshVertexPosition) * vertex_count;
			RHIDeviceSize vertex_varying_enable_blending_buffer_size = sizeof(MeshVertex::VulkanMeshVertexVaryingEnableBlending) * vertex_count;
			RHIDeviceSize vertex_varying_buffer_size = sizeof(MeshVertex::VulkanMeshVertexVarying) * vertex_count;
			RHIDeviceSize vertex_joint_binding_buffer_size = sizeof(MeshVertex::VulkanMeshVertexJointBinding) * index_count;

			RHIDeviceSize vertex_position_buffer_offset = 0;
			RHIDeviceSize vertex_varying_enable_blending_buffer_offset = vertex_position_buffer_offset + vertex_position_buffer_size;
			RHIDeviceSize vertex_varying_buffer_offset = vertex_varying_enable_blending_buffer_offset + vertex_varying_enable_blending_buffer_size;
			RHIDeviceSize vertex_joint_binding_buffer_offset = vertex_varying_buffer_offset + vertex_varying_buffer_size;

			//temporary staging buffer
			RHIDeviceSize inefficient_staging_buffer_size = vertex_position_buffer_size + vertex_varying_enable_blending_buffer_size + vertex_varying_buffer_size + vertex_joint_binding_buffer_size;
			RHIBuffer* inefficient_staging_buffer = RHI_NULL_HANDLE;
			RHIDeviceMemory* inefficient_staging_buffer_memory = RHI_NULL_HANDLE;
			rhi->createBuffer(
				inefficient_staging_buffer_size, RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
				RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				inefficient_staging_buffer,
				inefficient_staging_buffer_memory
			);
			void* inefficient_staging_buffer_data;
			rhi->mapMemory(inefficient_staging_buffer_memory, 0, RHI_WHOLE_SIZE, 0, &inefficient_staging_buffer_data);

			uintptr_t init_address = reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data);
			MeshVertex::VulkanMeshVertexPosition* mesh_verex_positions = reinterpret_cast<MeshVertex::VulkanMeshVertexPosition*>(init_address + vertex_position_buffer_offset);
			MeshVertex::VulkanMeshVertexVaryingEnableBlending* mesh_vertex_enable_blending_varyings = reinterpret_cast<MeshVertex::VulkanMeshVertexVaryingEnableBlending*>(init_address + vertex_varying_enable_blending_buffer_offset);
			MeshVertex::VulkanMeshVertexVarying* mesh_vertex_varyings = reinterpret_cast<MeshVertex::VulkanMeshVertexVarying*>(init_address + vertex_varying_buffer_offset);
			MeshVertex::VulkanMeshVertexJointBinding* mesh_vertex_joint_binding = reinterpret_cast<MeshVertex::VulkanMeshVertexJointBinding*>(init_address + vertex_joint_binding_buffer_offset);

			for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
				Vector3 position = Vector3(
					vertex_buffer_data[vertex_index].x,
					vertex_buffer_data[vertex_index].y,
					vertex_buffer_data[vertex_index].z
				);
				Vector3 normal = Vector3(
					vertex_buffer_data[vertex_index].nx,
					vertex_buffer_data[vertex_index].ny,
					vertex_buffer_data[vertex_index].nz
				);
				Vector3 tangent = Vector3(
					vertex_buffer_data[vertex_index].tx,
					vertex_buffer_data[vertex_index].ty,
					vertex_buffer_data[vertex_index].tz
				);
				Vector2 texcoord = Vector2(
					vertex_buffer_data[vertex_index].u,
					vertex_buffer_data[vertex_index].v
				);
				mesh_verex_positions[vertex_index].position = position;
				mesh_vertex_enable_blending_varyings[vertex_index].normal = normal;
				mesh_vertex_enable_blending_varyings[vertex_index].tangent = tangent;
				mesh_vertex_varyings[vertex_index].texcoord = texcoord;
			}

			for (uint32_t index_index = 0; index_index < index_count; ++index_index) {
				//TODO(move to assets loading process)
				uint32_t vertex_buffer_index = index_buffer_data[index_index];
				mesh_vertex_joint_binding[index_index].indices[0] = joint_binding_buffer_data[vertex_buffer_index].m_index0;
				mesh_vertex_joint_binding[index_index].indices[1] = joint_binding_buffer_data[vertex_buffer_index].m_index1;
				mesh_vertex_joint_binding[index_index].indices[2] = joint_binding_buffer_data[vertex_buffer_index].m_index2;
				mesh_vertex_joint_binding[index_index].indices[3] = joint_binding_buffer_data[vertex_buffer_index].m_index3;
				float inv_total_weight = joint_binding_buffer_data[vertex_buffer_index].m_weight0 + joint_binding_buffer_data[vertex_buffer_index].m_weight1 + joint_binding_buffer_data[vertex_buffer_index].m_weight2 + joint_binding_buffer_data[vertex_buffer_index].m_weight3;
				inv_total_weight = (inv_total_weight != 0.0) ? 1 / inv_total_weight : 1.0;
				Vector4 weigts = Vector4(
					joint_binding_buffer_data[vertex_buffer_index].m_weight0 * inv_total_weight,
					joint_binding_buffer_data[vertex_buffer_index].m_weight1 * inv_total_weight,
					joint_binding_buffer_data[vertex_buffer_index].m_weight2 * inv_total_weight,
					joint_binding_buffer_data[vertex_buffer_index].m_weight3 * inv_total_weight
				);
				mesh_vertex_joint_binding[index_index].weights = weigts;
			}
			rhi->unmapMemory(inefficient_staging_buffer_memory);

			//use vamAllocator to allocate asset vertex buffer
			RHIBufferCreateInfo buffer_info = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			VmaAllocationCreateInfo alloc_info = {};
			alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			buffer_info.usage = RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
			buffer_info.size = vertex_position_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_position_buffer,
				&mesh.mesh_vertex_position_buffer_allocation, nullptr
			);
			buffer_info.size = vertex_varying_enable_blending_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_varying_enable_blending_buffer,
				&mesh.mesh_vertex_varying_enable_blending_buffer_allocation, nullptr
			);
			buffer_info.size = vertex_varying_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_varying_buffer,
				&mesh.mesh_vertex_varying_buffer_allocation, nullptr
			);
			buffer_info.usage = RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
			buffer_info.size = vertex_joint_binding_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_joint_binding_buffer,
				&mesh.mesh_vertex_joint_binding_buffer_allocation, nullptr
			);
			//use date from staging buffer
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_position_buffer,
				vertex_position_buffer_offset, 0, vertex_position_buffer_size
			);
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_varying_enable_blending_buffer,
				vertex_varying_enable_blending_buffer_offset, 0, vertex_varying_enable_blending_buffer_size
			);
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_varying_buffer,
				vertex_varying_buffer_offset, 0, vertex_varying_buffer_size
			);
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_joint_binding_buffer,
				vertex_joint_binding_buffer_offset, 0, vertex_joint_binding_buffer_size
			);
			//release staging buffer
			rhi->destroyBuffer(inefficient_staging_buffer);
			rhi->freeMemory(inefficient_staging_buffer_memory);

			//update descriptor set
			RHIDescriptorSetAllocateInfo mesh_vertex_blending_per_mesh_descriptor_set_alloc_info{};
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pNext = nullptr;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorPool = vulkan_context->m_descriptor_pool;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorSetCount = 1;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pSetLayouts = m_mesh_descriptor_set_layout;
			auto success = rhi->allocateDescriptorSets(&mesh_vertex_blending_per_mesh_descriptor_set_alloc_info, mesh.mesh_vertex_blending_descriptor_set) == RHI_SUCCESS;
			if (!success) {
				LOG_FATAL("allocate mesh vertex blending per mesh descriptor set failed");
			}

			RHIDescriptorBufferInfo mesh_vertex_joint_binding_storage_buffer_info = {};
			mesh_vertex_joint_binding_storage_buffer_info.offset = 0;
			mesh_vertex_joint_binding_storage_buffer_info.range = vertex_joint_binding_buffer_size;
			mesh_vertex_joint_binding_storage_buffer_info.buffer = mesh.mesh_vertex_joint_binding_buffer;
			ASSERT(mesh_vertex_joint_binding_storage_buffer_info.range < m_global_render_resource.m_storage_buffer.m_max_storage_buffer_range);

			RHIDescriptorSet* descriptor_set_to_write = mesh.mesh_vertex_blending_descriptor_set;

			RHIWriteDescriptorSet descriptor_writes[1];

			RHIWriteDescriptorSet& mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info = descriptor_writes[0];
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.pNext = nullptr;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.dstSet = descriptor_set_to_write;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.dstBinding = 0;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.dstArrayElement = 0;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.descriptorCount = 1;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.pBufferInfo = &mesh_vertex_joint_binding_storage_buffer_info;

			rhi->updateDescriptorSets(sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
		}
		else {
			ASSERT((vertex_buffer_size % sizeof(MeshVertexDataDefinition)) == 0);
			uint32_t vertex_count = vertex_buffer_size / sizeof(MeshVertexDataDefinition);

			RHIDeviceSize vertex_position_buffer_size = sizeof(MeshVertex::VulkanMeshVertexPosition) * vertex_count;
			RHIDeviceSize vertex_varying_enable_blending_buffer_size = sizeof(MeshVertex::VulkanMeshVertexVaryingEnableBlending) * vertex_count;
			RHIDeviceSize vertex_varying_buffer_size = sizeof(MeshVertex::VulkanMeshVertexVarying) * vertex_count;

			RHIDeviceSize vertex_position_buffer_offset = 0;
			RHIDeviceSize vertex_varying_enable_blending_buffer_offset = vertex_position_buffer_offset + vertex_position_buffer_size;
			RHIDeviceSize vertex_varying_buffer_offset = vertex_varying_enable_blending_buffer_offset + vertex_varying_enable_blending_buffer_size;

			//temporary staging buffer
			RHIDeviceSize inefficient_staging_buffer_size = vertex_position_buffer_size + vertex_varying_enable_blending_buffer_size + vertex_varying_buffer_size;
			RHIBuffer* inefficient_staging_buffer = RHI_NULL_HANDLE;
			RHIDeviceMemory* inefficient_staging_buffer_memory = RHI_NULL_HANDLE;
			rhi->createBuffer(
				inefficient_staging_buffer_size, RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
				RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				inefficient_staging_buffer,
				inefficient_staging_buffer_memory
			);
			void* inefficient_staging_buffer_data;
			rhi->mapMemory(inefficient_staging_buffer_memory, 0, RHI_WHOLE_SIZE, 0, &inefficient_staging_buffer_data);

			uintptr_t init_address = reinterpret_cast<uintptr_t>(inefficient_staging_buffer_data);
			MeshVertex::VulkanMeshVertexPosition* mesh_verex_positions = reinterpret_cast<MeshVertex::VulkanMeshVertexPosition*>(init_address + vertex_position_buffer_offset);
			MeshVertex::VulkanMeshVertexVaryingEnableBlending* mesh_vertex_enable_blending_varyings = reinterpret_cast<MeshVertex::VulkanMeshVertexVaryingEnableBlending*>(init_address + vertex_varying_enable_blending_buffer_offset);
			MeshVertex::VulkanMeshVertexVarying* mesh_vertex_varyings = reinterpret_cast<MeshVertex::VulkanMeshVertexVarying*>(init_address + vertex_varying_buffer_offset);

			for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
				Vector3 position = Vector3(
					vertex_buffer_data[vertex_index].x,
					vertex_buffer_data[vertex_index].y,
					vertex_buffer_data[vertex_index].z
				);
				Vector3 normal = Vector3(
					vertex_buffer_data[vertex_index].nx,
					vertex_buffer_data[vertex_index].ny,
					vertex_buffer_data[vertex_index].nz
				);
				Vector3 tangent = Vector3(
					vertex_buffer_data[vertex_index].tx,
					vertex_buffer_data[vertex_index].ty,
					vertex_buffer_data[vertex_index].tz
				);
				Vector2 texcoord = Vector2(
					vertex_buffer_data[vertex_index].u,
					vertex_buffer_data[vertex_index].v
				);
				mesh_verex_positions[vertex_index].position = position;
				mesh_vertex_enable_blending_varyings[vertex_index].normal = normal;
				mesh_vertex_enable_blending_varyings[vertex_index].tangent = tangent;
				mesh_vertex_varyings[vertex_index].texcoord = texcoord;
			}
			rhi->unmapMemory(inefficient_staging_buffer_memory);

			//use vamAllocator to allocate asset vertex buffer
			RHIBufferCreateInfo buffer_info = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			VmaAllocationCreateInfo alloc_info = {};
			alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			buffer_info.usage = RHI_BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
			buffer_info.size = vertex_position_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_position_buffer,
				&mesh.mesh_vertex_position_buffer_allocation, nullptr
			);
			buffer_info.size = vertex_varying_enable_blending_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_varying_enable_blending_buffer,
				&mesh.mesh_vertex_varying_enable_blending_buffer_allocation, nullptr
			);
			buffer_info.size = vertex_varying_buffer_size;
			rhi->createBufferVMA(
				vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
				mesh.mesh_vertex_varying_buffer,
				&mesh.mesh_vertex_varying_buffer_allocation, nullptr
			);

			//use date from staging buffer
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_position_buffer,
				vertex_position_buffer_offset, 0, vertex_position_buffer_size
			);
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_varying_enable_blending_buffer,
				vertex_varying_enable_blending_buffer_offset, 0, vertex_varying_enable_blending_buffer_size
			);
			rhi->copyBuffer(
				inefficient_staging_buffer, mesh.mesh_vertex_varying_buffer,
				vertex_varying_buffer_offset, 0, vertex_varying_buffer_size
			);

			//release staging buffer
			rhi->destroyBuffer(inefficient_staging_buffer);
			rhi->freeMemory(inefficient_staging_buffer_memory);

			//update descriptor set
			RHIDescriptorSetAllocateInfo mesh_vertex_blending_per_mesh_descriptor_set_alloc_info;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pNext = nullptr;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorPool = vulkan_context->m_descriptor_pool;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.descriptorSetCount = 1;
			mesh_vertex_blending_per_mesh_descriptor_set_alloc_info.pSetLayouts = m_mesh_descriptor_set_layout;
			auto success = rhi->allocateDescriptorSets(&mesh_vertex_blending_per_mesh_descriptor_set_alloc_info, mesh.mesh_vertex_blending_descriptor_set) == RHI_SUCCESS;
			if (!success) {
				LOG_FATAL("allocate mesh vertex blending per mesh descriptor set failed");
			}

			RHIDescriptorBufferInfo mesh_vertex_joint_binding_storage_buffer_info = {};
			mesh_vertex_joint_binding_storage_buffer_info.offset = 0;
			mesh_vertex_joint_binding_storage_buffer_info.range = 1;
			mesh_vertex_joint_binding_storage_buffer_info.buffer = m_global_render_resource.m_storage_buffer.m_global_null_descriptor_storage_buffer;
			ASSERT(mesh_vertex_joint_binding_storage_buffer_info.range < m_global_render_resource.m_storage_buffer.m_max_storage_buffer_range);

			RHIDescriptorSet* descriptor_set_to_write = mesh.mesh_vertex_blending_descriptor_set;

			RHIWriteDescriptorSet descriptor_writes[1];

			RHIWriteDescriptorSet& mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info = descriptor_writes[0];
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.pNext = nullptr;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.dstSet = descriptor_set_to_write;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.dstBinding = 0;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.dstArrayElement = 0;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.descriptorCount = 1;
			mesh_vertex_blending_vertex_joint_binding_storage_buffer_write_info.pBufferInfo = &mesh_vertex_joint_binding_storage_buffer_info;

			rhi->updateDescriptorSets(sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, nullptr);
		}
	}

	void RenderResource::updateIndexBuffer(std::shared_ptr<RHI> rhi, uint32_t index_buffer_size, void* index_buffer_data, VulkanMesh& mesh) {
		VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());

		//temp staging buffer
		RHIDeviceSize buffer_size = index_buffer_size;
		RHIBuffer* inefficient_staging_buffer;
		RHIDeviceMemory* inefficient_staging_buffer_memory;
		rhi->createBuffer(
			buffer_size, RHI_BUFFER_USAGE_TRANSFER_SRC_BIT,
			RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			inefficient_staging_buffer,
			inefficient_staging_buffer_memory
		);
		void* staging_buffer_data;
		rhi->mapMemory(inefficient_staging_buffer_memory, 0, buffer_size, 0, &staging_buffer_data);
		memcpy(staging_buffer_data, index_buffer_data, (size_t)buffer_size);
		rhi->unmapMemory(inefficient_staging_buffer_memory);

		//use vmaAllocator to allocate asset index buffer
		RHIBufferCreateInfo buffer_info = { RHI_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = buffer_size;
		buffer_info.usage = RHI_BUFFER_USAGE_INDEX_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT;
		VmaAllocationCreateInfo alloc_info = {};
		alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rhi->createBufferVMA(
			vulkan_context->m_assets_allocator, &buffer_info, &alloc_info,
			mesh.mesh_index_buffer, &mesh.mesh_index_buffer_allocation, nullptr
		);

		//use data form staging buffer
		rhi->copyBuffer(inefficient_staging_buffer, mesh.mesh_index_buffer, 0, 0, buffer_size);
		//release temp staging buffer
		rhi->destroyBuffer(inefficient_staging_buffer);
		rhi->freeMemory(inefficient_staging_buffer_memory);
	}

	void RenderResource::updateTextureImageData(std::shared_ptr<RHI> rhi, const TextureDataToUpdate& texture_data) {
		rhi->createGlobalImage(
			texture_data.now_material->base_color_texture_image,
			texture_data.now_material->base_color_image_view,
			texture_data.now_material->base_color_image_allocation,
			texture_data.base_color_image_width,
			texture_data.base_color_image_height,
			texture_data.base_color_image_pixels,
			texture_data.base_color_image_format
		);
		rhi->createGlobalImage(
			texture_data.now_material->metallic_roughness_texture_image,
			texture_data.now_material->metallic_roughness_image_view,
			texture_data.now_material->metallic_roughness_image_allocation,
			texture_data.metallic_roughness_image_width,
			texture_data.metallic_roughness_image_height,
			texture_data.metallic_roughness_image_pixels,
			texture_data.metallic_roughness_image_format
		);
		rhi->createGlobalImage(
			texture_data.now_material->normal_texture_image,
			texture_data.now_material->normal_image_view,
			texture_data.now_material->normal_image_allocation,
			texture_data.normal_image_width,
			texture_data.normal_image_height,
			texture_data.normal_image_pixels,
			texture_data.normal_image_format
		);
		rhi->createGlobalImage(
			texture_data.now_material->occlusion_texture_image,
			texture_data.now_material->occlusion_image_view,
			texture_data.now_material->occlusion_image_allocation,
			texture_data.occlusion_image_width,
			texture_data.occlusion_image_height,
			texture_data.occlusion_image_pixels,
			texture_data.occlusion_image_format
		);
		rhi->createGlobalImage(
			texture_data.now_material->emissive_texture_image,
			texture_data.now_material->emissive_image_view,
			texture_data.now_material->emissive_image_allocation,
			texture_data.emissive_image_width,
			texture_data.emissive_image_height,
			texture_data.emissive_image_pixels,
			texture_data.emissive_image_format
		);
	}

	VulkanMesh& RenderResource::getEntityMesh(RenderEntity entity) {
		size_t asset_id = entity.m_mesh_asset_id;
		auto it = m_vulkan_meshs.find(asset_id);
		if (it != m_vulkan_meshs.end()) {
			return it->second;
		}
		else {
			LOG_FATAL("failed to get entity mesh");
		}
	}

	VulkanPBRMaterial& RenderResource::getEntityMaterial(RenderEntity entity) {
		size_t asset_id = entity.m_material_asset_id;
		auto it = m_vulkan_pbr_material.find(asset_id);
		if (it != m_vulkan_pbr_material.end()) {
			return it->second;
		}
		else {
			LOG_FATAL("failed to get entity material");
		}
	}

	void RenderResource::resetRingBufferOffset(uint8_t current_frame_index) {
		m_global_render_resource.m_storage_buffer.m_global_upload_ringbuffers_end[current_frame_index] = m_global_render_resource.m_storage_buffer.m_global_upload_ringbuffers_begin[current_frame_index];
	}

	void RenderResource::createAndMapStorageBuffer(std::shared_ptr<RHI> rhi) {
		VulkanRHI* vulkan_context = static_cast<VulkanRHI*>(rhi.get());
		StorageBuffer& storage_buffer = m_global_render_resource.m_storage_buffer;
		uint32_t frames_in_flight = vulkan_context->k_max_frames_in_flight;

		RHIPhysicalDeviceProperties properties;
		rhi->getPhysicalDeviceProperties(&properties);

		storage_buffer.m_min_uniform_buffer_offset_alignment = static_cast<uint32_t>(properties.limits.minUniformBufferOffsetAlignment);
		storage_buffer.m_min_storage_buffer_offset_alignment = static_cast<uint32_t>(properties.limits.minStorageBufferOffsetAlignment);
		storage_buffer.m_max_storage_buffer_range = properties.limits.maxStorageBufferRange;
		storage_buffer.m_non_coherent_atom_size = properties.limits.nonCoherentAtomSize;

		//In Vulkan, the storage buffer should be pre-allocated.
		//The size is 128MB in NVIDIA D3D11
		//driver(https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0).
		uint32_t global_storage_buffer_size = 1024 * 1024 * 128;
		rhi->createBuffer(
			global_storage_buffer_size, RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			storage_buffer.m_global_upload_ringbuffer, storage_buffer.m_global_upload_ringbuffer_memory
		);
		storage_buffer.m_global_upload_ringbuffers_begin.resize(frames_in_flight);
		storage_buffer.m_global_upload_ringbuffers_end.resize(frames_in_flight);
		storage_buffer.m_global_upload_ringbuffers_size.resize(frames_in_flight);

		for (uint32_t i = 0; i < frames_in_flight; ++i) {
			storage_buffer.m_global_upload_ringbuffers_begin[i] = (global_storage_buffer_size * i) / frames_in_flight;
			storage_buffer.m_global_upload_ringbuffers_size[i] = (global_storage_buffer_size * (i + 1)) / frames_in_flight - (global_storage_buffer_size * i) / frames_in_flight;
		}

		//axis
		rhi->createBuffer(
			sizeof(AxisStorageBufferObject), RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			storage_buffer.m_axis_inefficient_storage_buffer, storage_buffer.m_axis_inefficient_storage_buffer_memory
		);

		//null descriptor
		rhi->createBuffer(
			64, RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0,
			storage_buffer.m_global_null_descriptor_storage_buffer,
			storage_buffer.m_global_null_descriptor_storage_buffer_memory
		);
		static_assert(64 >= sizeof(MeshVertex::VulkanMeshVertexJointBinding), "");

		//TODO(Umap when program terminates)
		rhi->mapMemory(
			storage_buffer.m_global_upload_ringbuffer_memory, 0, RHI_WHOLE_SIZE, 0,
			&storage_buffer.m_global_upload_ringbuffer_memory_pointer
		);
		rhi->mapMemory(
			storage_buffer.m_axis_inefficient_storage_buffer_memory, 0, RHI_WHOLE_SIZE, 0,
			&storage_buffer.m_axis_inefficient_storage_buffer_memory_pointer
		);
	}
}