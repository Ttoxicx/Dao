#include "runtime/function/render/passes/particle_pass.h"

#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "core/base/macro.h"

#include <particle_emit_comp.h>
#include <particle_kickoff_comp.h>
#include <particle_simulate_comp.h>
#include <particlebillboard_vert.h>
#include <particlebillboard_frag.h>

namespace Dao {
    void ParticleEmitterBufferBatch::freeUpBatch(std::shared_ptr<RHI> rhi) {
        rhi->freeMemory(m_counter_host_memory);
        rhi->freeMemory(m_position_host_memory);
        rhi->freeMemory(m_counter_device_memory);
        rhi->freeMemory(m_position_device_memory);
        rhi->freeMemory(m_indirect_dispatch_argument_memory);
        rhi->freeMemory(m_alive_list_memory);
        rhi->freeMemory(m_alive_list_next_memory);
        rhi->freeMemory(m_dead_list_memory);
        rhi->freeMemory(m_particle_component_res_memory);
        rhi->freeMemory(m_position_render_memory);

        rhi->destroyBuffer(m_counter_host_buffer);
        rhi->destroyBuffer(m_position_host_buffer);
        rhi->destroyBuffer(m_counter_device_buffer);
        rhi->destroyBuffer(m_position_device_buffer);
        rhi->destroyBuffer(m_indirect_dispatch_argument_buffer);
        rhi->destroyBuffer(m_alive_list_buffer);
        rhi->destroyBuffer(m_alive_list_next_buffer);
        rhi->destroyBuffer(m_dead_list_buffer);
        rhi->destroyBuffer(m_particle_component_res_buffer);
        rhi->destroyBuffer(m_position_render_buffer);
    }

    void ParticlePass::initialize(const RenderPassInitInfo* init_info) {
        RenderPass::initialize(nullptr);
        const ParticlePassInitInfo* info = static_cast<const ParticlePassInitInfo*>(init_info);
        _particle_manager = info->m_particle_manager;
    }

    void ParticlePass::copyNormalAndDepthImage() {
        uint8_t index = (m_rhi->getCurrentFrameIndex() + m_rhi->getMaxFramesInFlight() - 1) % m_rhi->getMaxFramesInFlight();

        m_rhi->waitForFencesPFN(1, &(m_rhi->getFenceList()[index]), RHI_TRUE, UINT64_MAX);

        RHICommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;

        bool res_begin_command_buffer = m_rhi->beginCommandBufferPFN(_copy_command_buffer, &command_buffer_begin_info);
        ASSERT(res_begin_command_buffer == RHI_SUCCESS);

        float color[4] = { 1.0f,1.0f,1.0f,1.0f };
        m_rhi->pushEvent(_copy_command_buffer, "Copy Depth Image for Particle", color);
        //depth image
        RHIImageSubresourceRange subresource_range = { RHI_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1 };
        RHIImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = RHI_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.subresourceRange = subresource_range;
        {
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_UNDEFINED;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier.srcAccessMask = 0;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier.image = _dst_depth_image;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_UNDEFINED;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_memory_barrier.srcAccessMask = RHI_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_TRANSFER_READ_BIT;
            image_memory_barrier.image = _src_depth_image;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
            m_rhi->cmdCopyImageToImage(
                _copy_command_buffer,
                _src_depth_image,
                RHI_IMAGE_ASPECT_DEPTH_BIT,
                _dst_depth_image,
                RHI_IMAGE_ASPECT_DEPTH_BIT,
                m_rhi->getSwapchainInfo().extent.width,
                m_rhi->getSwapchainInfo().extent.height
            );
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            image_memory_barrier.srcAccessMask = RHI_ACCESS_TRANSFER_READ_BIT;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | RHI_ACCESS_SHADER_READ_BIT;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_memory_barrier.srcAccessMask = RHI_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            image_memory_barrier.image = _dst_depth_image;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
        }

        m_rhi->popEvent(_copy_command_buffer);

        m_rhi->pushEvent(_copy_command_buffer, "Copy Normal Image for Particle", color);
        //color image
        subresource_range = { RHI_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
        image_memory_barrier.subresourceRange = subresource_range;
        {
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_UNDEFINED;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier.srcAccessMask = 0;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier.image = _dst_normal_image;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_UNDEFINED;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_memory_barrier.srcAccessMask = RHI_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_TRANSFER_READ_BIT;
            image_memory_barrier.image = _src_normal_image;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
            m_rhi->cmdCopyImageToImage(
                _copy_command_buffer,
                _src_normal_image,
                RHI_IMAGE_ASPECT_COLOR_BIT,
                _dst_normal_image,
                RHI_IMAGE_ASPECT_COLOR_BIT,
                m_rhi->getSwapchainInfo().extent.width,
                m_rhi->getSwapchainInfo().extent.height
            );
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_memory_barrier.srcAccessMask = RHI_ACCESS_TRANSFER_READ_BIT;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_COLOR_ATTACHMENT_READ_BIT | RHI_ACCESS_SHADER_READ_BIT;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
            image_memory_barrier.oldLayout = RHI_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            image_memory_barrier.newLayout = RHI_IMAGE_LAYOUT_GENERAL;
            image_memory_barrier.srcAccessMask = RHI_ACCESS_TRANSFER_WRITE_BIT;
            image_memory_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            image_memory_barrier.image = _dst_normal_image;
            m_rhi->cmdPipelineBarrier(
                _copy_command_buffer,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr, 0, nullptr,
                1, &image_memory_barrier
            );
        }

        m_rhi->popEvent(_copy_command_buffer);

        bool res_end_command_buffer = m_rhi->endCommandBufferPFN(_copy_command_buffer);
        ASSERT(res_end_command_buffer == RHI_SUCCESS);

        bool res_reset_fences = m_rhi->resetFencesPFN(1, &m_rhi->getFenceList()[index]);
        ASSERT(res_reset_fences == RHI_SUCCESS);

        RHIPipelineStageFlags wait_stages[] = { RHI_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        RHISubmitInfo submit_info{};
        submit_info.sType = RHI_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &(m_rhi->getTextureCopySemaphore(index));
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &_copy_command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = nullptr;

        bool res_queue_submit = m_rhi->queueSubmit(m_rhi->getGraphicsQueue(), 1, &submit_info, m_rhi->getFenceList()[index]);
        ASSERT(res_queue_submit == RHI_SUCCESS);

        m_rhi->queueWaitIdle(m_rhi->getGraphicsQueue());
    }

    void ParticlePass::updateAfterFramebufferRecreate() {
        m_rhi->destroyImage(_dst_depth_image);
        m_rhi->freeMemory(_dst_depth_image_memory);
        m_rhi->createImage(
            m_rhi->getSwapchainInfo().extent.width,
            m_rhi->getSwapchainInfo().extent.height,
            m_rhi->getDepthImageInfo().depth_image_format,
            RHI_IMAGE_TILING_OPTIMAL,
            RHI_IMAGE_USAGE_SAMPLED_BIT | RHI_IMAGE_USAGE_TRANSFER_DST_BIT,
            RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            _dst_depth_image,
            _dst_depth_image_memory,
            0, 1, 1
        );
        m_rhi->createImageView(
            _dst_depth_image,
            m_rhi->getDepthImageInfo().depth_image_format,
            RHI_IMAGE_ASPECT_DEPTH_BIT,
            RHI_IMAGE_VIEW_TYPE_2D,
            1, 1,
            _src_depth_image_view
        );

        m_rhi->destroyImage(_dst_normal_image);
        m_rhi->freeMemory(_dst_normal_image_memory);
        m_rhi->createImage(
            m_rhi->getSwapchainInfo().extent.width,
            m_rhi->getSwapchainInfo().extent.height,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_IMAGE_TILING_OPTIMAL,
            RHI_IMAGE_USAGE_STORAGE_BIT | RHI_IMAGE_USAGE_TRANSFER_DST_BIT,
            RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            _dst_normal_image,
            _dst_normal_image_memory,
            0, 1, 1
        );
        m_rhi->createImageView(
            _dst_normal_image,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_IMAGE_ASPECT_COLOR_BIT,
            RHI_IMAGE_VIEW_TYPE_2D,
            1, 1,
            _src_normal_image_view
        );

        updateDescriptorSet();
    }

    void ParticlePass::draw() {
        for (int i = 0; i < _emitter_count; ++i) {
            float color[4] = { 1.0f,1.0f,1.0f,1.0f };
            m_rhi->pushEvent(_render_command_buffer, "ParticleBillboard", color);
            m_rhi->cmdBindPipelinePFN(_render_command_buffer, RHI_PIPELINE_BIND_POINT_GRAPHICS, m_render_pipelines[1].pipeline);
            m_rhi->cmdSetViewportPFN(_render_command_buffer, 0, 1, m_rhi->getSwapchainInfo().viewport);
            m_rhi->cmdSetScissorPFN(_render_command_buffer, 0, 1, m_rhi->getSwapchainInfo().scissor);
            m_rhi->cmdBindDescriptorSetsPFN(
                _render_command_buffer,
                RHI_PIPELINE_BIND_POINT_GRAPHICS,
                m_render_pipelines[1].layout,
                0, 1,
                &m_descriptor_infos[i * 3 + 2].descriptor_set,
                0, nullptr
            );
            m_rhi->cmdDraw(_render_command_buffer, 4, _emitter_buffer_batches[i].m_num_particle, 0, 0);
            m_rhi->popEvent(_render_command_buffer);
        }
    }

    void ParticlePass::setupAttachments() {
        //billboard texture
        {
            std::shared_ptr<TextureData> m_particle_billboard_texture_resource = m_render_resource->loadTextureHDR(_particle_manager->getGlobalParticleRes().m_particle_billboard_texture_path);
            m_rhi->createGlobalImage(
                _particle_billboard_texture_image,
                _particle_billboard_texture_image_view,
                _particle_billboard_texture_vma_allocation,
                m_particle_billboard_texture_resource->m_width,
                m_particle_billboard_texture_resource->m_height,
                m_particle_billboard_texture_resource->m_pixels,
                m_particle_billboard_texture_resource->m_format
            );
        }
        //dao texture
        {
            std::shared_ptr<TextureData> m_dao_logo_texture_resource = m_render_resource->loadTexture(_particle_manager->getGlobalParticleRes().m_dao_logo_texture_path, true);
            m_rhi->createGlobalImage(
                _dao_logo_texture_image,
                _dao_logo_texture_image_view,
                _dao_logo_texture_vma_allocation,
                m_dao_logo_texture_resource->m_width,
                m_dao_logo_texture_resource->m_height,
                m_dao_logo_texture_resource->m_pixels,
                m_dao_logo_texture_resource->m_format
            );
        }

        m_rhi->createImage(
            m_rhi->getSwapchainInfo().extent.width,
            m_rhi->getSwapchainInfo().extent.height,
            m_rhi->getDepthImageInfo().depth_image_format,
            RHI_IMAGE_TILING_OPTIMAL,
            RHI_IMAGE_USAGE_SAMPLED_BIT | RHI_IMAGE_USAGE_TRANSFER_DST_BIT,
            RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            _dst_depth_image,
            _dst_depth_image_memory,
            0, 1, 1
        );
        m_rhi->createImageView(
            _dst_depth_image,
            m_rhi->getDepthImageInfo().depth_image_format,
            RHI_IMAGE_ASPECT_DEPTH_BIT,
            RHI_IMAGE_VIEW_TYPE_2D,
            1, 1,
            _src_depth_image_view
        );

        m_rhi->createImage(
            m_rhi->getSwapchainInfo().extent.width,
            m_rhi->getSwapchainInfo().extent.height,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_IMAGE_TILING_OPTIMAL,
            RHI_IMAGE_USAGE_STORAGE_BIT | RHI_IMAGE_USAGE_TRANSFER_DST_BIT,
            RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            _dst_normal_image,
            _dst_normal_image_memory,
            0, 1, 1
        );
        m_rhi->createImageView(
            _dst_normal_image,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_IMAGE_ASPECT_COLOR_BIT,
            RHI_IMAGE_VIEW_TYPE_2D,
            1, 1,
            _src_normal_image_view
        );
    }

    void ParticlePass::setupParticleDescriptorSet() {
        for (int eid = 0; eid < _emitter_count; ++eid) {
            RHIDescriptorSetAllocateInfo particlebillboard_global_descriptorset_alloc_info{};
            particlebillboard_global_descriptorset_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            particlebillboard_global_descriptorset_alloc_info.pNext = nullptr;
            particlebillboard_global_descriptorset_alloc_info.descriptorPool = m_rhi->getDescriptorPool();
            particlebillboard_global_descriptorset_alloc_info.descriptorSetCount = 1;
            particlebillboard_global_descriptorset_alloc_info.pSetLayouts = &m_descriptor_infos[2].layout;
            if (m_rhi->allocateDescriptorSets(&particlebillboard_global_descriptorset_alloc_info, m_descriptor_infos[eid * 3 + 2].descriptor_set) != RHI_SUCCESS) {
                LOG_FATAL("failed to allocate particle billboard global descriptor set");
            }

            RHIDescriptorBufferInfo particlebillboard_perframe_storage_buffer_info = {};
            particlebillboard_perframe_storage_buffer_info.offset = 0;
            particlebillboard_perframe_storage_buffer_info.range = RHI_WHOLE_SIZE;
            particlebillboard_perframe_storage_buffer_info.buffer = _particle_billboard_uniform_buffer;

            RHIDescriptorBufferInfo particlebillboard_perdrawcall_storage_buffer_info = {};
            particlebillboard_perdrawcall_storage_buffer_info.offset = 0;
            particlebillboard_perdrawcall_storage_buffer_info.range = RHI_WHOLE_SIZE;
            particlebillboard_perdrawcall_storage_buffer_info.buffer = _emitter_buffer_batches[eid].m_position_render_buffer;

            RHIWriteDescriptorSet particlebillboard_descriptor_writes_info[3];

            particlebillboard_descriptor_writes_info[0].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            particlebillboard_descriptor_writes_info[0].pNext = nullptr;
            particlebillboard_descriptor_writes_info[0].dstSet = m_descriptor_infos[eid * 3 + 2].descriptor_set;
            particlebillboard_descriptor_writes_info[0].dstBinding = 0;
            particlebillboard_descriptor_writes_info[0].dstArrayElement = 0;
            particlebillboard_descriptor_writes_info[0].descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            particlebillboard_descriptor_writes_info[0].descriptorCount = 1;
            particlebillboard_descriptor_writes_info[0].pBufferInfo = &particlebillboard_perframe_storage_buffer_info;

            particlebillboard_descriptor_writes_info[1].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            particlebillboard_descriptor_writes_info[1].pNext = nullptr;
            particlebillboard_descriptor_writes_info[1].dstSet = m_descriptor_infos[eid * 3 + 2].descriptor_set;
            particlebillboard_descriptor_writes_info[1].dstBinding = 1;
            particlebillboard_descriptor_writes_info[1].dstArrayElement = 0;
            particlebillboard_descriptor_writes_info[1].descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            particlebillboard_descriptor_writes_info[1].descriptorCount = 1;
            particlebillboard_descriptor_writes_info[1].pBufferInfo = &particlebillboard_perdrawcall_storage_buffer_info;

            RHISampler* sampler;
            RHISamplerCreateInfo sampler_create_info{};
            sampler_create_info.sType = RHI_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_create_info.maxAnisotropy = 1.0f;
            sampler_create_info.anisotropyEnable = true;
            sampler_create_info.magFilter = RHI_FILTER_LINEAR;
            sampler_create_info.minFilter = RHI_FILTER_LINEAR;
            sampler_create_info.mipmapMode = RHI_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_create_info.addressModeU = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.addressModeV = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.addressModeW = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.mipLodBias = 0.0f;
            sampler_create_info.compareOp = RHI_COMPARE_OP_NEVER;
            sampler_create_info.minLod = 0.0f;
            sampler_create_info.maxLod = 0.0f;
            sampler_create_info.borderColor = RHI_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            if (m_rhi->createSampler(&sampler_create_info, sampler) != RHI_SUCCESS) {
                LOG_FATAL("failed to create sampler");
            }

            RHIDescriptorImageInfo particle_texture_image_info{};
            particle_texture_image_info.sampler = sampler;
            particle_texture_image_info.imageView = _particle_billboard_texture_image_view;
            particle_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            particlebillboard_descriptor_writes_info[2].sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            particlebillboard_descriptor_writes_info[2].pNext = nullptr;
            particlebillboard_descriptor_writes_info[2].dstSet = m_descriptor_infos[eid * 3 + 2].descriptor_set;
            particlebillboard_descriptor_writes_info[2].dstBinding = 2;
            particlebillboard_descriptor_writes_info[2].dstArrayElement = 0;
            particlebillboard_descriptor_writes_info[2].descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            particlebillboard_descriptor_writes_info[2].descriptorCount = 1;
            particlebillboard_descriptor_writes_info[2].pImageInfo = &particle_texture_image_info;

            m_rhi->updateDescriptorSets(3, particlebillboard_descriptor_writes_info, 0, nullptr);
        }
    }

    void ParticlePass::setEmitterCount(int count) {
        for (int i = 0; i < _emitter_buffer_batches.size(); ++i) {
            _emitter_buffer_batches[i].freeUpBatch(m_rhi);
        }
        _emitter_count = count;
        _emitter_buffer_batches.resize(_emitter_count);
    }


    void ParticlePass::createEmitter(int id, const ParticleEmitterDesc& desc) {
        ParticleCounter counter;
        counter.alive_count = _emitter_buffer_batches[id].m_num_particle;
        counter.dead_count = s_max_particles - _emitter_buffer_batches[id].m_num_particle;
        counter.emit_count = 0;
        counter.alive_count_after_sim = _emitter_buffer_batches[id].m_num_particle;

        if constexpr (s_verbose_particle_alive_info) {
            LOG_INFO("Emitter {} info:", id);
            LOG_INFO(
                "Dead {}, Alive {}, After sim {}, Emit {}",
                counter.dead_count,
                counter.alive_count,
                counter.alive_count_after_sim,
                counter.emit_count
            );
        }

        {
            const RHIDeviceSize indirect_argument_size = sizeof(IndirectArgument);
            struct IndirectArgument indirectargument = {};
            indirectargument.alive_flap_bit = 1;
            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT | RHI_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                _emitter_buffer_batches[id].m_indirect_dispatch_argument_buffer,
                _emitter_buffer_batches[id].m_indirect_dispatch_argument_memory,
                indirect_argument_size,
                &indirectargument,
                indirect_argument_size
            );

            const RHIDeviceSize alive_list_size = 4 * sizeof(uint32_t) * s_max_particles;
            std::vector<int> alive_indices(s_max_particles * 4, 0);
            for (int i = 0; i < s_max_particles; ++i) {
                alive_indices[i * 4] = i;
            }
            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                _emitter_buffer_batches[id].m_alive_list_buffer,
                _emitter_buffer_batches[id].m_alive_list_memory,
                alive_list_size,
                alive_indices.data(),
                alive_list_size
            );

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                _emitter_buffer_batches[id].m_alive_list_next_buffer,
                _emitter_buffer_batches[id].m_alive_list_next_memory,
                alive_list_size
            );

            const RHIDeviceSize dead_list_size = 4 * sizeof(uint32_t) * s_max_particles;
            std::vector<int32_t> dead_indices(s_max_particles * 4, 0);
            for (int32_t i = 0; i < s_max_particles; ++i) {
                dead_indices[i * 4] = s_max_particles - 1 - i;
            }

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                _emitter_buffer_batches[id].m_dead_list_buffer,
                _emitter_buffer_batches[id].m_dead_list_memory,
                dead_list_size,
                dead_indices.data(),
                dead_list_size
            );
        }

        RHIFence* fence = nullptr;
        {
            const RHIDeviceSize counter_buffer_size = sizeof(ParticleCounter);
            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_TRANSFER_SRC_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                _emitter_buffer_batches[id].m_counter_host_buffer,
                _emitter_buffer_batches[id].m_counter_host_memory,
                counter_buffer_size,
                &counter,
                sizeof(counter)
            );

            //flush writes to host visible buffer
            void* mapped;

            m_rhi->mapMemory(_emitter_buffer_batches[id].m_counter_host_memory, 0, RHI_WHOLE_SIZE, 0, &mapped);

            m_rhi->flushMappedMemoryRanges(nullptr, _emitter_buffer_batches[id].m_counter_host_memory, 0, RHI_WHOLE_SIZE);

            m_rhi->unmapMemory(_emitter_buffer_batches[id].m_counter_host_memory);

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_SRC_BIT |
                RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
                RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                _emitter_buffer_batches[id].m_counter_device_buffer,
                _emitter_buffer_batches[id].m_counter_device_memory,
                counter_buffer_size
            );

            //copy to staging buffer
            RHICommandBufferAllocateInfo cmdbuffer_allocate_info{};
            cmdbuffer_allocate_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdbuffer_allocate_info.commandPool = m_rhi->getCommandPool();
            cmdbuffer_allocate_info.level = RHI_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbuffer_allocate_info.commandBufferCount = 1;
            RHICommandBuffer* copy_command;
            if (m_rhi->allocateCommandBuffers(&cmdbuffer_allocate_info, copy_command) != RHI_SUCCESS) {
                LOG_FATAL("failed to alloc command buffer");
            }

            RHICommandBufferBeginInfo cmdbuffer_begin_info{};
            cmdbuffer_begin_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (m_rhi->beginCommandBuffer(copy_command, &cmdbuffer_begin_info) != RHI_SUCCESS) {
                LOG_FATAL("failed to begin command buffer");
            }

            RHIBufferCopy copy_region = {};
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = counter_buffer_size;
            m_rhi->cmdCopyBuffer(
                copy_command,
                _emitter_buffer_batches[id].m_counter_host_buffer,
                _emitter_buffer_batches[id].m_counter_device_buffer,
                1, &copy_region
            );

            if (m_rhi->endCommandBuffer(copy_command) != RHI_SUCCESS) {
                LOG_FATAL("failed to copy buffer");
            }

            RHISubmitInfo submit_info{};
            submit_info.sType = RHI_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &copy_command;
            RHIFenceCreateInfo fenceInfo{};
            fenceInfo.sType = RHI_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = 0;
            if (m_rhi->createFence(&fenceInfo, fence) != RHI_SUCCESS) {
                LOG_FATAL("failed to create fence");
            }

            //submit to the queue
            if (m_rhi->queueSubmit(m_rhi->getComputeQueue(), 1, &submit_info, fence) != RHI_SUCCESS) {
                LOG_FATAL("failed to do queue submit");
            }

            if (m_rhi->waitForFencesPFN(1, &fence, RHI_TRUE, UINT64_MAX) != RHI_SUCCESS) {
                LOG_FATAL("failed to wait fence submit");
            }

            m_rhi->destroyFence(fence);
            m_rhi->freeCommandBuffers(m_rhi->getCommandPool(), 1, copy_command);
        }

        //fill in data
        {
            _emitter_buffer_batches[id].m_emitter_desc = desc;

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                _emitter_buffer_batches[id].m_particle_component_res_buffer,
                _emitter_buffer_batches[id].m_particle_component_res_memory,
                sizeof(ParticleEmitterDesc),
                &_emitter_buffer_batches[id].m_emitter_desc,
                sizeof(ParticleEmitterDesc)
            );

            if (m_rhi->mapMemory(_emitter_buffer_batches[id].m_particle_component_res_memory,
                0,
                RHI_WHOLE_SIZE,
                0,
                &_emitter_buffer_batches[id].m_emitter_desc_mapped) != RHI_SUCCESS)
            {
                LOG_FATAL("failed to map emitter component res buffer");
            }

            const VkDeviceSize stagging_buffer_size = s_max_particles * sizeof(Particle);

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_TRANSFER_SRC_BIT | RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                _emitter_buffer_batches[id].m_position_host_buffer,
                _emitter_buffer_batches[id].m_position_host_memory,
                stagging_buffer_size
            );

            //flush writes to host visible buffer
            void* mapped;
            m_rhi->mapMemory(_emitter_buffer_batches[id].m_position_host_memory, 0, RHI_WHOLE_SIZE, 0, &mapped);

            m_rhi->flushMappedMemoryRanges(nullptr, _emitter_buffer_batches[id].m_position_host_memory, 0, RHI_WHOLE_SIZE);

            m_rhi->unmapMemory(_emitter_buffer_batches[id].m_position_host_memory);

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_SRC_BIT |
                RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
                RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                _emitter_buffer_batches[id].m_position_device_buffer,
                _emitter_buffer_batches[id].m_position_device_memory,
                stagging_buffer_size
            );

            m_rhi->createBufferAndInitialize(
                RHI_BUFFER_USAGE_STORAGE_BUFFER_BIT | RHI_BUFFER_USAGE_TRANSFER_SRC_BIT |
                RHI_BUFFER_USAGE_TRANSFER_DST_BIT,
                RHI_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                _emitter_buffer_batches[id].m_position_render_buffer,
                _emitter_buffer_batches[id].m_position_render_memory,
                stagging_buffer_size
            );

            //copy to staging buffer
            RHICommandBufferAllocateInfo cmdbuffer_allocate_info{};
            cmdbuffer_allocate_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdbuffer_allocate_info.commandPool = m_rhi->getCommandPool();
            cmdbuffer_allocate_info.level = RHI_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbuffer_allocate_info.commandBufferCount = 1;
            RHICommandBuffer* copy_command;
            if (m_rhi->allocateCommandBuffers(&cmdbuffer_allocate_info, copy_command) != RHI_SUCCESS) {
                LOG_FATAL("failed to alloc command buffer");
            }
            RHICommandBufferBeginInfo cmdbuffer_info{};
            cmdbuffer_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (m_rhi->beginCommandBuffer(copy_command, &cmdbuffer_info) != RHI_SUCCESS) {
                LOG_FATAL("failed to begin command buffer");
            }

            RHIBufferCopy copy_region = {};
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = stagging_buffer_size;
            m_rhi->cmdCopyBuffer(
                copy_command,
                _emitter_buffer_batches[id].m_position_host_buffer,
                _emitter_buffer_batches[id].m_position_device_buffer,
                1,
                &copy_region
            );

            if (m_rhi->endCommandBuffer(copy_command) != RHI_SUCCESS) {
                LOG_FATAL("failed to do buffer copy");
            }

            RHISubmitInfo submit_info{};
            submit_info.sType = RHI_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &copy_command;
            RHIFenceCreateInfo fence_info{};
            fence_info.sType = RHI_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.flags = 0;
            if (m_rhi->createFence(&fence_info, fence) != RHI_SUCCESS) {
                LOG_FATAL("failed to create fence");
            }

            //submit to the queue
            if (m_rhi->queueSubmit(m_rhi->getComputeQueue(), 1, &submit_info, fence) != RHI_SUCCESS) {
                LOG_FATAL("failed to do queue submit");
            }

            if (m_rhi->waitForFencesPFN(1, &fence, RHI_TRUE, UINT64_MAX) != RHI_SUCCESS) {
                LOG_FATAL("failed to wait fence submit");
            }

            m_rhi->destroyFence(fence);
            m_rhi->freeCommandBuffers(m_rhi->getCommandPool(), 1, copy_command);
        }
    }

    void ParticlePass::initializeEmitters() {
        allocateDescriptorSet();
        updateDescriptorSet();
        setupParticleDescriptorSet();
    }

    void ParticlePass::setupParticlePass() {
        prepareUniformBuffer();
        setupDescriptorSetLayout();
        setupPipelines();
        setupAttachments();

        RHICommandBufferAllocateInfo cmdbuffer_allocate_info{};
        cmdbuffer_allocate_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbuffer_allocate_info.commandPool = m_rhi->getCommandPool();
        cmdbuffer_allocate_info.level = RHI_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdbuffer_allocate_info.commandBufferCount = 1;
        if (m_rhi->allocateCommandBuffers(&cmdbuffer_allocate_info, _compute_command_buffer) != RHI_SUCCESS) {
            LOG_FATAL("failed to alloc compute command buffer");
        }
        if (m_rhi->allocateCommandBuffers(&cmdbuffer_allocate_info, _copy_command_buffer) != RHI_SUCCESS) {
            LOG_FATAL("failed to alloc copy command buffer");
        }

        RHIFenceCreateInfo fence_create_info{};
        fence_create_info.sType = RHI_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = 0;
        if (m_rhi->createFence(&fence_create_info, _fence) != RHI_SUCCESS) {
            LOG_FATAL("failed to create fence");
        }
    }

    void ParticlePass::setupDescriptorSetLayout() {
        m_descriptor_infos.resize(3);

        //compute descriptor sets
        {
            RHIDescriptorSetLayoutBinding particle_layout_bindings[11] = {};
            {
                RHIDescriptorSetLayoutBinding& uniform_layout_bingding = particle_layout_bindings[0];
                uniform_layout_bingding.binding = 0;
                uniform_layout_bingding.descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uniform_layout_bingding.descriptorCount = 1;
                uniform_layout_bingding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& storage_position_layout_binding = particle_layout_bindings[1];
                storage_position_layout_binding.binding = 1;
                storage_position_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                storage_position_layout_binding.descriptorCount = 1;
                storage_position_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& storage_counter_layout_binding = particle_layout_bindings[2];
                storage_counter_layout_binding.binding = 2;
                storage_counter_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                storage_counter_layout_binding.descriptorCount = 1;
                storage_counter_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& storage_indirectargument_layout_binding = particle_layout_bindings[3];
                storage_indirectargument_layout_binding.binding = 3;
                storage_indirectargument_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                storage_indirectargument_layout_binding.descriptorCount = 1;
                storage_indirectargument_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& alive_list_layout_binding = particle_layout_bindings[4];
                alive_list_layout_binding.binding = 4;
                alive_list_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                alive_list_layout_binding.descriptorCount = 1;
                alive_list_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& dead_list_layout_binding = particle_layout_bindings[5];
                dead_list_layout_binding.binding = 5;
                dead_list_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                dead_list_layout_binding.descriptorCount = 1;
                dead_list_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& alive_list_next_layout_binding = particle_layout_bindings[6];
                alive_list_next_layout_binding.binding = 6;
                alive_list_next_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                alive_list_next_layout_binding.descriptorCount = 1;
                alive_list_next_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& particle_res_layout_binding = particle_layout_bindings[7];
                particle_res_layout_binding.binding = 7;
                particle_res_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                particle_res_layout_binding.descriptorCount = 1;
                particle_res_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& scene_uniformbuffer_layout_binding = particle_layout_bindings[8];
                scene_uniformbuffer_layout_binding.binding = 8;
                scene_uniformbuffer_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                scene_uniformbuffer_layout_binding.descriptorCount = 1;
                scene_uniformbuffer_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& storage_render_position_layout_binding = particle_layout_bindings[9];
                storage_render_position_layout_binding.binding = 9;
                storage_render_position_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                storage_render_position_layout_binding.descriptorCount = 1;
                storage_render_position_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            {
                RHIDescriptorSetLayoutBinding& dao_texture_layout_binding = particle_layout_bindings[10];
                dao_texture_layout_binding.binding = 10;
                dao_texture_layout_binding.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                dao_texture_layout_binding.descriptorCount = 1;
                dao_texture_layout_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;
            }

            RHIDescriptorSetLayoutCreateInfo particle_descriptor_layout_create_info;
            particle_descriptor_layout_create_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            particle_descriptor_layout_create_info.pNext = nullptr;
            particle_descriptor_layout_create_info.flags = 0;
            particle_descriptor_layout_create_info.bindingCount = sizeof(particle_layout_bindings) / sizeof(particle_layout_bindings[0]);
            particle_descriptor_layout_create_info.pBindings = particle_layout_bindings;

            if (m_rhi->createDescriptorSetLayout(&particle_descriptor_layout_create_info, m_descriptor_infos[0].layout) != RHI_SUCCESS) {
                LOG_FATAL("failed to setup particle compute Descriptor");
            }
            LOG_INFO("setup particle compute Descriptor done");
        }
        //scene depth and normal binding
        {
            RHIDescriptorSetLayoutBinding scene_global_layout_bindings[2] = {};

            RHIDescriptorSetLayoutBinding& gbuffer_normal_global_layout_input_attachment_binding = scene_global_layout_bindings[0];
            gbuffer_normal_global_layout_input_attachment_binding.binding = 0;
            gbuffer_normal_global_layout_input_attachment_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            gbuffer_normal_global_layout_input_attachment_binding.descriptorCount = 1;
            gbuffer_normal_global_layout_input_attachment_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;

            RHIDescriptorSetLayoutBinding& gbuffer_depth_global_layout_input_attachment_binding = scene_global_layout_bindings[1];
            gbuffer_depth_global_layout_input_attachment_binding.binding = 1;
            gbuffer_depth_global_layout_input_attachment_binding.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            gbuffer_depth_global_layout_input_attachment_binding.descriptorCount = 1;
            gbuffer_depth_global_layout_input_attachment_binding.stageFlags = RHI_SHADER_STAGE_COMPUTE_BIT;

            RHIDescriptorSetLayoutCreateInfo gbuffer_lighting_global_layout_create_info;
            gbuffer_lighting_global_layout_create_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            gbuffer_lighting_global_layout_create_info.pNext = nullptr;
            gbuffer_lighting_global_layout_create_info.flags = 0;
            gbuffer_lighting_global_layout_create_info.bindingCount = sizeof(scene_global_layout_bindings) / sizeof(scene_global_layout_bindings[0]);
            gbuffer_lighting_global_layout_create_info.pBindings = scene_global_layout_bindings;

            if (m_rhi->createDescriptorSetLayout(&gbuffer_lighting_global_layout_create_info, m_descriptor_infos[1].layout) != RHI_SUCCESS) {
                LOG_FATAL("create scene normal and depth global layout");
            }
        }

        {
            RHIDescriptorSetLayoutBinding particlebillboard_global_layout_bindings[3];

            RHIDescriptorSetLayoutBinding& particlebillboard_global_layout_perframe_storage_buffer_binding = particlebillboard_global_layout_bindings[0];
            particlebillboard_global_layout_perframe_storage_buffer_binding.binding = 0;
            particlebillboard_global_layout_perframe_storage_buffer_binding.descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            particlebillboard_global_layout_perframe_storage_buffer_binding.descriptorCount = 1;
            particlebillboard_global_layout_perframe_storage_buffer_binding.stageFlags = RHI_SHADER_STAGE_VERTEX_BIT;
            particlebillboard_global_layout_perframe_storage_buffer_binding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& particlebillboard_global_layout_perdrawcall_storage_buffer_binding = particlebillboard_global_layout_bindings[1];
            particlebillboard_global_layout_perdrawcall_storage_buffer_binding.binding = 1;
            particlebillboard_global_layout_perdrawcall_storage_buffer_binding.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            particlebillboard_global_layout_perdrawcall_storage_buffer_binding.descriptorCount = 1;
            particlebillboard_global_layout_perdrawcall_storage_buffer_binding.stageFlags = RHI_SHADER_STAGE_VERTEX_BIT;
            particlebillboard_global_layout_perdrawcall_storage_buffer_binding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutBinding& particlebillboard_global_layout_texture_binding = particlebillboard_global_layout_bindings[2];
            particlebillboard_global_layout_texture_binding.binding = 2;
            particlebillboard_global_layout_texture_binding.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            particlebillboard_global_layout_texture_binding.descriptorCount = 1;
            particlebillboard_global_layout_texture_binding.stageFlags = RHI_SHADER_STAGE_FRAGMENT_BIT;
            particlebillboard_global_layout_texture_binding.pImmutableSamplers = nullptr;

            RHIDescriptorSetLayoutCreateInfo particlebillboard_global_layout_create_info;
            particlebillboard_global_layout_create_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            particlebillboard_global_layout_create_info.pNext = nullptr;
            particlebillboard_global_layout_create_info.flags = 0;
            particlebillboard_global_layout_create_info.bindingCount = 3;
            particlebillboard_global_layout_create_info.pBindings = particlebillboard_global_layout_bindings;

            if (m_rhi->createDescriptorSetLayout(&particlebillboard_global_layout_create_info, m_descriptor_infos[2].layout) != RHI_SUCCESS) {
                LOG_FATAL("failed to create particle billboard global layout");
            }
        }
    }

    void ParticlePass::setupPipelines() {
        m_render_pipelines.resize(2);

        //compute pipeline
        {
            RHIDescriptorSetLayout* descriptorset_layouts[2] = { m_descriptor_infos[0].layout,m_descriptor_infos[1].layout };
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info{};
            pipeline_layout_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = sizeof(descriptorset_layouts) / sizeof(descriptorset_layouts[0]);
            pipeline_layout_create_info.pSetLayouts = descriptorset_layouts;

            if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[0].layout) != RHI_SUCCESS) {
                LOG_FATAL("failed to create compute pass pipe layout");
            }
            LOG_INFO("compute pipe layout done");
        }

        /*struct SpecializationData {
            uint32_t BUFFER_ELEMENT_COUNT = 32;
        } specialization_data;

        RHISpecializationMapEntry specialization_map_entry{};
        specialization_map_entry.constantID = 0;
        specialization_map_entry.offset = 0;
        specialization_map_entry.size = sizeof(uint32_t);

        RHISpecializationInfo specialization_info{};
        specialization_info.mapEntryCount = 1;
        specialization_info.pMapEntries = &specialization_map_entry;
        specialization_info.dataSize = sizeof(specialization_data);
        specialization_info.pData = &specialization_data;*/

        RHIComputePipelineCreateInfo compute_pipeline_createInfo{};

        compute_pipeline_createInfo.sType = RHI_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        compute_pipeline_createInfo.layout = m_render_pipelines[0].layout;
        compute_pipeline_createInfo.flags = 0;

        RHIPipelineShaderStageCreateInfo shader_stage = {};
        shader_stage.sType = RHI_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage.stage = RHI_SHADER_STAGE_COMPUTE_BIT;
        shader_stage.pName = "main";

        {
            shader_stage.module = m_rhi->createShaderModule(PARTICLE_KICKOFF_COMP);
            shader_stage.pSpecializationInfo = nullptr;
            ASSERT(shader_stage.module != RHI_NULL_HANDLE);

            compute_pipeline_createInfo.pStage = &shader_stage;
            if (m_rhi->createComputePipelines(nullptr, 1, &compute_pipeline_createInfo, _kickoff_pipeline) != RHI_SUCCESS) {
                LOG_FATAL("failed to create particle kickoff pipeline");
            }
        }

        {
            shader_stage.module = m_rhi->createShaderModule(PARTICLE_EMIT_COMP);
            shader_stage.pSpecializationInfo = nullptr;
            ASSERT(shader_stage.module != RHI_NULL_HANDLE);

            compute_pipeline_createInfo.pStage = &shader_stage;
            if (m_rhi->createComputePipelines(nullptr, 1, &compute_pipeline_createInfo, _emit_pipeline) != RHI_SUCCESS) {
                LOG_FATAL("failed to create particle emit pipe");
            }
        }

        {
            shader_stage.module = m_rhi->createShaderModule(PARTICLE_SIMULATE_COMP);
            shader_stage.pSpecializationInfo = nullptr;
            ASSERT(shader_stage.module != RHI_NULL_HANDLE);

            compute_pipeline_createInfo.pStage = &shader_stage;
            if (m_rhi->createComputePipelines(nullptr, 1, &compute_pipeline_createInfo, _simulate_pipeline) != RHI_SUCCESS) {
                LOG_FATAL("create particle simulate pipe");
            }
        }

        //particle billboard
        {
            RHIDescriptorSetLayout* descriptorset_layouts[1] = { m_descriptor_infos[2].layout };
            RHIPipelineLayoutCreateInfo pipeline_layout_create_info{};
            pipeline_layout_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_create_info.setLayoutCount = 1;
            pipeline_layout_create_info.pSetLayouts = descriptorset_layouts;

            if (m_rhi->createPipelineLayout(&pipeline_layout_create_info, m_render_pipelines[1].layout) != RHI_SUCCESS) {
                LOG_FATAL("failed to create particle billboard pipeline layout");
            }

            RHIShader* vert_shader_module = m_rhi->createShaderModule(PARTICLEBILLBOARD_VERT);
            RHIShader* frag_shader_module = m_rhi->createShaderModule(PARTICLEBILLBOARD_FRAG);

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
            viewport_state_create_info.pViewports = m_rhi->getSwapchainInfo().viewport;
            viewport_state_create_info.scissorCount = 1;
            viewport_state_create_info.pScissors = m_rhi->getSwapchainInfo().scissor;

            RHIPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
            rasterization_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterization_state_create_info.depthClampEnable = RHI_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = RHI_FALSE;
            rasterization_state_create_info.polygonMode = RHI_POLYGON_MODE_FILL;
            rasterization_state_create_info.lineWidth = 1.0f;
            rasterization_state_create_info.cullMode = RHI_CULL_MODE_NONE;
            rasterization_state_create_info.frontFace = RHI_FRONT_FACE_CLOCKWISE;
            rasterization_state_create_info.depthBiasEnable = RHI_FALSE;
            rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
            rasterization_state_create_info.depthBiasClamp = 0.0f;
            rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;

            RHIPipelineMultisampleStateCreateInfo multisample_state_create_info{};
            multisample_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisample_state_create_info.sampleShadingEnable = RHI_FALSE;
            multisample_state_create_info.rasterizationSamples = RHI_SAMPLE_COUNT_1_BIT;

            RHIPipelineColorBlendAttachmentState color_blend_attachments[1] = {};
            color_blend_attachments[0].colorWriteMask = RHI_COLOR_COMPONENT_R_BIT | RHI_COLOR_COMPONENT_G_BIT | RHI_COLOR_COMPONENT_B_BIT | RHI_COLOR_COMPONENT_A_BIT;
            color_blend_attachments[0].blendEnable = RHI_TRUE;
            color_blend_attachments[0].srcColorBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstColorBlendFactor = RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            color_blend_attachments[0].colorBlendOp = RHI_BLEND_OP_ADD;
            color_blend_attachments[0].srcAlphaBlendFactor = RHI_BLEND_FACTOR_ONE;
            color_blend_attachments[0].dstAlphaBlendFactor = RHI_BLEND_FACTOR_ZERO;
            color_blend_attachments[0].alphaBlendOp = RHI_BLEND_OP_ADD;

            RHIPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
            color_blend_state_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_state_create_info.logicOpEnable = RHI_FALSE;
            color_blend_state_create_info.logicOp = RHI_LOGIC_OP_COPY;
            color_blend_state_create_info.attachmentCount = sizeof(color_blend_attachments) / sizeof(color_blend_attachments[0]);
            color_blend_state_create_info.pAttachments = &color_blend_attachments[0];
            color_blend_state_create_info.blendConstants[0] = 0.0f;
            color_blend_state_create_info.blendConstants[1] = 0.0f;
            color_blend_state_create_info.blendConstants[2] = 0.0f;
            color_blend_state_create_info.blendConstants[3] = 0.0f;

            RHIPipelineDepthStencilStateCreateInfo depth_stencil_create_info{};
            depth_stencil_create_info.sType = RHI_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil_create_info.depthTestEnable = RHI_TRUE;
            depth_stencil_create_info.depthWriteEnable = RHI_FALSE;
            depth_stencil_create_info.depthCompareOp = RHI_COMPARE_OP_LESS;
            depth_stencil_create_info.depthBoundsTestEnable = RHI_FALSE;
            depth_stencil_create_info.stencilTestEnable = RHI_FALSE;

            RHIDynamicState dynamic_states[] = { RHI_DYNAMIC_STATE_VIEWPORT, RHI_DYNAMIC_STATE_SCISSOR };

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
            pipeline_info.pDepthStencilState = &depth_stencil_create_info;
            pipeline_info.layout = m_render_pipelines[1].layout;
            pipeline_info.renderPass = _render_pass;
            pipeline_info.subpass = main_camera_subpass_forward_lighting;
            pipeline_info.basePipelineHandle = RHI_NULL_HANDLE;
            pipeline_info.pDynamicState = &dynamic_state_create_info;

            if (m_rhi->createGraphicsPipelines(RHI_NULL_HANDLE, 1, &pipeline_info, m_render_pipelines[1].pipeline) != RHI_SUCCESS) {
                LOG_FATAL("create particle billboard graphics pipeline");
            }

            m_rhi->destroyShaderModule(vert_shader_module);
            m_rhi->destroyShaderModule(frag_shader_module);
        }
    }

    void ParticlePass::allocateDescriptorSet() {
        RHIDescriptorSetAllocateInfo particle_descriptor_set_alloc_info;
        particle_descriptor_set_alloc_info.sType = RHI_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        particle_descriptor_set_alloc_info.descriptorPool = m_rhi->getDescriptorPool();

        m_descriptor_infos.resize(3 * _emitter_count);
        for (int eid = 0; eid < _emitter_count; ++eid) {
            particle_descriptor_set_alloc_info.pSetLayouts = &m_descriptor_infos[0].layout;
            particle_descriptor_set_alloc_info.descriptorSetCount = 1;
            particle_descriptor_set_alloc_info.pNext = nullptr;

            if (m_rhi->allocateDescriptorSets(&particle_descriptor_set_alloc_info, m_descriptor_infos[eid * 3].descriptor_set) != RHI_SUCCESS) {
                LOG_FATAL("failed to allocate compute descriptor set");
            }
            particle_descriptor_set_alloc_info.pSetLayouts = &m_descriptor_infos[1].layout;
            particle_descriptor_set_alloc_info.descriptorSetCount = 1;
            particle_descriptor_set_alloc_info.pNext = nullptr;

            if (m_rhi->allocateDescriptorSets(&particle_descriptor_set_alloc_info, m_descriptor_infos[eid * 3 + 1].descriptor_set) != RHI_SUCCESS) {
                LOG_INFO("failed to allocate normal and depth descriptor set");
            }
        }
    }

    void ParticlePass::updateDescriptorSet() {
        for (int eid = 0; eid < _emitter_count; ++eid) {
            //compute part
            {
                std::vector<RHIWriteDescriptorSet> compute_write_descriptorSets{ {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} };

                RHIDescriptorBufferInfo uniform_buffer_descriptor = { _compute_uniform_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[0];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorset.dstBinding = 0;
                    descriptorset.pBufferInfo = &uniform_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo position_buffer_descriptor = { _emitter_buffer_batches[eid].m_position_device_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[1];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 1;
                    descriptorset.pBufferInfo = &position_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo counter_buffer_descriptor = { _emitter_buffer_batches[eid].m_counter_device_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[2];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 2;
                    descriptorset.pBufferInfo = &counter_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo indirect_argument_buffer_descriptor = { _emitter_buffer_batches[eid].m_indirect_dispatch_argument_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[3];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 3;
                    descriptorset.pBufferInfo = &indirect_argument_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo alive_list_buffer_descriptor = { _emitter_buffer_batches[eid].m_alive_list_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[4];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 4;
                    descriptorset.pBufferInfo = &alive_list_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo dead_list_buffer_descriptor = { _emitter_buffer_batches[eid].m_dead_list_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[5];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 5;
                    descriptorset.pBufferInfo = &dead_list_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo alive_list_next_buffer_descriptor = { _emitter_buffer_batches[eid].m_alive_list_next_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[6];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 6;
                    descriptorset.pBufferInfo = &alive_list_next_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo particle_component_res_buffer_descriptor = { _emitter_buffer_batches[eid].m_particle_component_res_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[7];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 7;
                    descriptorset.pBufferInfo = &particle_component_res_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo particle_scene_uniform_buffer_descriptor = { _scene_uniform_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[8];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorset.dstBinding = 8;
                    descriptorset.pBufferInfo = &particle_scene_uniform_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHIDescriptorBufferInfo position_render_buffer_descriptor = { _emitter_buffer_batches[eid].m_position_render_buffer, 0, RHI_WHOLE_SIZE };
                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[9];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    descriptorset.dstBinding = 9;
                    descriptorset.pBufferInfo = &position_render_buffer_descriptor;
                    descriptorset.descriptorCount = 1;
                }

                RHISampler* sampler;
                RHISamplerCreateInfo sampler_create_info{};
                sampler_create_info.sType = RHI_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler_create_info.maxAnisotropy = 1.0f;
                sampler_create_info.anisotropyEnable = true;
                sampler_create_info.magFilter = RHI_FILTER_LINEAR;
                sampler_create_info.minFilter = RHI_FILTER_LINEAR;
                sampler_create_info.mipmapMode = RHI_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler_create_info.addressModeU = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_create_info.addressModeV = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_create_info.addressModeW = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_create_info.mipLodBias = 0.0f;
                sampler_create_info.compareOp = RHI_COMPARE_OP_NEVER;
                sampler_create_info.minLod = 0.0f;
                sampler_create_info.maxLod = 0.0f;
                sampler_create_info.borderColor = RHI_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

                if (m_rhi->createSampler(&sampler_create_info, sampler) != RHI_SUCCESS) {
                    LOG_FATAL("failed to create sampler");
                }

                RHIDescriptorImageInfo dao_texture_image_info = {};
                dao_texture_image_info.sampler = sampler;
                dao_texture_image_info.imageView = _dao_logo_texture_image_view;
                dao_texture_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                {
                    RHIWriteDescriptorSet& descriptorset = compute_write_descriptorSets[10];
                    descriptorset.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorset.dstSet = m_descriptor_infos[eid * 3].descriptor_set;
                    descriptorset.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    descriptorset.dstBinding = 10;
                    descriptorset.pImageInfo = &dao_texture_image_info;
                    descriptorset.descriptorCount = 1;
                }

                m_rhi->updateDescriptorSets(
                    static_cast<uint32_t>(compute_write_descriptorSets.size()),
                    compute_write_descriptorSets.data(), 0, nullptr
                );
            }
            {
                RHIWriteDescriptorSet descriptor_input_attachment_writes_info[2] = { {}, {} };

                RHIDescriptorImageInfo gbuffer_normal_descriptor_image_info = {};
                gbuffer_normal_descriptor_image_info.sampler = nullptr;
                gbuffer_normal_descriptor_image_info.imageView = _src_normal_image_view;
                gbuffer_normal_descriptor_image_info.imageLayout = RHI_IMAGE_LAYOUT_GENERAL;
                {

                    RHIWriteDescriptorSet& gbuffer_normal_descriptor_input_attachment_write_info = descriptor_input_attachment_writes_info[0];
                    gbuffer_normal_descriptor_input_attachment_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    gbuffer_normal_descriptor_input_attachment_write_info.pNext = nullptr;
                    gbuffer_normal_descriptor_input_attachment_write_info.dstSet = m_descriptor_infos[eid * 3 + 1].descriptor_set;
                    gbuffer_normal_descriptor_input_attachment_write_info.dstBinding = 0;
                    gbuffer_normal_descriptor_input_attachment_write_info.dstArrayElement = 0;
                    gbuffer_normal_descriptor_input_attachment_write_info.descriptorType = RHI_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    gbuffer_normal_descriptor_input_attachment_write_info.descriptorCount = 1;
                    gbuffer_normal_descriptor_input_attachment_write_info.pImageInfo = &gbuffer_normal_descriptor_image_info;
                }

                RHISampler* sampler;
                RHISamplerCreateInfo sampler_create_info{};
                sampler_create_info.sType = RHI_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                sampler_create_info.maxAnisotropy = 1.0f;
                sampler_create_info.anisotropyEnable = true;
                sampler_create_info.magFilter = RHI_FILTER_NEAREST;
                sampler_create_info.minFilter = RHI_FILTER_NEAREST;
                sampler_create_info.mipmapMode = RHI_SAMPLER_MIPMAP_MODE_LINEAR;
                sampler_create_info.addressModeU = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_create_info.addressModeV = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_create_info.addressModeW = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_create_info.mipLodBias = 0.0f;
                sampler_create_info.compareOp = RHI_COMPARE_OP_NEVER;
                sampler_create_info.minLod = 0.0f;
                sampler_create_info.maxLod = 0.0f;
                sampler_create_info.borderColor = RHI_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                if (m_rhi->createSampler(&sampler_create_info, sampler) != RHI_SUCCESS) {
                    LOG_FATAL("failed to create sampler");
                }

                RHIDescriptorImageInfo depth_descriptor_image_info = {};
                depth_descriptor_image_info.sampler = sampler;
                depth_descriptor_image_info.imageView = _src_depth_image_view;
                depth_descriptor_image_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                {
                    RHIWriteDescriptorSet& depth_descriptor_input_attachment_write_info = descriptor_input_attachment_writes_info[1];
                    depth_descriptor_input_attachment_write_info.sType = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    depth_descriptor_input_attachment_write_info.pNext = nullptr;
                    depth_descriptor_input_attachment_write_info.dstSet = m_descriptor_infos[eid * 3 + 1].descriptor_set;
                    depth_descriptor_input_attachment_write_info.dstBinding = 1;
                    depth_descriptor_input_attachment_write_info.dstArrayElement = 0;
                    depth_descriptor_input_attachment_write_info.descriptorType = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    depth_descriptor_input_attachment_write_info.descriptorCount = 1;
                    depth_descriptor_input_attachment_write_info.pImageInfo = &depth_descriptor_image_info;
                }

                m_rhi->updateDescriptorSets(
                    sizeof(descriptor_input_attachment_writes_info) / sizeof(descriptor_input_attachment_writes_info[0]),
                    descriptor_input_attachment_writes_info, 0, nullptr
                );
            }
        }
    }

    void ParticlePass::simulate() {
        for (auto i : _emitter_tick_indices)
        {
            RHICommandBufferBeginInfo cmdbuffer_info{};
            cmdbuffer_info.sType = RHI_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            // particle compute pass
            if (m_rhi->beginCommandBuffer(_compute_command_buffer, &cmdbuffer_info) != RHI_SUCCESS) {
                LOG_FATAL("failed to begin command buffer");
            }

            float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            m_rhi->pushEvent(_compute_command_buffer, "Particle Compute", color);
            m_rhi->pushEvent(_compute_command_buffer, "Particle Kickoff", color);

            m_rhi->cmdBindPipelinePFN(_compute_command_buffer, RHI_PIPELINE_BIND_POINT_COMPUTE, _kickoff_pipeline);
            RHIDescriptorSet* descriptorsets[2] = { m_descriptor_infos[i * 3].descriptor_set,m_descriptor_infos[i * 3 + 1].descriptor_set };
            m_rhi->cmdBindDescriptorSetsPFN(
                _compute_command_buffer,
                RHI_PIPELINE_BIND_POINT_COMPUTE,
                m_render_pipelines[0].layout,
                0, 2,
                descriptorsets,
                0, 0
            );

            //start compute shader and set workgoup on x,y,z dimension, final workgroup count equals x*y*z
            m_rhi->cmdDispatch(_compute_command_buffer, 1, 1, 1);

            m_rhi->popEvent(_compute_command_buffer); //end particle kickoff label

            RHIBufferMemoryBarrier buffer_barrier{};
            buffer_barrier.sType = RHI_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            buffer_barrier.buffer = _emitter_buffer_batches[i].m_counter_device_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0,
                nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_indirect_dispatch_argument_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_INDIRECT_COMMAND_READ_BIT | RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            m_rhi->pushEvent(_compute_command_buffer, "Particle Emit", color);

            m_rhi->cmdBindPipelinePFN(_compute_command_buffer, RHI_PIPELINE_BIND_POINT_COMPUTE, _emit_pipeline);
            //用于启动计算着色器的间接调度，使用缓冲区中的参数来指定工作组的数量
            m_rhi->cmdDispatchIndirect(_compute_command_buffer, _emitter_buffer_batches[i].m_indirect_dispatch_argument_buffer, s_argument_offset_emit);

            m_rhi->popEvent(_compute_command_buffer); //end particle emit label

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_position_device_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_position_render_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_counter_device_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_alive_list_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_dead_list_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            buffer_barrier.buffer = _emitter_buffer_batches[i].m_alive_list_next_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_SHADER_READ_BIT;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            m_rhi->pushEvent(_compute_command_buffer, "Particle Simulate", color);

            m_rhi->cmdBindPipelinePFN(_compute_command_buffer, RHI_PIPELINE_BIND_POINT_COMPUTE, _simulate_pipeline);
            m_rhi->cmdDispatchIndirect(_compute_command_buffer, _emitter_buffer_batches[i].m_indirect_dispatch_argument_buffer, s_argument_offset_simulate);

            m_rhi->popEvent(_compute_command_buffer); //end particle simulate label

            if (m_rhi->endCommandBuffer(_compute_command_buffer) != RHI_SUCCESS) {
                LOG_FATAL("failed to end command buffer");
            }
            m_rhi->resetFencesPFN(1, &_fence);

            RHISubmitInfo compute_submit_info{};
            compute_submit_info.sType = RHI_STRUCTURE_TYPE_SUBMIT_INFO;
            compute_submit_info.pWaitDstStageMask = 0;
            compute_submit_info.commandBufferCount = 1;
            compute_submit_info.pCommandBuffers = &_compute_command_buffer;

            if (m_rhi->queueSubmit(m_rhi->getComputeQueue(), 1, &compute_submit_info, _fence) != RHI_SUCCESS) {
                LOG_FATAL("failed to do compute queue submit");
            }

            if (m_rhi->waitForFencesPFN(1, &_fence, RHI_TRUE, UINT64_MAX) != RHI_SUCCESS) {
                LOG_FATAL("failed to wait for fence");
            }

            if (m_rhi->beginCommandBuffer(_compute_command_buffer, &cmdbuffer_info) != RHI_SUCCESS) {
                LOG_FATAL("failed to begin command buffer");
            }

            m_rhi->pushEvent(_compute_command_buffer, "Copy Particle Counter Buffer", color);

            //barrier to ensure that shader writes are finished before buffer is read back from GPU
            buffer_barrier.srcAccessMask = RHI_ACCESS_SHADER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_TRANSFER_READ_BIT;
            buffer_barrier.buffer = _emitter_buffer_batches[i].m_counter_device_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                RHI_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );
            //read back to host visible buffer

            RHIBufferCopy copy_region{};
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = sizeof(ParticleCounter);

            m_rhi->cmdCopyBuffer(
                _compute_command_buffer,
                _emitter_buffer_batches[i].m_counter_device_buffer,
                _emitter_buffer_batches[i].m_counter_host_buffer,
                1, &copy_region
            );

            //barrier to ensure that buffer copy is finished before host reading from it
            buffer_barrier.srcAccessMask = RHI_ACCESS_TRANSFER_WRITE_BIT;
            buffer_barrier.dstAccessMask = RHI_ACCESS_HOST_READ_BIT;
            buffer_barrier.buffer = _emitter_buffer_batches[i].m_counter_host_buffer;
            buffer_barrier.size = RHI_WHOLE_SIZE;
            buffer_barrier.srcQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;
            buffer_barrier.dstQueueFamilyIndex = RHI_QUEUE_FAMILY_IGNORED;

            m_rhi->cmdPipelineBarrier(
                _compute_command_buffer,
                RHI_PIPELINE_STAGE_TRANSFER_BIT,
                RHI_PIPELINE_STAGE_HOST_BIT,
                0, 0, nullptr,
                1, &buffer_barrier,
                0, nullptr
            );

            m_rhi->popEvent(_compute_command_buffer); //end particle counter copy label

            m_rhi->popEvent(_compute_command_buffer); //end particle compute label

            if (m_rhi->endCommandBuffer(_compute_command_buffer) != RHI_SUCCESS) {
                LOG_FATAL("failed to end command buffer");
            }

            //submit compute work
            m_rhi->resetFencesPFN(1, &_fence);
            compute_submit_info = {};
            const VkPipelineStageFlags waitStageMask = RHI_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            compute_submit_info.sType = RHI_STRUCTURE_TYPE_SUBMIT_INFO;
            compute_submit_info.pWaitDstStageMask = &waitStageMask;
            compute_submit_info.commandBufferCount = 1;
            compute_submit_info.pCommandBuffers = &_compute_command_buffer;

            if (m_rhi->queueSubmit(m_rhi->getComputeQueue(), 1, &compute_submit_info, _fence) != RHI_SUCCESS) {
                LOG_FATAL("failed to compute queue submit");
            }

            if (m_rhi->waitForFencesPFN(1, &_fence, RHI_TRUE, UINT64_MAX) != RHI_SUCCESS) {
                LOG_FATAL("failed to wait for fence");
            }

            m_rhi->queueWaitIdle(m_rhi->getComputeQueue());

            //make device writes visible to the host
            void* mapped;
            m_rhi->mapMemory(_emitter_buffer_batches[i].m_counter_host_memory, 0, RHI_WHOLE_SIZE, 0, &mapped);

            m_rhi->invalidateMappedMemoryRanges(nullptr, _emitter_buffer_batches[i].m_counter_host_memory, 0, RHI_WHOLE_SIZE);

            //copy to output
            ParticleCounter counter_next{};
            memcpy(&counter_next, mapped, sizeof(ParticleCounter));
            m_rhi->unmapMemory(_emitter_buffer_batches[i].m_counter_host_memory);

            if constexpr (s_verbose_particle_alive_info) {
                LOG_INFO(
                    "{} {} {} {}",
                    counter_next.dead_count,
                    counter_next.alive_count,
                    counter_next.alive_count_after_sim,
                    counter_next.emit_count
                );
            }
            _emitter_buffer_batches[i].m_num_particle = counter_next.alive_count_after_sim;
        }
        _emitter_tick_indices.clear();
        _emitter_transform_indices.clear();
    }

    void ParticlePass::prepareUniformBuffer() {
        RHIDeviceMemory* d_mem;
        m_rhi->createBuffer(
            sizeof(_particle_collision_perframe_storage_buffer_object),
            RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _scene_uniform_buffer, d_mem
        );

        if (m_rhi->mapMemory(d_mem, 0, RHI_WHOLE_SIZE, 0, &_scene_uniform_buffer_mapped) != RHI_SUCCESS) {
            LOG_FATAL("failed to map billboard uniform buffer");
        }
        RHIDeviceMemory* d_uniformdmemory;

        m_rhi->createBufferAndInitialize(
            RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _compute_uniform_buffer,
            d_uniformdmemory,
            sizeof(m_ubo)
        );

        if (m_rhi->mapMemory(d_uniformdmemory, 0, RHI_WHOLE_SIZE, 0, &_particle_compute_buffer_mapped) != RHI_SUCCESS) {
            LOG_FATAL("failed to map buffer");
        }

        const GlobalParticleRes& global_res = _particle_manager->getGlobalParticleRes();

        m_ubo.emit_gap = global_res.m_emit_gap;
        m_ubo.time_step = global_res.m_time_step;
        m_ubo.max_life = global_res.m_max_life;
        m_ubo.gravity = global_res.m_gravity;
        std::random_device r;
        std::seed_seq seed{ r() };
        _random_engine.seed(seed);
        float rnd0 = _random_engine.uniformDistribution<float>(0, 1000) * 0.001f;
        float rnd1 = _random_engine.uniformDistribution<float>(0, 1000) * 0.001f;
        float rnd2 = _random_engine.uniformDistribution<float>(0, 1000) * 0.001f;
        m_ubo.pack = Vector4{ rnd0, static_cast<float>(m_rhi->getCurrentFrameIndex()), rnd1, rnd2 };
        m_ubo.xemit_count = 100000;

        m_viewport_params = *m_rhi->getSwapchainInfo().viewport;
        m_ubo.viewport.x = m_viewport_params.x;
        m_ubo.viewport.y = m_viewport_params.y;
        m_ubo.viewport.z = m_viewport_params.width;
        m_ubo.viewport.w = m_viewport_params.height;
        m_ubo.extent.x = m_rhi->getSwapchainInfo().scissor->extent.width;
        m_ubo.extent.y = m_rhi->getSwapchainInfo().scissor->extent.height;

        memcpy(_particle_compute_buffer_mapped, &m_ubo, sizeof(m_ubo));

        {
            RHIDeviceMemory* d_mem;
            m_rhi->createBuffer(
                sizeof(_particle_billboard_perframe_storage_buffer_object),
                RHI_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                RHI_MEMORY_PROPERTY_HOST_VISIBLE_BIT | RHI_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                _particle_billboard_uniform_buffer,
                d_mem
            );

            if (m_rhi->mapMemory(d_mem, 0, RHI_WHOLE_SIZE, 0, &_particle_billboard_uniform_buffer_mapped) != RHI_SUCCESS) {
                LOG_FATAL("map billboard uniform buffer");
            }
        }
    }

    void ParticlePass::updateEmitterTransform() {
        for (ParticleEmitterTransformDesc& transform_desc : _emitter_transform_indices)
        {
            int index = transform_desc.m_id;
            _emitter_buffer_batches[index].m_emitter_desc.m_position = transform_desc.m_position;
            _emitter_buffer_batches[index].m_emitter_desc.m_rotation = transform_desc.m_rotation;

            memcpy(_emitter_buffer_batches[index].m_emitter_desc_mapped, &_emitter_buffer_batches[index].m_emitter_desc, sizeof(ParticleEmitterDesc));
        }
    }

    void ParticlePass::updateUniformBuffer() {
        std::random_device r;
        std::seed_seq seed{ r() };
        _random_engine.seed(seed);
        float rnd0 = _random_engine.uniformDistribution<float>(0, 1000) * 0.001f;
        float rnd1 = _random_engine.uniformDistribution<float>(0, 1000) * 0.001f;
        float rnd2 = _random_engine.uniformDistribution<float>(0, 1000) * 0.001f;
        m_ubo.pack = Vector4{ rnd0, rnd1, rnd2, static_cast<float>(m_rhi->getCurrentFrameIndex()) };

        m_ubo.viewport.x = m_rhi->getSwapchainInfo().viewport->x;
        m_ubo.viewport.y = m_rhi->getSwapchainInfo().viewport->y;
        m_ubo.viewport.z = m_rhi->getSwapchainInfo().viewport->width;
        m_ubo.viewport.w = m_rhi->getSwapchainInfo().viewport->height;
        m_ubo.extent.x = m_rhi->getSwapchainInfo().scissor->extent.width;
        m_ubo.extent.y = m_rhi->getSwapchainInfo().scissor->extent.height;

        m_ubo.extent.z = g_runtime_global_context.m_render_system->getRenderCamera()->m_znear;
        m_ubo.extent.w = g_runtime_global_context.m_render_system->getRenderCamera()->m_zfar;
        memcpy(_particle_compute_buffer_mapped, &m_ubo, sizeof(m_ubo));
    }

    void ParticlePass::preparePassData(std::shared_ptr<RenderResourceBase> render_resource) {
        const RenderResource* vulkan_resource = static_cast<const RenderResource*>(render_resource.get());
        if (vulkan_resource)
        {
            _particle_collision_perframe_storage_buffer_object = vulkan_resource->m_particle_collision_perframe_storage_buffer_object;
            memcpy(_scene_uniform_buffer_mapped, &_particle_collision_perframe_storage_buffer_object, sizeof(ParticleCollisionPerframeStorageBufferObject));

            _particle_billboard_perframe_storage_buffer_object = vulkan_resource->m_particle_billboard_perframe_storage_buffer_object;
            memcpy(_particle_billboard_uniform_buffer_mapped, &_particle_billboard_perframe_storage_buffer_object, sizeof(_particle_billboard_perframe_storage_buffer_object));

            m_viewport_params = *m_rhi->getSwapchainInfo().viewport;
            updateUniformBuffer();
            updateEmitterTransform();
        }
    }

    void ParticlePass::setDepthAndNormalImage(RHIImage* depth_image, RHIImage* normal_image) {
        _src_depth_image = depth_image;
        _src_normal_image = normal_image;
    }

    void ParticlePass::setRenderCommandBufferHandle(RHICommandBuffer* command_buffer) {
        _render_command_buffer = command_buffer;
    }

    void ParticlePass::setRenderPassHandle(RHIRenderPass* render_pass) {
        _render_pass = render_pass;
    }

    void ParticlePass::setTickIndices(const std::vector<ParticleEmitterID>& tick_indices) {
        _emitter_tick_indices = tick_indices;
    }

    void ParticlePass::setTransformIndices(const std::vector<ParticleEmitterTransformDesc>& transform_indices) {
        _emitter_transform_indices = transform_indices;
    }
}