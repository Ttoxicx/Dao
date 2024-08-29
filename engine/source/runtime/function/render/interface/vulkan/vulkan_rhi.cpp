#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"

#include <algorithm>
#include <cmath>


#define DAO_XSTR(s) DAO_STR(s)
#define DAO_STR(s) #s

#ifdef _MSC_VER
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN 1
#define NOGDICAPMASKS 1
#define NOVIRTUALKEYCODES 1
#define NOWINMESSAGES 1
#define NOWINSTYLES 1
#define NOSYSMETRICS 1
#define NOMENUS 1
#define NOICONS 1
#define NOKEYSTATES 1
#define NOSYSCOMMANDS 1
#define NORASTEROPS 1
#define NOSHOWWINDOW 1
#define NOATOM 1
#define NOCLIPBOARD 1
#define NOCOLOR 1
#define NOCTLMGR 1
#define NODRAWTEXT 1
#define NOGDI 1
#define NOKERNEL 1
#define NOUSER 1
#define NONLS 1
#define NOMB 1
#define NOMEMMGR 1
#define NOMETAFILE 1
#define NOMINMAX 1
#define NOMSG 1
#define NOOPENFILE 1
#define NOSCROLL 1
#define NOSERVICE 1
#define NOSOUND 1
#define NOTEXTMETRIC 1
#define NOWH 1
#define NOWINOFFSETS 1
#define NOCOMM 1
#define NOKANJI 1
#define NOHELP 1
#define NOPROFILER 1
#define NODEFERWINDOWPOS 1
#define NOMCX 1
#include <Windows.h>
#else
#error Unknown Compiler
#endif

#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace Dao {

	VulkanRHI::~VulkanRHI() {

	}

	void VulkanRHI::initialize(RHIInitInfo init_info) {
		m_window = init_info.window_system->getWindow();
		std::array<int, 2> window_size = init_info.window_system->getWindowSize();
		m_viewport = { 0.0f,0.0f,(float)window_size[0],(float)window_size[1],0.0f,1.0f };
		m_scissor = { {0,0},{(uint32_t)window_size[0],(uint32_t)window_size[1]} };

#ifndef NDEBUG
		_enable_validation_layers = true;
        _enable_debug_utils_label = true;
#else
		_enable_validation_layers = false;
        _enable_debug_utils_label = false;
#endif // !NDEBUG

#ifdef _MSC_VER
		_enable_point_light_shadow = true;
#else
		_enable_point_light_shadow = false;
#endif

#ifdef _MSC_VER
		char const* vk_layer_path = DAO_XSTR(DAO_VK_LAYER_PATH);
		SetEnvironmentVariableA("VK_LAYER_PATH", vk_layer_path);
		SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");
#else 
#error Unknown Compiler
#endif
		createInstance();
		initializeDebugMessenger();
		createWindowSurface();
		initializePhysicalDevice();
		createLogicalDevice();
		createCommandPool();
		createCommandBuffers();
		createDescriptorPool();
		createSyncPrimitives();
		createSwapchain();
		createSwapchainImageViews();
        createFramebufferImageAndView();
		createAssetAllocator();
	}

	void VulkanRHI::prepareContext() {
		m_vk_current_command_buffer = m_vk_command_buffers[m_current_frame_index];
		((VulkanCommandBuffer*)m_current_command_buffer)->setResource(m_vk_current_command_buffer);
	}

	void VulkanRHI::clear() {
		if (_enable_validation_layers) {
			destroyDebugUtilsMessengerEXT(m_instance, _debug_messenger, nullptr);
		}
	}

	void VulkanRHI::waitForFences() {
		VkResult res_wait_for_fences = f_vkWaitForFences(
			m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index], 
			VK_TRUE, UINT64_MAX
		);
		if (res_wait_for_fences != VK_SUCCESS) {
			LOG_ERROR("failed to synchronize!");
		}
	}

	bool VulkanRHI::waitForFences(uint32_t fence_count, const RHIFence* const* fences, RHIBool32 wait_all, uint64_t timeout) {
		int fence_size = fence_count;
		std::vector<VkFence> vk_fence_list(fence_size);
		for (int i = 0; i < fence_size; ++i) {
			const auto& rhi_fence_element = fences[i];
			auto& vk_fence_element = vk_fence_list[i];
			vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
		}
		VkResult result = vkWaitForFences(m_device, fence_count, vk_fence_list.data(), wait_all, timeout);
		if (result == VK_SUCCESS) {
			return RHI_SUCCESS;
		}
		else {
			LOG_ERROR("waitForFences failed");
			return RHI_FALSE;
		}
	}

	void VulkanRHI::getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* properties) {
        VkPhysicalDeviceProperties vk_physical_device_properties;
        vkGetPhysicalDeviceProperties(m_physical_device, &vk_physical_device_properties);
        properties->apiVersion = vk_physical_device_properties.apiVersion;
        properties->driverVersion = vk_physical_device_properties.driverVersion;
        properties->vendorID = vk_physical_device_properties.vendorID;
        properties->deviceID = vk_physical_device_properties.deviceID;
        properties->deviceType = (RHIPhysicalDeviceType)vk_physical_device_properties.deviceType;
        for (uint32_t i = 0; i < RHI_MAX_PHYSICAL_DEVICE_NAME_SIZE; i++)
        {
            properties->deviceName[i] = vk_physical_device_properties.deviceName[i];
        }
        for (uint32_t i = 0; i < RHI_UUID_SIZE; i++)
        {
            properties->pipelineCacheUUID[i] = vk_physical_device_properties.pipelineCacheUUID[i];
        }
        properties->sparseProperties.residencyStandard2DBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DBlockShape;
        properties->sparseProperties.residencyStandard2DMultisampleBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DMultisampleBlockShape;
        properties->sparseProperties.residencyStandard3DBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard3DBlockShape;
        properties->sparseProperties.residencyAlignedMipSize = (VkBool32)vk_physical_device_properties.sparseProperties.residencyAlignedMipSize;
        properties->sparseProperties.residencyNonResidentStrict = (VkBool32)vk_physical_device_properties.sparseProperties.residencyNonResidentStrict;

        properties->limits.maxImageDimension1D = vk_physical_device_properties.limits.maxImageDimension1D;
        properties->limits.maxImageDimension2D = vk_physical_device_properties.limits.maxImageDimension2D;
        properties->limits.maxImageDimension3D = vk_physical_device_properties.limits.maxImageDimension3D;
        properties->limits.maxImageDimensionCube = vk_physical_device_properties.limits.maxImageDimensionCube;
        properties->limits.maxImageArrayLayers = vk_physical_device_properties.limits.maxImageArrayLayers;
        properties->limits.maxTexelBufferElements = vk_physical_device_properties.limits.maxTexelBufferElements;
        properties->limits.maxUniformBufferRange = vk_physical_device_properties.limits.maxUniformBufferRange;
        properties->limits.maxStorageBufferRange = vk_physical_device_properties.limits.maxStorageBufferRange;
        properties->limits.maxPushConstantsSize = vk_physical_device_properties.limits.maxPushConstantsSize;
        properties->limits.maxMemoryAllocationCount = vk_physical_device_properties.limits.maxMemoryAllocationCount;
        properties->limits.maxSamplerAllocationCount = vk_physical_device_properties.limits.maxSamplerAllocationCount;
        properties->limits.bufferImageGranularity = (VkDeviceSize)vk_physical_device_properties.limits.bufferImageGranularity;
        properties->limits.sparseAddressSpaceSize = (VkDeviceSize)vk_physical_device_properties.limits.sparseAddressSpaceSize;
        properties->limits.maxBoundDescriptorSets = vk_physical_device_properties.limits.maxBoundDescriptorSets;
        properties->limits.maxPerStageDescriptorSamplers = vk_physical_device_properties.limits.maxPerStageDescriptorSamplers;
        properties->limits.maxPerStageDescriptorUniformBuffers = vk_physical_device_properties.limits.maxPerStageDescriptorUniformBuffers;
        properties->limits.maxPerStageDescriptorStorageBuffers = vk_physical_device_properties.limits.maxPerStageDescriptorStorageBuffers;
        properties->limits.maxPerStageDescriptorSampledImages = vk_physical_device_properties.limits.maxPerStageDescriptorSampledImages;
        properties->limits.maxPerStageDescriptorStorageImages = vk_physical_device_properties.limits.maxPerStageDescriptorStorageImages;
        properties->limits.maxPerStageDescriptorInputAttachments = vk_physical_device_properties.limits.maxPerStageDescriptorInputAttachments;
        properties->limits.maxPerStageResources = vk_physical_device_properties.limits.maxPerStageResources;
        properties->limits.maxDescriptorSetSamplers = vk_physical_device_properties.limits.maxDescriptorSetSamplers;
        properties->limits.maxDescriptorSetUniformBuffers = vk_physical_device_properties.limits.maxDescriptorSetUniformBuffers;
        properties->limits.maxDescriptorSetUniformBuffersDynamic = vk_physical_device_properties.limits.maxDescriptorSetUniformBuffersDynamic;
        properties->limits.maxDescriptorSetStorageBuffers = vk_physical_device_properties.limits.maxDescriptorSetStorageBuffers;
        properties->limits.maxDescriptorSetStorageBuffersDynamic = vk_physical_device_properties.limits.maxDescriptorSetStorageBuffersDynamic;
        properties->limits.maxDescriptorSetSampledImages = vk_physical_device_properties.limits.maxDescriptorSetSampledImages;
        properties->limits.maxDescriptorSetStorageImages = vk_physical_device_properties.limits.maxDescriptorSetStorageImages;
        properties->limits.maxDescriptorSetInputAttachments = vk_physical_device_properties.limits.maxDescriptorSetInputAttachments;
        properties->limits.maxVertexInputAttributes = vk_physical_device_properties.limits.maxVertexInputAttributes;
        properties->limits.maxVertexInputBindings = vk_physical_device_properties.limits.maxVertexInputBindings;
        properties->limits.maxVertexInputAttributeOffset = vk_physical_device_properties.limits.maxVertexInputAttributeOffset;
        properties->limits.maxVertexInputBindingStride = vk_physical_device_properties.limits.maxVertexInputBindingStride;
        properties->limits.maxVertexOutputComponents = vk_physical_device_properties.limits.maxVertexOutputComponents;
        properties->limits.maxTessellationGenerationLevel = vk_physical_device_properties.limits.maxTessellationGenerationLevel;
        properties->limits.maxTessellationPatchSize = vk_physical_device_properties.limits.maxTessellationPatchSize;
        properties->limits.maxTessellationControlPerVertexInputComponents = vk_physical_device_properties.limits.maxTessellationControlPerVertexInputComponents;
        properties->limits.maxTessellationControlPerVertexOutputComponents = vk_physical_device_properties.limits.maxTessellationControlPerVertexOutputComponents;
        properties->limits.maxTessellationControlPerPatchOutputComponents = vk_physical_device_properties.limits.maxTessellationControlPerPatchOutputComponents;
        properties->limits.maxTessellationControlTotalOutputComponents = vk_physical_device_properties.limits.maxTessellationControlTotalOutputComponents;
        properties->limits.maxTessellationEvaluationInputComponents = vk_physical_device_properties.limits.maxTessellationEvaluationInputComponents;
        properties->limits.maxTessellationEvaluationOutputComponents = vk_physical_device_properties.limits.maxTessellationEvaluationOutputComponents;
        properties->limits.maxGeometryShaderInvocations = vk_physical_device_properties.limits.maxGeometryShaderInvocations;
        properties->limits.maxGeometryInputComponents = vk_physical_device_properties.limits.maxGeometryInputComponents;
        properties->limits.maxGeometryOutputComponents = vk_physical_device_properties.limits.maxGeometryOutputComponents;
        properties->limits.maxGeometryOutputVertices = vk_physical_device_properties.limits.maxGeometryOutputVertices;
        properties->limits.maxGeometryTotalOutputComponents = vk_physical_device_properties.limits.maxGeometryTotalOutputComponents;
        properties->limits.maxFragmentInputComponents = vk_physical_device_properties.limits.maxFragmentInputComponents;
        properties->limits.maxFragmentOutputAttachments = vk_physical_device_properties.limits.maxFragmentOutputAttachments;
        properties->limits.maxFragmentDualSrcAttachments = vk_physical_device_properties.limits.maxFragmentDualSrcAttachments;
        properties->limits.maxFragmentCombinedOutputResources = vk_physical_device_properties.limits.maxFragmentCombinedOutputResources;
        properties->limits.maxComputeSharedMemorySize = vk_physical_device_properties.limits.maxComputeSharedMemorySize;
        for (uint32_t i = 0; i < 3; i++)
        {
            properties->limits.maxComputeWorkGroupCount[i] = vk_physical_device_properties.limits.maxComputeWorkGroupCount[i];
        }
        properties->limits.maxComputeWorkGroupInvocations = vk_physical_device_properties.limits.maxComputeWorkGroupInvocations;
        for (uint32_t i = 0; i < 3; i++)
        {
            properties->limits.maxComputeWorkGroupSize[i] = vk_physical_device_properties.limits.maxComputeWorkGroupSize[i];
        }
        properties->limits.subPixelPrecisionBits = vk_physical_device_properties.limits.subPixelPrecisionBits;
        properties->limits.subTexelPrecisionBits = vk_physical_device_properties.limits.subTexelPrecisionBits;
        properties->limits.mipmapPrecisionBits = vk_physical_device_properties.limits.mipmapPrecisionBits;
        properties->limits.maxDrawIndexedIndexValue = vk_physical_device_properties.limits.maxDrawIndexedIndexValue;
        properties->limits.maxDrawIndirectCount = vk_physical_device_properties.limits.maxDrawIndirectCount;
        properties->limits.maxSamplerLodBias = vk_physical_device_properties.limits.maxSamplerLodBias;
        properties->limits.maxSamplerAnisotropy = vk_physical_device_properties.limits.maxSamplerAnisotropy;
        properties->limits.maxViewports = vk_physical_device_properties.limits.maxViewports;
        for (uint32_t i = 0; i < 2; i++)
        {
            properties->limits.maxViewportDimensions[i] = vk_physical_device_properties.limits.maxViewportDimensions[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            properties->limits.viewportBoundsRange[i] = vk_physical_device_properties.limits.viewportBoundsRange[i];
        }
        properties->limits.viewportSubPixelBits = vk_physical_device_properties.limits.viewportSubPixelBits;
        properties->limits.minMemoryMapAlignment = vk_physical_device_properties.limits.minMemoryMapAlignment;
        properties->limits.minTexelBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minTexelBufferOffsetAlignment;
        properties->limits.minUniformBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minUniformBufferOffsetAlignment;
        properties->limits.minStorageBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minStorageBufferOffsetAlignment;
        properties->limits.minTexelOffset = vk_physical_device_properties.limits.minTexelOffset;
        properties->limits.maxTexelOffset = vk_physical_device_properties.limits.maxTexelOffset;
        properties->limits.minTexelGatherOffset = vk_physical_device_properties.limits.minTexelGatherOffset;
        properties->limits.maxTexelGatherOffset = vk_physical_device_properties.limits.maxTexelGatherOffset;
        properties->limits.minInterpolationOffset = vk_physical_device_properties.limits.minInterpolationOffset;
        properties->limits.maxInterpolationOffset = vk_physical_device_properties.limits.maxInterpolationOffset;
        properties->limits.subPixelInterpolationOffsetBits = vk_physical_device_properties.limits.subPixelInterpolationOffsetBits;
        properties->limits.maxFramebufferWidth = vk_physical_device_properties.limits.maxFramebufferWidth;
        properties->limits.maxFramebufferHeight = vk_physical_device_properties.limits.maxFramebufferHeight;
        properties->limits.maxFramebufferLayers = vk_physical_device_properties.limits.maxFramebufferLayers;
        properties->limits.framebufferColorSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferColorSampleCounts;
        properties->limits.framebufferDepthSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferDepthSampleCounts;
        properties->limits.framebufferStencilSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferStencilSampleCounts;
        properties->limits.framebufferNoAttachmentsSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferNoAttachmentsSampleCounts;
        properties->limits.maxColorAttachments = vk_physical_device_properties.limits.maxColorAttachments;
        properties->limits.sampledImageColorSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageColorSampleCounts;
        properties->limits.sampledImageIntegerSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageIntegerSampleCounts;
        properties->limits.sampledImageDepthSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageDepthSampleCounts;
        properties->limits.sampledImageStencilSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageStencilSampleCounts;
        properties->limits.storageImageSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.storageImageSampleCounts;
        properties->limits.maxSampleMaskWords = vk_physical_device_properties.limits.maxSampleMaskWords;
        properties->limits.timestampComputeAndGraphics = (VkBool32)vk_physical_device_properties.limits.timestampComputeAndGraphics;
        properties->limits.timestampPeriod = vk_physical_device_properties.limits.timestampPeriod;
        properties->limits.maxClipDistances = vk_physical_device_properties.limits.maxClipDistances;
        properties->limits.maxCullDistances = vk_physical_device_properties.limits.maxCullDistances;
        properties->limits.maxCombinedClipAndCullDistances = vk_physical_device_properties.limits.maxCombinedClipAndCullDistances;
        properties->limits.discreteQueuePriorities = vk_physical_device_properties.limits.discreteQueuePriorities;
        for (uint32_t i = 0; i < 2; i++)
        {
            properties->limits.pointSizeRange[i] = vk_physical_device_properties.limits.pointSizeRange[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            properties->limits.lineWidthRange[i] = vk_physical_device_properties.limits.lineWidthRange[i];
        }
        properties->limits.pointSizeGranularity = vk_physical_device_properties.limits.pointSizeGranularity;
        properties->limits.lineWidthGranularity = vk_physical_device_properties.limits.lineWidthGranularity;
        properties->limits.strictLines = (VkBool32)vk_physical_device_properties.limits.strictLines;
        properties->limits.standardSampleLocations = (VkBool32)vk_physical_device_properties.limits.standardSampleLocations;
        properties->limits.optimalBufferCopyOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
        properties->limits.optimalBufferCopyRowPitchAlignment = (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
        properties->limits.nonCoherentAtomSize = (VkDeviceSize)vk_physical_device_properties.limits.nonCoherentAtomSize;
	}

    void VulkanRHI::resetCommandPool() {
        VkResult res_reset_command_pool = f_vkResetCommandPool(m_device, m_command_pools[m_current_frame_index], 0);
        if (res_reset_command_pool != VK_SUCCESS) {
            LOG_ERROR("failed to reset command pool");
        }
    }

    bool VulkanRHI::prepareBeforePass(std::function<void()> pass_update_after_recreate_swapchain) {
        VkResult acquire_image_result = vkAcquireNextImageKHR(
            m_device, m_swapchain, UINT64_MAX,
            m_image_available_for_render_semaphores[m_current_frame_index],
            VK_NULL_HANDLE, &m_current_swapchain_image_index
        );
        if (acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            pass_update_after_recreate_swapchain();
            return RHI_SUCCESS;
        }
        else if (acquire_image_result == VK_SUBOPTIMAL_KHR) {
            recreateSwapchain();
            pass_update_after_recreate_swapchain();
            VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &m_image_available_for_render_semaphores[m_current_frame_index];
            submit_info.pWaitDstStageMask = wait_stages;
            submit_info.commandBufferCount = 0;
            submit_info.pCommandBuffers = nullptr;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = nullptr;
            VkResult res_reset_fences = f_vkResetFences(m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index]);
            if (res_reset_fences != VK_SUCCESS) {
                LOG_ERROR("f_vkResetFences failed!");
                return RHI_FALSE;
            }
            VkResult res_queue_submit = vkQueueSubmit(
                ((VulkanQueue*)m_graphics_queue)->getResource(), 1, &submit_info,
                m_is_frame_in_flight_fences[m_current_frame_index]);
            if (res_queue_submit != VK_SUCCESS) {
                LOG_ERROR("vkQueueSubmit failed!");
                return RHI_FALSE;
            }
            m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;
            return RHI_SUCCESS;
        }
        else {
            if (acquire_image_result != VK_SUCCESS) {
                LOG_ERROR("vkAcquireNextImageKHR failed!");
                return RHI_FALSE;
            }
        }
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        VkResult res_begin_command_buffer = f_vkBeginCommandBuffer(m_vk_command_buffers[m_current_frame_index], &command_buffer_begin_info);
        if (res_begin_command_buffer != VK_SUCCESS) {
            LOG_ERROR("f_vkBeginCommandBuffer failed!");
            return RHI_FALSE;
        }
        return RHI_FALSE;
    }

    void VulkanRHI::submitRendering(std::function<void()> pass_update_after_recreate_swapchain) {
        VkResult res_end_command_buffer = f_vkEndCommandBuffer(m_vk_command_buffers[m_current_frame_index]);
        if (res_end_command_buffer != VK_SUCCESS) {
            LOG_ERROR("f_vkEndCommandBuffer failed!");
            return;
        }
        VkSemaphore semaphores[2] = {
            ((VulkanSemaphore*)m_image_available_for_texturescopy_semaphores[m_current_frame_index])->getResource(),
            m_image_finished_for_presentation_samaphores[m_current_frame_index]
        };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &m_image_available_for_render_semaphores[m_current_frame_index];
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_vk_command_buffers[m_current_frame_index];
        submit_info.signalSemaphoreCount = 2;
        submit_info.pSignalSemaphores = semaphores;
        VkResult res_reset_fences = f_vkResetFences(m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index]);
        if (res_reset_fences != VK_SUCCESS) {
            LOG_ERROR("f_vkResetFences failed!");
            return;
        }
        VkResult res_queue_submit = vkQueueSubmit(
            ((VulkanQueue*)m_graphics_queue)->getResource(), 1, &submit_info,
            m_is_frame_in_flight_fences[m_current_frame_index]
        );
        if (res_queue_submit != VK_SUCCESS) {
            LOG_ERROR("vkQueueSubmit failed!");
            return;
        }
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_image_finished_for_presentation_samaphores[m_current_frame_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_swapchain;
        present_info.pImageIndices = &m_current_swapchain_image_index;
        VkResult present_result = vkQueuePresentKHR(m_present_queue, &present_info);
        if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
            recreateSwapchain();
            pass_update_after_recreate_swapchain();
        }
        else {
            if (present_result != VK_SUCCESS) {
                LOG_ERROR("vkQueuePresentKHR failed!");
                return;
            }
        }
        m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;
    }

    RHICommandBuffer* VulkanRHI::beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = ((VulkanCommandPool*)m_rhi_command_pool)->getResource();
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(m_device, &allocInfo, &command_buffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        f_vkBeginCommandBuffer(command_buffer, &beginInfo);
        RHICommandBuffer* rhi_command_buffer = new VulkanCommandBuffer();
        ((VulkanCommandBuffer*)rhi_command_buffer)->setResource(command_buffer);
        return rhi_command_buffer;
    }

    void VulkanRHI::endSingleTimeCommands(RHICommandBuffer* command_buffer) {
        VkCommandBuffer vk_command_buffer = ((VulkanCommandBuffer*)command_buffer)->getResource();
        f_vkEndCommandBuffer(vk_command_buffer);
        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &vk_command_buffer;
        vkQueueSubmit(((VulkanQueue*)m_graphics_queue)->getResource(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(((VulkanQueue*)m_graphics_queue)->getResource());
        vkFreeCommandBuffers(m_device, ((VulkanCommandPool*)m_rhi_command_pool)->getResource(), 1, &vk_command_buffer);
        delete(command_buffer);
    }

    bool VulkanRHI::checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (const char* layer_name : _validation_layers) {
            bool layer_found = false;
            for (const auto& layer_properties : availableLayers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }
            if (!layer_found) {
                return RHI_FALSE;
            }
        }
        return RHI_SUCCESS;
    }

    std::vector<const char*> VulkanRHI::getRequredExtensions() {
        uint32_t glfwExtentionCount = 0;
        const char** glfwExtendions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);
        std::vector<const char*> extensions(glfwExtendions, glfwExtendions + glfwExtentionCount);
        if (_enable_validation_layers || _enable_debug_utils_label) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void*
    ) {
        std::cerr << "validation layer: " << callback_data->pMessage << std::endl;
        return VK_FALSE;
    }

    void VulkanRHI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void VulkanRHI::createInstance() {
        if (_enable_validation_layers && !checkValidationLayerSupport()) {
            LOG_ERROR("validation layer requested, but not available!");
        }
        _vulkan_api_version = VK_API_VERSION_1_0;

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "dao_renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Dao";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = _vulkan_api_version;

        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &appInfo;

        std::vector<const char*> extensions = getRequredExtensions();
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instance_create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (_enable_validation_layers) {
            instance_create_info.enabledLayerCount = static_cast<uint32_t>(_validation_layers.size());
            instance_create_info.ppEnabledLayerNames = _validation_layers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.pNext = nullptr;
        }

        if (vkCreateInstance(&instance_create_info, nullptr, &m_instance) != VK_SUCCESS) {
            LOG_ERROR("vkCreateInstance failed!");
        }
    }

    void VulkanRHI::initializeDebugMessenger() {
        if (_enable_validation_layers) {
            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            populateDebugMessengerCreateInfo(createInfo);
            if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &_debug_messenger) != VK_SUCCESS) {
                LOG_ERROR("failed to set up debug messenger!");
            }
        }
        if (_enable_debug_utils_label) {
            f_vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdBeginDebugUtilsLabelEXT");
            f_vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_instance, "vkCmdEndDebugUtilsLabelEXT");
        }
    }

    void VulkanRHI::createWindowSurface() {
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
            LOG_ERROR("glfwCreateWindowSurface failed!");
        }
    }

    void VulkanRHI::initializePhysicalDevice() {
        uint32_t physical_device_count;
        vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr);
        if (physical_device_count == 0) {
            LOG_ERROR("vkEnumeratePhysicalDevices failed!");
        }
        else {
            std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
            vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices.data());
            std::vector<std::pair<int, VkPhysicalDevice>> ranked_physical_devices;
            for (const auto& device : physical_devices) {
                VkPhysicalDeviceProperties physical_device_properties;
                vkGetPhysicalDeviceProperties(device, &physical_device_properties);
                int score = 0;
                if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    score += 1000;
                }
                else if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                    score += 100;
                }
                ranked_physical_devices.push_back({ score,device });
            }
            std::sort(
                ranked_physical_devices.begin(),
                ranked_physical_devices.end(),
                [](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2) {
                    return p1.first > p2.first;
                });
            for (const auto& device : ranked_physical_devices) {
                if (isDeviceSuitable(device.second)) {
                    m_physical_device = device.second;
                    break;
                }
            }
            if (m_physical_device == VK_NULL_HANDLE) {
                LOG_ERROR("failed to find suitable physical device");
            }
        }
    }

    void VulkanRHI::createLogicalDevice() {
        m_queue_indices = findQueueFamilies(m_physical_device);
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> queue_families = {
            m_queue_indices.graphics_family.value(),
            m_queue_indices.present_family.value(),
            m_queue_indices.compute_family.value()
        };
        float queue_priority = 1.0f;
        for (uint32_t queue_family : queue_families) {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures physical_device_feature = {};
        physical_device_feature.samplerAnisotropy = VK_TRUE;
        physical_device_feature.fragmentStoresAndAtomics = VK_TRUE;
        physical_device_feature.independentBlend = VK_TRUE;
        if (_enable_point_light_shadow) {
            physical_device_feature.geometryShader = VK_TRUE;
        }

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.pEnabledFeatures = &physical_device_feature;
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(_device_extensions.size());
        device_create_info.ppEnabledExtensionNames = _device_extensions.data();
        device_create_info.enabledLayerCount = 0;

        if (vkCreateDevice(m_physical_device, &device_create_info, nullptr, &m_device) != VK_SUCCESS) {
            LOG_ERROR("vkCreateDevice failed!");
        }

        VkQueue vk_graphic_queue;
        vkGetDeviceQueue(m_device, m_queue_indices.graphics_family.value(), 0, &vk_graphic_queue);
        m_graphics_queue = new VulkanQueue();
        ((VulkanQueue*)m_graphics_queue)->setResource(vk_graphic_queue);

        vkGetDeviceQueue(m_device, m_queue_indices.present_family.value(), 0, &m_present_queue);

        VkQueue vk_compute_queue;
        vkGetDeviceQueue(m_device, m_queue_indices.compute_family.value(), 0, &vk_compute_queue);
        m_compute_queue = new VulkanQueue();
        ((VulkanQueue*)m_compute_queue)->setResource(vk_compute_queue);

        f_vkResetCommandPool = (PFN_vkResetCommandPool)vkGetDeviceProcAddr(m_device, "vkResetCommandPool");
        f_vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(m_device, "vkBeginCommandBuffer");
        f_vkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(m_device, "vkEndCommandBuffer");
        f_vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(m_device, "vkCmdBeginRenderPass");
        f_vkCmdNextSubpass = (PFN_vkCmdNextSubpass)vkGetDeviceProcAddr(m_device, "vkCmdNextSubpass");
        f_vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(m_device, "vkCmdEndRenderPass");
        f_vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(m_device, "vkCmdBindPipeline");
        f_vkCmdSetViewport = (PFN_vkCmdSetViewport)vkGetDeviceProcAddr(m_device, "vkCmdSetViewport");
        f_vkCmdSetScissor = (PFN_vkCmdSetScissor)vkGetDeviceProcAddr(m_device, "vkCmdSetScissor");
        f_vkWaitForFences = (PFN_vkWaitForFences)vkGetDeviceProcAddr(m_device, "vkWaitForFences");
        f_vkResetFences = (PFN_vkResetFences)vkGetDeviceProcAddr(m_device, "vkResetFences");
        f_vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(m_device, "vkCmdDrawIndexed");
        f_vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(m_device, "vkCmdBindVertexBuffers");
        f_vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(m_device, "vkCmdBindIndexBuffer");
        f_vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(m_device, "vkCmdBindDescriptorSets");
        f_vkCmdClearAttachments = (PFN_vkCmdClearAttachments)vkGetDeviceProcAddr(m_device, "vkCmdClearAttachments");

        m_depth_image_format = (RHIFormat)findDepthFormat();
    }

    void VulkanRHI::createCommandPool() {
        m_rhi_command_pool = new VulkanCommandPool();
        VkCommandPool vk_command_pool;
        VkCommandPoolCreateInfo command_pool_create_info{};
        command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.pNext = nullptr;
        command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        command_pool_create_info.queueFamilyIndex = m_queue_indices.graphics_family.value();
        if (vkCreateCommandPool(m_device, &command_pool_create_info, nullptr, &vk_command_pool) != VK_SUCCESS) {
            LOG_ERROR("vkCreateCommandPool failed!");
        }
        ((VulkanCommandPool*)m_rhi_command_pool)->setResource(vk_command_pool);

        VkCommandPoolCreateInfo command_pool_create_info_transient;
        command_pool_create_info_transient.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info_transient.pNext = nullptr;
        command_pool_create_info_transient.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        command_pool_create_info_transient.queueFamilyIndex = m_queue_indices.graphics_family.value();
        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i) {
            if (vkCreateCommandPool(m_device, &command_pool_create_info_transient, nullptr, &m_command_pools[i]) != VK_SUCCESS) {
                LOG_ERROR("vkCreateCommandPool failed!");
            }
        }
    }

    bool VulkanRHI::createCommandPool(const RHICommandPoolCreateInfo* create_info, RHICommandPool*& command_pool) {
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkCommandPoolCreateFlags)create_info->flags;
        createInfo.queueFamilyIndex = create_info->queueFamilyIndex;

        command_pool = new VulkanCommandPool();
        VkCommandPool vk_command_pool;
        if (vkCreateCommandPool(m_device, &createInfo, nullptr, &vk_command_pool) == VK_SUCCESS) {
            ((VulkanCommandPool*)command_pool)->setResource(vk_command_pool);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateCommandPool failed!");
            delete(command_pool);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createDescriptorPool(const RHIDescriptorPoolCreateInfo* create_info, RHIDescriptorPool*& descriptor_pool) {
        int size = create_info->poolSizeCount;
        std::vector<VkDescriptorPoolSize> descriptor_pool_size(size);
        for (int i = 0; i < size; ++i) {
            const auto& rhi_desc = create_info->pPoolSizes[i];
            auto& vk_desc = descriptor_pool_size[i];
            vk_desc.type = (VkDescriptorType)rhi_desc.type;
            vk_desc.descriptorCount = rhi_desc.descriptorCount;
        }
        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkDescriptorPoolCreateFlags)create_info->flags;
        createInfo.maxSets = create_info->maxSets;
        createInfo.poolSizeCount = create_info->poolSizeCount;
        createInfo.pPoolSizes = descriptor_pool_size.data();
        descriptor_pool = new VulkanDescriptorPool();
        VkDescriptorPool vk_descriptor_pool;
        if (vkCreateDescriptorPool(m_device, &createInfo, nullptr, &vk_descriptor_pool) == VK_SUCCESS) {
            ((VulkanDescriptorPool*)descriptor_pool)->setResource(vk_descriptor_pool);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateDescriptorPool failed!");
            delete(descriptor_pool);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* create_info, RHIDescriptorSetLayout*& set_layout) {
        int descriptor_set_layout_binding_size = create_info->bindingCount;
        std::vector<VkDescriptorSetLayoutBinding> vk_descriptor_set_layout_binding_list(descriptor_set_layout_binding_size);
        
        int sampler_count = 0;
        for (int i = 0; i < descriptor_set_layout_binding_size; ++i) {
            const auto& rhi_descriptor_set_layout_binding_element = create_info->pBindings[i];
            if (rhi_descriptor_set_layout_binding_element.pImmutableSamplers != nullptr) {
                sampler_count += rhi_descriptor_set_layout_binding_element.descriptorCount;
            }
        }
        std::vector<VkSampler> sampler_list(sampler_count);
        int sampler_current = 0;
        for (int i = 0; i < descriptor_set_layout_binding_size; ++i) {
            const auto& rhi_descriptor_set_layout_binding_element = create_info->pBindings[i];
            auto& vk_descriptor_set_layout_binding_element = vk_descriptor_set_layout_binding_list[i];
            vk_descriptor_set_layout_binding_element.pImmutableSamplers = nullptr;
            if (rhi_descriptor_set_layout_binding_element.pImmutableSamplers) {
                vk_descriptor_set_layout_binding_element.pImmutableSamplers = &sampler_list[sampler_current];
                for (int j = 0; j < rhi_descriptor_set_layout_binding_element.descriptorCount; ++j) {
                    const auto& rhi_sampler_element = rhi_descriptor_set_layout_binding_element.pImmutableSamplers[j];
                    auto& vk_sampler_element = sampler_list[sampler_current];
                    vk_sampler_element = ((VulkanSampler*)rhi_sampler_element)->getResource();
                    sampler_current++;
                }
            }
            vk_descriptor_set_layout_binding_element.binding = rhi_descriptor_set_layout_binding_element.binding;
            vk_descriptor_set_layout_binding_element.descriptorType = (VkDescriptorType)rhi_descriptor_set_layout_binding_element.descriptorType;
            vk_descriptor_set_layout_binding_element.descriptorCount = rhi_descriptor_set_layout_binding_element.descriptorCount;
            vk_descriptor_set_layout_binding_element.stageFlags = rhi_descriptor_set_layout_binding_element.stageFlags;
        }
        if (sampler_count != sampler_current) {
            LOG_ERROR("sampler_count != sampler_current");
            return RHI_FALSE;
        }

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkDescriptorSetLayoutCreateFlags)create_info->flags;
        createInfo.bindingCount = create_info->bindingCount;
        createInfo.pBindings = vk_descriptor_set_layout_binding_list.data();

        set_layout = new VulkanDescriptorSetLayout();
        VkDescriptorSetLayout vk_descriptor_set_layout;
        if (vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &vk_descriptor_set_layout) == VK_SUCCESS) {
            ((VulkanDescriptorSetLayout*)set_layout)->setResource(vk_descriptor_set_layout);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateDescriptorSetLayout failed!");
            delete(set_layout);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createFence(const RHIFenceCreateInfo* create_info, RHIFence*& fence) {
        VkFenceCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkFenceCreateFlags)create_info->flags;
        fence = new VulkanFence();
        VkFence vk_fence;
        if (vkCreateFence(m_device, &createInfo, nullptr, &vk_fence) == VK_SUCCESS) {
            ((VulkanFence*)fence)->setResource(vk_fence);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateFence failed!");
            delete(fence);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createFramebuffer(const RHIFramebufferCreateInfo* create_info, RHIFramebuffer*& frame_buffer) {
        int image_view_size = create_info->attachmentCount;
        std::vector<VkImageView> vk_image_view_list(image_view_size);
        for (int i = 0; i < image_view_size; ++i) {
            const auto& rhi_image_view_element = create_info->pAttachments[i];
            auto& vk_image_view_element = vk_image_view_list[i];
            vk_image_view_element = ((VulkanImageView*)rhi_image_view_element)->getResource();
        }

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkFramebufferCreateFlags)create_info->flags;
        createInfo.renderPass = ((VulkanRenderPass*)create_info->renderPass)->getResource();
        createInfo.attachmentCount = create_info->attachmentCount;
        createInfo.pAttachments = vk_image_view_list.data();
        createInfo.width = create_info->width;
        createInfo.height = create_info->height;
        createInfo.layers = create_info->layers;

        frame_buffer = new VulkanFramebuffer();
        VkFramebuffer vk_framebuffer;
        if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &vk_framebuffer) == VK_SUCCESS) {
            ((VulkanFramebuffer*)frame_buffer)->setResource(vk_framebuffer);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateFramebuffer failed");
            delete(frame_buffer);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createGraphicsPipelines(
        RHIPipelineCache* pipeline_cache,
        uint32_t create_info_count,
        const RHIGraphicsPipelineCreateInfo* create_infos,
        RHIPipeline*& pipelines
    ) {
        std::vector<VkGraphicsPipelineCreateInfo> createInfos(create_info_count);
        std::vector<std::vector<VkSpecializationInfo>> specializationInfoListArrays(create_info_count);
        std::vector<std::vector<VkSpecializationMapEntry>> specializationMapEntryListArrays(create_info_count);
        std::vector<std::vector<VkPipelineShaderStageCreateInfo>> shaderStageListArrays(create_info_count);
        std::vector<std::vector<VkVertexInputBindingDescription>> vertexBindingDescriptionListArrays(create_info_count);
        std::vector<std::vector<VkVertexInputAttributeDescription>> vertexAttributeDescriptionListArrays(create_info_count);
        std::vector<VkPipelineVertexInputStateCreateInfo> vertexInputStateList(create_info_count);
        std::vector<VkPipelineInputAssemblyStateCreateInfo> inputAssemblyStateList(create_info_count);
        std::vector<VkPipelineTessellationStateCreateInfo*> tessellationStatePtrList(create_info_count);
        std::vector<VkPipelineTessellationStateCreateInfo> tessellationStateList(create_info_count);
        std::vector<std::vector<VkViewport>> viewportListArrays(create_info_count);
        std::vector<std::vector<VkRect2D>> rects2DListArrays(create_info_count);
        std::vector<VkPipelineViewportStateCreateInfo> viewportStateList(create_info_count);
        std::vector<VkPipelineRasterizationStateCreateInfo> rasterizationStateList(create_info_count);
        std::vector<VkPipelineMultisampleStateCreateInfo> multisampleStateList(create_info_count);
        std::vector<VkPipelineDepthStencilStateCreateInfo> depthStencilStateList(create_info_count);
        std::vector<std::vector<VkPipelineColorBlendAttachmentState>> colorBlendAttachmentStateListArrays(create_info_count);
        std::vector<VkPipelineColorBlendStateCreateInfo> colorBlendStateList(create_info_count);
        std::vector<std::vector<VkDynamicState>> dynamincStateListArrays(create_info_count);
        std::vector<VkPipelineDynamicStateCreateInfo> dynamincStateList(create_info_count);
        for (int k = 0; k < create_info_count; ++k) {
            const auto& create_info = &create_infos[k];
            int pipeline_shader_stage_create_info_size = create_info->stageCount;
            std::vector<VkPipelineShaderStageCreateInfo>& vk_pipeline_shader_stage_create_info_list = shaderStageListArrays[k];
            vk_pipeline_shader_stage_create_info_list.resize(pipeline_shader_stage_create_info_size);
            int specialization_map_entry_size_total = 0;
            int specialization_info_total = 0;
            for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i) {
                const auto& rhi_pipeline_shader_stage_create_info_element = create_info->pStages[i];
                if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr) {
                    specialization_info_total++;
                    specialization_map_entry_size_total += rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                }
            }
            std::vector<VkSpecializationInfo>& vk_specialization_info_list = specializationInfoListArrays[k];
            vk_specialization_info_list.resize(specialization_info_total);
            std::vector<VkSpecializationMapEntry>& vk_specialization_map_entry_list = specializationMapEntryListArrays[k];
            vk_specialization_map_entry_list.resize(specialization_map_entry_size_total);
            int specialization_map_entry_current = 0;
            int specialization_info_current = 0;
            for (int i = 0; i < pipeline_shader_stage_create_info_size; ++i) {
                const auto& rhi_pipeline_shader_stage_create_info_element = create_info->pStages[i];
                auto& vk_pipeline_shader_stage_create_info_element = vk_pipeline_shader_stage_create_info_list[i];
                if (rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo != nullptr) {
                    auto& vk_specializtion_info = vk_specialization_info_list[specialization_info_current];
                    vk_specializtion_info.mapEntryCount = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount;
                    vk_specializtion_info.pMapEntries = &vk_specialization_map_entry_list[specialization_map_entry_current];
                    vk_specializtion_info.dataSize = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->dataSize;
                    vk_specializtion_info.pData = (const void*)rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pData;
                    vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = &vk_specialization_info_list[specialization_info_current];
                    for (int j = 0; j < rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->mapEntryCount; ++j) {
                        const auto& rhi_specialization_map_entry_element = rhi_pipeline_shader_stage_create_info_element.pSpecializationInfo->pMapEntries[j];
                        auto& vk_specialization_map_entry_element = vk_specialization_map_entry_list[specialization_map_entry_current];
                        vk_specialization_map_entry_element.constantID = rhi_specialization_map_entry_element.constantID;
                        vk_specialization_map_entry_element.offset = rhi_specialization_map_entry_element.offset;
                        vk_specialization_map_entry_element.size = rhi_specialization_map_entry_element.size;
                        specialization_map_entry_current++;
                    }
                    specialization_info_current++;
                }
                else {
                    vk_pipeline_shader_stage_create_info_element.pSpecializationInfo = nullptr;
                }
                vk_pipeline_shader_stage_create_info_element.sType = (VkStructureType)rhi_pipeline_shader_stage_create_info_element.sType;
                vk_pipeline_shader_stage_create_info_element.pNext = (const void*)rhi_pipeline_shader_stage_create_info_element.pNext;
                vk_pipeline_shader_stage_create_info_element.flags = (VkPipelineShaderStageCreateFlags)rhi_pipeline_shader_stage_create_info_element.flags;
                vk_pipeline_shader_stage_create_info_element.stage = (VkShaderStageFlagBits)rhi_pipeline_shader_stage_create_info_element.stage;
                vk_pipeline_shader_stage_create_info_element.module = ((VulkanShader*)rhi_pipeline_shader_stage_create_info_element.module)->getResource();
                vk_pipeline_shader_stage_create_info_element.pName = rhi_pipeline_shader_stage_create_info_element.pName;
            }
            if ((specialization_map_entry_size_total != specialization_map_entry_current) || (specialization_info_total != specialization_info_current)) {
                LOG_ERROR("(specialization_map_entry_size_total != specialization_map_entry_current) && (specialization_info_total != specialization_info_current)");
                return RHI_FALSE;
            }

            int vertex_input_binding_description_size = create_info->pVertexInputState->vertexBindingDescriptionCount;
            std::vector<VkVertexInputBindingDescription>& vk_vertex_input_binding_description_list = vertexBindingDescriptionListArrays[k];
            vk_vertex_input_binding_description_list.resize(vertex_input_binding_description_size);
            for (int i = 0; i < vertex_input_binding_description_size; ++i) {
                const auto& rhi_vertex_input_binding_decription_element = create_info->pVertexInputState->pVertexBindingDescriptions[i];
                auto& vk_vertex_input_binding_decription_element = vk_vertex_input_binding_description_list[i];
                vk_vertex_input_binding_decription_element.binding = rhi_vertex_input_binding_decription_element.binding;
                vk_vertex_input_binding_decription_element.stride = rhi_vertex_input_binding_decription_element.stride;
                vk_vertex_input_binding_decription_element.inputRate = (VkVertexInputRate)rhi_vertex_input_binding_decription_element.inputRate;
            }
            int vertex_input_attribute_description_size = create_info->pVertexInputState->vertexAttributeDescriptionCount;
            std::vector<VkVertexInputAttributeDescription>& vk_vertex_input_attribute_description_list = vertexAttributeDescriptionListArrays[k];
            vk_vertex_input_attribute_description_list.resize(vertex_input_attribute_description_size);
            for (int i = 0; i < vertex_input_attribute_description_size; ++i) {
                const auto& rhi_vertex_input_attribute_description_element = create_info->pVertexInputState->pVertexAttributeDescriptions[i];
                auto& vk_vertex_input_attribute_description_element = vk_vertex_input_attribute_description_list[i];
                vk_vertex_input_attribute_description_element.location = rhi_vertex_input_attribute_description_element.location;
                vk_vertex_input_attribute_description_element.binding = rhi_vertex_input_attribute_description_element.binding;
                vk_vertex_input_attribute_description_element.format = (VkFormat)rhi_vertex_input_attribute_description_element.format;
                vk_vertex_input_attribute_description_element.offset = rhi_vertex_input_attribute_description_element.offset;
            }
            VkPipelineVertexInputStateCreateInfo& vk_pipeline_vertex_input_state_create_info = vertexInputStateList[k];
            vk_pipeline_vertex_input_state_create_info.sType = (VkStructureType)create_info->pVertexInputState->sType;
            vk_pipeline_vertex_input_state_create_info.pNext = (const void*)create_info->pVertexInputState->pNext;
            vk_pipeline_vertex_input_state_create_info.flags = (VkPipelineVertexInputStateCreateFlags)create_info->pVertexInputState->flags;
            vk_pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = vertex_input_binding_description_size;
            vk_pipeline_vertex_input_state_create_info.pVertexBindingDescriptions = vk_vertex_input_binding_description_list.data();
            vk_pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_input_attribute_description_size;
            vk_pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = vk_vertex_input_attribute_description_list.data();

            VkPipelineInputAssemblyStateCreateInfo& vk_pipeline_input_assembly_state_create_info = inputAssemblyStateList[k];
            vk_pipeline_input_assembly_state_create_info.sType = (VkStructureType)create_info->pInputAssemblyState->sType;
            vk_pipeline_input_assembly_state_create_info.pNext = (const void*)create_info->pInputAssemblyState->pNext;
            vk_pipeline_input_assembly_state_create_info.flags = (VkPipelineInputAssemblyStateCreateFlags)create_info->pInputAssemblyState->flags;
            vk_pipeline_input_assembly_state_create_info.topology = (VkPrimitiveTopology)create_info->pInputAssemblyState->topology;
            vk_pipeline_input_assembly_state_create_info.primitiveRestartEnable = (VkBool32)create_info->pInputAssemblyState->primitiveRestartEnable;

            VkPipelineTessellationStateCreateInfo*& vk_pipeline_tessellation_state_create_info_ptr = tessellationStatePtrList[k];
            VkPipelineTessellationStateCreateInfo& vk_pipeline_tessellation_state_create_info = tessellationStateList[k];
            if (create_info->pTessellationState != nullptr) {
                vk_pipeline_tessellation_state_create_info.sType = (VkStructureType)create_info->pTessellationState->sType;
                vk_pipeline_tessellation_state_create_info.pNext = (const void*)create_info->pTessellationState->pNext;
                vk_pipeline_tessellation_state_create_info.flags = (VkPipelineTessellationStateCreateFlags)create_info->pTessellationState->flags;
                vk_pipeline_tessellation_state_create_info.patchControlPoints = create_info->pTessellationState->patchControlPoints;
                vk_pipeline_tessellation_state_create_info_ptr = &vk_pipeline_tessellation_state_create_info;
            }

            int viewport_size = create_info->pViewportState->viewportCount;
            std::vector<VkViewport>& vk_viewport_list = viewportListArrays[k];
            vk_viewport_list.resize(viewport_size);
            for (int i = 0; i < viewport_size; ++i) {
                const auto& rhi_viewport_element = create_info->pViewportState->pViewports[i];
                auto& vk_viewport_element = vk_viewport_list[i];
                vk_viewport_element.x = rhi_viewport_element.x;
                vk_viewport_element.y = rhi_viewport_element.y;
                vk_viewport_element.width = rhi_viewport_element.width;
                vk_viewport_element.height = rhi_viewport_element.height;
                vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
                vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
            }
            int rect_2d_size = create_info->pViewportState->scissorCount;
            std::vector<VkRect2D>& vk_rect_2d_list = rects2DListArrays[k];
            vk_rect_2d_list.resize(rect_2d_size);
            for (int i = 0; i < rect_2d_size; ++i) {
                const auto& rhi_rect_2d_element = create_info->pViewportState->pScissors[i];
                auto& vk_rect_2d_element = vk_rect_2d_list[i];
                VkOffset2D offset2d{};
                offset2d.x = rhi_rect_2d_element.offset.x;
                offset2d.y = rhi_rect_2d_element.offset.y;
                VkExtent2D extend2d{};
                extend2d.width = rhi_rect_2d_element.extent.width;
                extend2d.height = rhi_rect_2d_element.extent.height;
                vk_rect_2d_element.offset = offset2d;
                vk_rect_2d_element.extent = extend2d;
            }
            VkPipelineViewportStateCreateInfo& vk_pipeline_viewport_state_create_info = viewportStateList[k];
            vk_pipeline_viewport_state_create_info.sType = (VkStructureType)create_info->pViewportState->sType;
            vk_pipeline_viewport_state_create_info.pNext = (const void*)create_info->pViewportState->pNext;
            vk_pipeline_viewport_state_create_info.flags = (VkPipelineViewportStateCreateFlags)create_info->pVertexInputState->flags;
            vk_pipeline_viewport_state_create_info.viewportCount = viewport_size;
            vk_pipeline_viewport_state_create_info.pViewports = vk_viewport_list.data();
            vk_pipeline_viewport_state_create_info.scissorCount = rect_2d_size;
            vk_pipeline_viewport_state_create_info.pScissors = vk_rect_2d_list.data();

            VkPipelineRasterizationStateCreateInfo& vk_pipeline_rasterization_state_create_info = rasterizationStateList[k];
            vk_pipeline_rasterization_state_create_info.sType = (VkStructureType)create_info->pRasterizationState->sType;
            vk_pipeline_rasterization_state_create_info.pNext = (const void*)create_info->pRasterizationState->pNext;
            vk_pipeline_rasterization_state_create_info.flags = (VkPipelineRasterizationStateCreateFlags)create_info->pRasterizationState->flags;
            vk_pipeline_rasterization_state_create_info.depthClampEnable = (VkBool32)create_info->pRasterizationState->depthClampEnable;
            vk_pipeline_rasterization_state_create_info.rasterizerDiscardEnable = (VkBool32)create_info->pRasterizationState->rasterizerDiscardEnable;
            vk_pipeline_rasterization_state_create_info.polygonMode = (VkPolygonMode)create_info->pRasterizationState->polygonMode;
            vk_pipeline_rasterization_state_create_info.cullMode = (VkCullModeFlags)create_info->pRasterizationState->cullMode;
            vk_pipeline_rasterization_state_create_info.frontFace = (VkFrontFace)create_info->pRasterizationState->frontFace;
            vk_pipeline_rasterization_state_create_info.depthBiasEnable = (VkBool32)create_info->pRasterizationState->depthBiasEnable;
            vk_pipeline_rasterization_state_create_info.depthBiasConstantFactor = create_info->pRasterizationState->depthBiasConstantFactor;
            vk_pipeline_rasterization_state_create_info.depthBiasClamp = create_info->pRasterizationState->depthBiasClamp;
            vk_pipeline_rasterization_state_create_info.depthBiasSlopeFactor = create_info->pRasterizationState->depthBiasSlopeFactor;
            vk_pipeline_rasterization_state_create_info.lineWidth = create_info->pRasterizationState->lineWidth;

            VkPipelineMultisampleStateCreateInfo& vk_pipeline_multisample_state_create_info = multisampleStateList[k];
            vk_pipeline_multisample_state_create_info.sType = (VkStructureType)create_info->pMultisampleState->sType;
            vk_pipeline_multisample_state_create_info.pNext = (const void*)create_info->pMultisampleState->pNext;
            vk_pipeline_multisample_state_create_info.flags = (VkPipelineMultisampleStateCreateFlags)create_info->pMultisampleState->flags;
            vk_pipeline_multisample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)create_info->pMultisampleState->rasterizationSamples;
            vk_pipeline_multisample_state_create_info.sampleShadingEnable = (VkBool32)create_info->pMultisampleState->sampleShadingEnable;
            vk_pipeline_multisample_state_create_info.minSampleShading = create_info->pMultisampleState->minSampleShading;
            vk_pipeline_multisample_state_create_info.pSampleMask = (const RHISampleMask*)create_info->pMultisampleState->pSampleMask;
            vk_pipeline_multisample_state_create_info.alphaToCoverageEnable = (VkBool32)create_info->pMultisampleState->alphaToCoverageEnable;
            vk_pipeline_multisample_state_create_info.alphaToOneEnable = (VkBool32)create_info->pMultisampleState->alphaToOneEnable;

            VkStencilOpState stencil_op_state_front{};
            stencil_op_state_front.failOp = (VkStencilOp)create_info->pDepthStencilState->front.failOp;
            stencil_op_state_front.passOp = (VkStencilOp)create_info->pDepthStencilState->front.passOp;
            stencil_op_state_front.depthFailOp = (VkStencilOp)create_info->pDepthStencilState->front.depthFailOp;
            stencil_op_state_front.compareOp = (VkCompareOp)create_info->pDepthStencilState->front.compareOp;
            stencil_op_state_front.compareMask = create_info->pDepthStencilState->front.compareMask;
            stencil_op_state_front.writeMask = create_info->pDepthStencilState->front.writeMask;
            stencil_op_state_front.reference = create_info->pDepthStencilState->front.reference;
            VkStencilOpState stencil_op_state_back{};
            stencil_op_state_back.failOp = (VkStencilOp)create_info->pDepthStencilState->back.failOp;
            stencil_op_state_back.passOp = (VkStencilOp)create_info->pDepthStencilState->back.passOp;
            stencil_op_state_back.depthFailOp = (VkStencilOp)create_info->pDepthStencilState->back.depthFailOp;
            stencil_op_state_back.compareOp = (VkCompareOp)create_info->pDepthStencilState->back.compareOp;
            stencil_op_state_back.compareMask = create_info->pDepthStencilState->back.compareMask;
            stencil_op_state_back.writeMask = create_info->pDepthStencilState->back.writeMask;
            stencil_op_state_back.reference = create_info->pDepthStencilState->back.reference;
            VkPipelineDepthStencilStateCreateInfo& vk_pipeline_depth_stencil_state_create_info = depthStencilStateList[k];
            vk_pipeline_depth_stencil_state_create_info.sType = (VkStructureType)create_info->pDepthStencilState->sType;
            vk_pipeline_depth_stencil_state_create_info.pNext = (const void*)create_info->pDepthStencilState->pNext;
            vk_pipeline_depth_stencil_state_create_info.flags = (VkPipelineDepthStencilStateCreateFlags)create_info->pDepthStencilState->flags;
            vk_pipeline_depth_stencil_state_create_info.depthTestEnable = (VkBool32)create_info->pDepthStencilState->depthTestEnable;
            vk_pipeline_depth_stencil_state_create_info.depthWriteEnable = (VkBool32)create_info->pDepthStencilState->depthWriteEnable;
            vk_pipeline_depth_stencil_state_create_info.depthCompareOp = (VkCompareOp)create_info->pDepthStencilState->depthCompareOp;
            vk_pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = (VkBool32)create_info->pDepthStencilState->depthBoundsTestEnable;
            vk_pipeline_depth_stencil_state_create_info.stencilTestEnable = (VkBool32)create_info->pDepthStencilState->stencilTestEnable;
            vk_pipeline_depth_stencil_state_create_info.front = stencil_op_state_front;
            vk_pipeline_depth_stencil_state_create_info.back = stencil_op_state_back;
            vk_pipeline_depth_stencil_state_create_info.minDepthBounds = create_info->pDepthStencilState->minDepthBounds;
            vk_pipeline_depth_stencil_state_create_info.maxDepthBounds = create_info->pDepthStencilState->maxDepthBounds;

            int pipeline_color_blend_attachment_state_size = create_info->pColorBlendState->attachmentCount;
            std::vector<VkPipelineColorBlendAttachmentState>& vk_pipeline_color_blend_attachment_state_list = colorBlendAttachmentStateListArrays[k];
            vk_pipeline_color_blend_attachment_state_list.resize(pipeline_color_blend_attachment_state_size);
            for (int i = 0; i < pipeline_color_blend_attachment_state_size; ++i) {
                const auto& rhi_pipeline_color_blend_attachment_state_element = create_info->pColorBlendState->pAttachments[i];
                auto& vk_pipeline_color_blend_attachment_state_element = vk_pipeline_color_blend_attachment_state_list[i];
                vk_pipeline_color_blend_attachment_state_element.blendEnable = (VkBool32)rhi_pipeline_color_blend_attachment_state_element.blendEnable;
                vk_pipeline_color_blend_attachment_state_element.srcColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcColorBlendFactor;
                vk_pipeline_color_blend_attachment_state_element.dstColorBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstColorBlendFactor;
                vk_pipeline_color_blend_attachment_state_element.colorBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.colorBlendOp;
                vk_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.srcAlphaBlendFactor;
                vk_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor = (VkBlendFactor)rhi_pipeline_color_blend_attachment_state_element.dstAlphaBlendFactor;
                vk_pipeline_color_blend_attachment_state_element.alphaBlendOp = (VkBlendOp)rhi_pipeline_color_blend_attachment_state_element.alphaBlendOp;
                vk_pipeline_color_blend_attachment_state_element.colorWriteMask = (VkColorComponentFlags)rhi_pipeline_color_blend_attachment_state_element.colorWriteMask;
            }
            VkPipelineColorBlendStateCreateInfo& vk_pipeline_color_blend_state_create_info = colorBlendStateList[k];
            vk_pipeline_color_blend_state_create_info.sType = (VkStructureType)create_info->pColorBlendState->sType;
            vk_pipeline_color_blend_state_create_info.pNext = (const void*)create_info->pColorBlendState->pNext;
            vk_pipeline_color_blend_state_create_info.flags = (VkPipelineColorBlendStateCreateFlags)create_info->pColorBlendState->flags;
            vk_pipeline_color_blend_state_create_info.logicOpEnable = (VkBool32)create_info->pColorBlendState->logicOpEnable;
            vk_pipeline_color_blend_state_create_info.logicOp = (VkLogicOp)create_info->pColorBlendState->logicOp;
            vk_pipeline_color_blend_state_create_info.attachmentCount = create_info->pColorBlendState->attachmentCount;
            vk_pipeline_color_blend_state_create_info.pAttachments = vk_pipeline_color_blend_attachment_state_list.data();
            for (int i = 0; i < 4; ++i) {
                vk_pipeline_color_blend_state_create_info.blendConstants[i] = create_info->pColorBlendState->blendConstants[i];
            }

            int dynamic_state_size = create_info->pDynamicState->dynamicStateCount;
            std::vector<VkDynamicState>& vk_dynamic_state_list = dynamincStateListArrays[k];
            vk_dynamic_state_list.resize(dynamic_state_size);
            for (int i = 0; i < dynamic_state_size; ++i) {
                const auto& rhi_dynamic_state_element = create_info->pDynamicState->pDynamicStates[i];
                auto& vk_dynamic_state_element = vk_dynamic_state_list[i];
                vk_dynamic_state_element = (VkDynamicState)rhi_dynamic_state_element;
            }
            VkPipelineDynamicStateCreateInfo& vk_pipeline_dynamic_state_create_info = dynamincStateList[k];
            vk_pipeline_dynamic_state_create_info.sType = (VkStructureType)create_info->pDynamicState->sType;
            vk_pipeline_dynamic_state_create_info.pNext = (const void*)create_info->pDynamicState->pNext;
            vk_pipeline_dynamic_state_create_info.flags = (VkPipelineDynamicStateCreateFlags)create_info->pDynamicState->flags;
            vk_pipeline_dynamic_state_create_info.dynamicStateCount = create_info->pDynamicState->dynamicStateCount;
            vk_pipeline_dynamic_state_create_info.pDynamicStates = vk_dynamic_state_list.data();

            auto& createInfo = createInfos[k];
            createInfo.sType = (VkStructureType)create_info->sType;
            createInfo.pNext = (const void*)create_info->pNext;
            createInfo.flags = (VkPipelineCreateFlags)create_info->flags;
            createInfo.stageCount = create_info->stageCount;
            createInfo.pStages = vk_pipeline_shader_stage_create_info_list.data();
            createInfo.pVertexInputState = &vk_pipeline_vertex_input_state_create_info;
            createInfo.pInputAssemblyState = &vk_pipeline_input_assembly_state_create_info;
            createInfo.pTessellationState = vk_pipeline_tessellation_state_create_info_ptr;
            createInfo.pViewportState = &vk_pipeline_viewport_state_create_info;
            createInfo.pRasterizationState = &vk_pipeline_rasterization_state_create_info;
            createInfo.pMultisampleState = &vk_pipeline_multisample_state_create_info;
            createInfo.pDepthStencilState = &vk_pipeline_depth_stencil_state_create_info;
            createInfo.pColorBlendState = &vk_pipeline_color_blend_state_create_info;
            createInfo.pDynamicState = &vk_pipeline_dynamic_state_create_info;
            createInfo.layout = ((VulkanPipelineLayout*)create_info->layout)->getResource();
            createInfo.renderPass = ((VulkanRenderPass*)create_info->renderPass)->getResource();
            createInfo.subpass = create_info->subpass;
            if (create_info->basePipelineHandle != nullptr) {
                createInfo.basePipelineHandle = ((VulkanPipeline*)create_info->basePipelineHandle)->getResource();
            }
            else {
                createInfo.basePipelineHandle = VK_NULL_HANDLE;
            }
            createInfo.basePipelineIndex = create_info->basePipelineIndex;
        }
        pipelines = new VulkanPipeline[create_info_count];
        std::vector<VkPipeline> vk_pipelines(create_info_count);
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipeline_cache != nullptr) {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipeline_cache)->getResource();
        }
        if (vkCreateGraphicsPipelines(m_device, vk_pipeline_cache, create_info_count, createInfos.data(), nullptr, vk_pipelines.data()) == VK_SUCCESS) {
            for (int i = 0; i < create_info_count; ++i) {
                ((VulkanPipeline*)&pipelines[i])->setResource(vk_pipelines[i]);
            }
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateGraphicsPipelines failed!");
            delete[] pipelines;
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createComputePipelines(
        RHIPipelineCache* pipeline_cache,
        uint32_t create_info_count,
        const RHIComputePipelineCreateInfo* create_infos,
        RHIPipeline*& pipelines
    ) {
        std::vector<VkComputePipelineCreateInfo> createInfos(create_info_count);
        std::vector<VkSpecializationInfo> specializationInfoList(create_info_count);
        std::vector<std::vector<VkSpecializationMapEntry>> specializationMapEntryListArrays(create_info_count);
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageList(create_info_count);
        for (int k = 0; k < create_info_count; ++k) {
            const auto& create_info = &create_infos[k];
            VkPipelineShaderStageCreateInfo& shader_stage_create_info = shaderStageList[k];
            if (create_info->pStage->pSpecializationInfo != nullptr) {
                VkSpecializationInfo& vk_specialization_info = specializationInfoList[k];
                std::vector<VkSpecializationMapEntry> vk_specialization_map_entry_list = specializationMapEntryListArrays[k];
                vk_specialization_map_entry_list.resize(create_info->pStage->pSpecializationInfo->mapEntryCount);
                vk_specialization_info.mapEntryCount = create_info->pStage->pSpecializationInfo->mapEntryCount;
                vk_specialization_info.pMapEntries = vk_specialization_map_entry_list.data();
                vk_specialization_info.dataSize = create_info->pStage->pSpecializationInfo->dataSize;
                vk_specialization_info.pData = (const void*)create_info->pStage->pSpecializationInfo->pData;
                for (int i = 0; i < create_info->pStage->pSpecializationInfo->mapEntryCount; ++i) {
                    const auto& rhi_specialization_map_entry_element = create_info->pStage->pSpecializationInfo->pMapEntries[i];
                    auto& vk_specialization_map_entry_element = vk_specialization_map_entry_list[i];

                    vk_specialization_map_entry_element.constantID = rhi_specialization_map_entry_element.constantID;
                    vk_specialization_map_entry_element.offset = rhi_specialization_map_entry_element.offset;
                    vk_specialization_map_entry_element.size = rhi_specialization_map_entry_element.size;
                }
                shader_stage_create_info.pSpecializationInfo = &vk_specialization_info;
            }
            else {
                shader_stage_create_info.pSpecializationInfo = nullptr;
            }
            shader_stage_create_info.sType = (VkStructureType)create_info->pStage->sType;
            shader_stage_create_info.pNext = (const void*)create_info->pStage->pNext;
            shader_stage_create_info.flags = (VkPipelineShaderStageCreateFlags)create_info->pStage->flags;
            shader_stage_create_info.stage = (VkShaderStageFlagBits)create_info->pStage->stage;
            shader_stage_create_info.module = ((VulkanShader*)create_info->pStage->module)->getResource();
            shader_stage_create_info.pName = create_info->pStage->pName;

            auto& createInfo = createInfos[k];
            createInfo.sType = (VkStructureType)create_info->sType;
            createInfo.pNext = (const void*)create_info->pNext;
            createInfo.flags = (VkPipelineCreateFlags)create_info->flags;
            createInfo.stage = shader_stage_create_info;
            createInfo.layout = ((VulkanPipelineLayout*)create_info->layout)->getResource();
            if (create_infos->basePipelineHandle != nullptr) {
                createInfo.basePipelineHandle = ((VulkanPipeline*)create_info->basePipelineHandle)->getResource();
            }
            else {
                createInfo.basePipelineHandle = VK_NULL_HANDLE;
            }
            createInfo.basePipelineIndex = create_info->basePipelineIndex;
        }

        pipelines = new VulkanPipeline[create_info_count];
        std::vector<VkPipeline> vk_pipelines(create_info_count);
        VkPipelineCache vk_pipeline_cache = VK_NULL_HANDLE;
        if (pipeline_cache != nullptr) {
            vk_pipeline_cache = ((VulkanPipelineCache*)pipeline_cache)->getResource();
        }
        if (vkCreateComputePipelines(m_device, vk_pipeline_cache, create_info_count, createInfos.data(), nullptr, vk_pipelines.data()) == VK_SUCCESS) {
            for (int i = 0; i < create_info_count; ++i) {
                ((VulkanPipeline*)&pipelines[i])->setResource(vk_pipelines[i]);
            }
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateComputePipelines failed!");
            delete[] pipelines;
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createPipelineLayout(const RHIPipelineLayoutCreateInfo* create_info, RHIPipelineLayout*& pipeline_layout) {
        int descriptor_set_layout_size = create_info->setLayoutCount;
        std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_layout_size);
        for (int i = 0; i < descriptor_set_layout_size; ++i) {
            const auto& rhi_descriptor_set_layout_element = create_info->pSetLayouts[i];
            auto& vk_descriptor_set_layout_element = vk_descriptor_set_layout_list[i];
            vk_descriptor_set_layout_element = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->getResource();
        }
        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkPipelineLayoutCreateFlags)create_info->flags;
        createInfo.setLayoutCount = create_info->setLayoutCount;
        createInfo.pSetLayouts = vk_descriptor_set_layout_list.data();

        pipeline_layout = new VulkanPipelineLayout();
        VkPipelineLayout vk_pipeline_layout;
        if (vkCreatePipelineLayout(m_device, &createInfo, nullptr, &vk_pipeline_layout) == VK_SUCCESS) {
            ((VulkanPipelineLayout*)pipeline_layout)->setResource(vk_pipeline_layout);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreatePipelineLayout failed!");
            delete(pipeline_layout);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createRenderPass(const RHIRenderPassCreateInfo* create_info, RHIRenderPass*& render_pass) {
        std::vector<VkAttachmentDescription> vk_attachments(create_info->attachmentCount);
        for (int i = 0; i < create_info->attachmentCount; ++i) {
            const auto& rhi_desc = create_info->pAttachments[i];
            auto& vk_desc = vk_attachments[i];
            vk_desc.flags = (VkAttachmentDescriptionFlags)rhi_desc.flags;
            vk_desc.format = (VkFormat)rhi_desc.format;
            vk_desc.samples = (VkSampleCountFlagBits)rhi_desc.samples;
            vk_desc.loadOp = (VkAttachmentLoadOp)rhi_desc.loadOp;
            vk_desc.storeOp = (VkAttachmentStoreOp)rhi_desc.storeOp;
            vk_desc.stencilLoadOp = (VkAttachmentLoadOp)rhi_desc.stencilLoadOp;
            vk_desc.stencilStoreOp = (VkAttachmentStoreOp)rhi_desc.stencilStoreOp;
            vk_desc.initialLayout = (VkImageLayout)rhi_desc.initialLayout;
            vk_desc.finalLayout = (VkImageLayout)rhi_desc.finalLayout;
        }

        int totalAttachmentReference = 0;
        for (int i = 0; i < create_info->subpassCount; ++i) {
            const auto& rhi_desc = create_info->pSubpasses[i];
            totalAttachmentReference += rhi_desc.inputAttachmentCount;
            totalAttachmentReference += rhi_desc.colorAttachmentCount;
            if (rhi_desc.pResolveAttachments != nullptr) {
                totalAttachmentReference += rhi_desc.colorAttachmentCount;
            }
            if (rhi_desc.pDepthStencilAttachment != nullptr) {
                totalAttachmentReference += 1;
            }
        }
        std::vector<VkSubpassDescription> vk_subpass_description(create_info->subpassCount);
        std::vector<VkAttachmentReference> vk_attachment_reference(totalAttachmentReference);
        int currentAttachmentReference = 0;
        for (int i = 0; i < create_info->subpassCount; ++i) {
            const auto& rhi_desc = create_info->pSubpasses[i];
            auto& vk_desc = vk_subpass_description[i];
            vk_desc.flags = (RHISubpassDescriptionFlags)rhi_desc.flags;
            vk_desc.pipelineBindPoint = (VkPipelineBindPoint)rhi_desc.pipelineBindPoint;
            vk_desc.preserveAttachmentCount = rhi_desc.preserveAttachmentCount;
            vk_desc.pPreserveAttachments = (const uint32_t*)rhi_desc.pPreserveAttachments;
            vk_desc.inputAttachmentCount = rhi_desc.inputAttachmentCount;
            vk_desc.pInputAttachments = &vk_attachment_reference[currentAttachmentReference];
            for (int j = 0; j < rhi_desc.inputAttachmentCount; ++j) {
                const auto& rhi_attachment_referene_input = rhi_desc.pInputAttachments[j];
                auto& vk_attachment_reference_input = vk_attachment_reference[currentAttachmentReference];
                vk_attachment_reference_input.attachment = rhi_attachment_referene_input.attachment;
                vk_attachment_reference_input.layout = (VkImageLayout)rhi_attachment_referene_input.layout;
                currentAttachmentReference += 1;
            }
            vk_desc.colorAttachmentCount = rhi_desc.colorAttachmentCount;
            vk_desc.pColorAttachments = &vk_attachment_reference[currentAttachmentReference];
            for (int j = 0; j < rhi_desc.colorAttachmentCount; ++j) {
                const auto& rhi_attachment_reference_color = rhi_desc.pColorAttachments[j];
                auto& vk_attachment_reference_color = vk_attachment_reference[currentAttachmentReference];
                vk_attachment_reference_color.attachment = rhi_attachment_reference_color.attachment;
                vk_attachment_reference_color.layout = (VkImageLayout)rhi_attachment_reference_color.layout;
                currentAttachmentReference += 1;
            }
            if (rhi_desc.pResolveAttachments != nullptr) {
                vk_desc.pResolveAttachments = &vk_attachment_reference[currentAttachmentReference];
                for (int j = 0; j < rhi_desc.colorAttachmentCount; ++j) {
                    const auto& rhi_attachment_refernce_resolve = rhi_desc.pResolveAttachments[j];
                    auto& vk_attachment_refernce_resolve = vk_attachment_reference[currentAttachmentReference];
                    vk_attachment_refernce_resolve.attachment = rhi_attachment_refernce_resolve.attachment;
                    vk_attachment_refernce_resolve.layout = (VkImageLayout)rhi_attachment_refernce_resolve.layout;
                    currentAttachmentReference += 1;
                }
            }
            if (rhi_desc.pDepthStencilAttachment != nullptr) {
                vk_desc.pDepthStencilAttachment = &vk_attachment_reference[currentAttachmentReference];
                const auto& rhi_attachment_reference_depth = rhi_desc.pDepthStencilAttachment;
                auto& vk_attachment_reference_depth = vk_attachment_reference[currentAttachmentReference];
                vk_attachment_reference_depth.attachment = rhi_attachment_reference_depth->attachment;
                vk_attachment_reference_depth.layout = (VkImageLayout)rhi_attachment_reference_depth->layout;
                currentAttachmentReference += 1;
            }
        }
        if (currentAttachmentReference != totalAttachmentReference) {
            LOG_ERROR("currentAttachmentRefence != totalAttachmentRefenrence");
            return RHI_FALSE;
        }
        
        std::vector<VkSubpassDependency> vk_subpass_dependency(create_info->dependencyCount);
        for (int i = 0; i < create_info->dependencyCount; ++i) {
            const auto& rhi_desc = create_info->pDependencies[i];
            auto& vk_desc = vk_subpass_dependency[i];
            vk_desc.srcSubpass = rhi_desc.srcSubpass;
            vk_desc.dstSubpass = rhi_desc.dstSubpass;
            vk_desc.srcStageMask = (VkPipelineStageFlags)rhi_desc.srcStageMask;
            vk_desc.dstStageMask = (VkPipelineStageFlags)rhi_desc.dstStageMask;
            vk_desc.srcAccessMask = (VkAccessFlags)rhi_desc.srcAccessMask;
            vk_desc.dstAccessMask = (VkAccessFlags)rhi_desc.dstAccessMask;
            vk_desc.dependencyFlags = (VkDependencyFlags)rhi_desc.dependencyFlags;
        }

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkRenderPassCreateFlags)create_info->flags;
        createInfo.attachmentCount = create_info->attachmentCount;
        createInfo.pAttachments = vk_attachments.data();
        createInfo.subpassCount = create_info->subpassCount;
        createInfo.pSubpasses = vk_subpass_description.data();
        createInfo.dependencyCount = create_info->dependencyCount;
        createInfo.pDependencies = vk_subpass_dependency.data();

        render_pass = new VulkanRenderPass();
        VkRenderPass vk_render_pass;
        if (vkCreateRenderPass(m_device, &createInfo, nullptr, &vk_render_pass) == VK_SUCCESS) {
            ((VulkanRenderPass*)render_pass)->setResource(vk_render_pass);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateRenderPass failed!");
            delete(render_pass);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createSampler(const RHISamplerCreateInfo* create_info, RHISampler*& sampler) {
        VkSamplerCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkSamplerCreateFlags)create_info->flags;
        createInfo.magFilter = (VkFilter)create_info->magFilter;
        createInfo.minFilter = (VkFilter)create_info->minFilter;
        createInfo.mipmapMode = (VkSamplerMipmapMode)create_info->mipmapMode;
        createInfo.addressModeU = (VkSamplerAddressMode)create_info->addressModeU;
        createInfo.addressModeV = (VkSamplerAddressMode)create_info->addressModeV;
        createInfo.addressModeW = (VkSamplerAddressMode)create_info->addressModeW;
        createInfo.mipLodBias = create_info->mipLodBias;
        createInfo.anisotropyEnable = (VkBool32)create_info->anisotropyEnable;
        createInfo.maxAnisotropy = create_info->maxAnisotropy;
        createInfo.compareEnable = (VkBool32)create_info->compareEnable;
        createInfo.compareOp = (VkCompareOp)create_info->compareOp;
        createInfo.minLod = create_info->minLod;
        createInfo.maxLod = create_info->maxLod;
        createInfo.borderColor = (VkBorderColor)create_info->borderColor;
        createInfo.unnormalizedCoordinates = (VkBool32)create_info->unnormalizedCoordinates;

        sampler = new VulkanSampler();
        VkSampler vk_sampler;
        if (vkCreateSampler(m_device, &createInfo, nullptr, &vk_sampler) == VK_SUCCESS) {
            ((VulkanSampler*)sampler)->setResource(vk_sampler);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateSampler failed!");
            delete(sampler);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createSemaphore(const RHISemaphoreCreateInfo* create_info, RHISemaphore*& semaphore) {
        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = (VkStructureType)create_info->sType;
        createInfo.pNext = (const void*)create_info->pNext;
        createInfo.flags = (VkSemaphoreCreateFlags)create_info->flags;

        semaphore = new VulkanSemaphore();
        VkSemaphore vk_semaphore;
        if (vkCreateSemaphore(m_device, &createInfo, nullptr, &vk_semaphore) == VK_SUCCESS) {
            ((VulkanSemaphore*)semaphore)->setResource(vk_semaphore);
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkCreateSemaphore failed!");
            delete(semaphore);
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::waitForFencesPFN(
        uint32_t fence_count, 
        RHIFence* const* fences, 
        RHIBool32 wait_all, 
        uint64_t timeout
    ) {
        std::vector<VkFence> vk_fence_list(fence_count);
        for (int i = 0; i < fence_count; ++i) {
            const auto& rhi_fence_element = fences[i];
            auto& vk_fence_element = vk_fence_list[i];
            vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
        }

        if (f_vkWaitForFences(m_device, fence_count, vk_fence_list.data(), wait_all, timeout) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("f_vkWaitForFences failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::resetFencesPFN(uint32_t fence_count, RHIFence* const* fences) {
        std::vector<VkFence> vk_fence_list(fence_count);
        for (int i = 0; i < fence_count; ++i) {
            const auto& rhi_fence_element = fences[i];
            auto& vk_fence_element = vk_fence_list[i];
            vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
        }

        if (f_vkResetFences(m_device, fence_count, vk_fence_list.data()) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("f_vkResetFences failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::resetCommandPoolPFN(RHICommandPool* command_pool, RHICommandPoolResetFlags flags) {
        if (f_vkResetCommandPool(m_device,
            ((VulkanCommandPool*)command_pool)->getResource(),
            (VkCommandPoolResetFlags)flags) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("f_vkResetCommandPool failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::beginCommandBufferPFN(RHICommandBuffer* command_buffer, const RHICommandBufferBeginInfo* begin_info) {
        VkCommandBufferInheritanceInfo* command_buffer_inheritance_info_ptr = nullptr;
        VkCommandBufferInheritanceInfo command_buffer_inheritance_info{};
        if (begin_info->pInheritanceInfo != nullptr) {
            command_buffer_inheritance_info.sType = (VkStructureType)begin_info->pInheritanceInfo->sType;
            command_buffer_inheritance_info.pNext = (const void*)begin_info->pInheritanceInfo->pNext;
            command_buffer_inheritance_info.renderPass = ((VulkanRenderPass*)begin_info->pInheritanceInfo->renderPass)->getResource();
            command_buffer_inheritance_info.subpass = begin_info->pInheritanceInfo->subpass;
            command_buffer_inheritance_info.framebuffer = ((VulkanFramebuffer*)begin_info->pInheritanceInfo->framebuffer)->getResource();
            command_buffer_inheritance_info.occlusionQueryEnable = (VkBool32)begin_info->pInheritanceInfo->occlusionQueryEnable;
            command_buffer_inheritance_info.queryFlags = (VkQueryControlFlags)begin_info->pInheritanceInfo->queryFlags;
            command_buffer_inheritance_info.pipelineStatistics = (VkQueryPipelineStatisticFlags)begin_info->pInheritanceInfo->pipelineStatistics;
            command_buffer_inheritance_info_ptr = &command_buffer_inheritance_info;
        }

        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = (VkStructureType)begin_info->sType;
        command_buffer_begin_info.pNext = (const void*)begin_info->pNext;
        command_buffer_begin_info.flags = (VkCommandBufferUsageFlags)begin_info->flags;
        command_buffer_begin_info.pInheritanceInfo = command_buffer_inheritance_info_ptr;
        if (f_vkBeginCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(), &command_buffer_begin_info) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("f_vkBeginCommandBuffer failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::endCommandBufferPFN(RHICommandBuffer* command_buffer) {
        if (f_vkEndCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource()) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("f_vkEndCommandBuffer failed");
            return RHI_FALSE;
        }
    }

    void VulkanRHI::cmdBeginRenderPassPFN(
        RHICommandBuffer* command_buffer,
        const RHIRenderPassBeginInfo* begin_info,
        RHISubpassContents contents
    ) {
        VkOffset2D offset_2d{};
        offset_2d.x = begin_info->renderArea.offset.x;
        offset_2d.y = begin_info->renderArea.offset.y;
        VkExtent2D extent_2d{};
        extent_2d.width = begin_info->renderArea.extent.width;
        extent_2d.height = begin_info->renderArea.extent.height;
        VkRect2D rect_2d{};
        rect_2d.offset = offset_2d;
        rect_2d.extent = extent_2d;

        int clear_value_size = begin_info->clearValueCount;
        std::vector<VkClearValue> vk_clear_value_list(clear_value_size);
        for (int i = 0; i < clear_value_size; ++i) {
            const auto& rhi_clear_value_element = begin_info->pClearValues[i];
            auto& vk_clear_value_element = vk_clear_value_list[i];

            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_value_element.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_value_element.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_value_element.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_value_element.color.float32[3];
            vk_clear_color_value.int32[0] = rhi_clear_value_element.color.int32[0];
            vk_clear_color_value.int32[1] = rhi_clear_value_element.color.int32[1];
            vk_clear_color_value.int32[2] = rhi_clear_value_element.color.int32[2];
            vk_clear_color_value.int32[3] = rhi_clear_value_element.color.int32[3];
            vk_clear_color_value.uint32[0] = rhi_clear_value_element.color.uint32[0];
            vk_clear_color_value.uint32[1] = rhi_clear_value_element.color.uint32[1];
            vk_clear_color_value.uint32[2] = rhi_clear_value_element.color.uint32[2];
            vk_clear_color_value.uint32[3] = rhi_clear_value_element.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth = rhi_clear_value_element.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_value_element.depthStencil.stencil;

            vk_clear_value_element.color = vk_clear_color_value;
            vk_clear_value_element.depthStencil = vk_clear_depth_stencil_value;
        }

        VkRenderPassBeginInfo vk_render_pass_begin_info{};
        vk_render_pass_begin_info.sType = (VkStructureType)begin_info->sType;
        vk_render_pass_begin_info.pNext = (const void*)begin_info->pNext;
        vk_render_pass_begin_info.renderPass = ((VulkanRenderPass*)begin_info->renderPass)->getResource();
        vk_render_pass_begin_info.framebuffer = ((VulkanFramebuffer*)begin_info->framebuffer)->getResource();
        vk_render_pass_begin_info.renderArea = rect_2d;
        vk_render_pass_begin_info.clearValueCount = begin_info->clearValueCount;
        vk_render_pass_begin_info.pClearValues = vk_clear_value_list.data();
        
        f_vkCmdBeginRenderPass(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            &vk_render_pass_begin_info,
            (VkSubpassContents)contents
        );
    }

    void VulkanRHI::cmdNextSubpassPFN(RHICommandBuffer* command_buffer, RHISubpassContents contents) {
        f_vkCmdNextSubpass(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            ((VkSubpassContents)contents)
        );
    }

    void VulkanRHI::cmdEndRenderPassPFN(RHICommandBuffer* command_buffer) {
        f_vkCmdEndRenderPass(((VulkanCommandBuffer*)command_buffer)->getResource());
    }

    void VulkanRHI::cmdBindPipelinePFN(
        RHICommandBuffer* command_buffer,
        RHIPipelineBindPoint pipeline_bind_point,
        RHIPipeline* pipeline
    ) {
        f_vkCmdBindPipeline(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            (VkPipelineBindPoint)pipeline_bind_point,
            ((VulkanPipeline*)pipeline)->getResource()
        );
    }

    void VulkanRHI::cmdSetViewportPFN(
        RHICommandBuffer* command_buffer,
        uint32_t first_viewport,
        uint32_t viewport_count,
        const RHIViewport* viewports
    ) {
        std::vector<VkViewport> vk_viewport_list(viewport_count);
        for (int i = 0; i < viewport_count; ++i) {
            const auto& rhi_viewport_element = viewports[i];
            auto& vk_viewport_element = vk_viewport_list[i];
            vk_viewport_element.x = rhi_viewport_element.x;
            vk_viewport_element.y = rhi_viewport_element.y;
            vk_viewport_element.width = rhi_viewport_element.width;
            vk_viewport_element.height = rhi_viewport_element.height;
            vk_viewport_element.minDepth = rhi_viewport_element.minDepth;
            vk_viewport_element.maxDepth = rhi_viewport_element.maxDepth;
        }

        f_vkCmdSetViewport(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            first_viewport, viewport_count,
            vk_viewport_list.data()
        );
    }

    void VulkanRHI::cmdSetScissorPFN(
        RHICommandBuffer* command_buffer, 
        uint32_t first_scissor, 
        uint32_t scissor_count, 
        const RHIRect2D* scissors
    ) {
        std::vector<VkRect2D> vk_rect_2d_list(scissor_count);
        for (int i = 0; i < scissor_count; ++i) {
            const auto& rhi_rect_2d_element = scissors[i];
            auto& vk_rect_2d_element = vk_rect_2d_list[i];

            VkOffset2D offset_2d{};
            offset_2d.x = rhi_rect_2d_element.offset.x;
            offset_2d.y = rhi_rect_2d_element.offset.y;
            VkExtent2D extent_2d{};
            extent_2d.width = rhi_rect_2d_element.extent.width;
            extent_2d.height = rhi_rect_2d_element.extent.height;

            vk_rect_2d_element.offset = offset_2d;
            vk_rect_2d_element.extent = extent_2d;
        }

        f_vkCmdSetScissor(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            first_scissor, scissor_count,
            vk_rect_2d_list.data()
        );
    }

    void VulkanRHI::cmdBindVertexBuffersPFN(
        RHICommandBuffer* command_buffer,
        uint32_t first_binding,
        uint32_t binding_count,
        RHIBuffer* const* buffers,
        const RHIDeviceSize* offsets
    ) {
        std::vector<VkBuffer> vk_buffer_list(binding_count);
        for (int i = 0; i < binding_count; ++i) {
            const auto& rhi_buffer_element = buffers[i];
            auto& vk_buffer_element = vk_buffer_list[i];
            vk_buffer_element = ((VulkanBuffer*)rhi_buffer_element)->getResource();
        }

        std::vector<VkDeviceSize> vk_device_size_list(binding_count);
        for (int i = 0; i < binding_count; ++i) {
            const auto& rhi_offset_element = offsets[i];
            auto& vk_offset_element = vk_device_size_list[i];
            vk_offset_element = rhi_offset_element;
        }

        f_vkCmdBindVertexBuffers(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            first_binding, binding_count, vk_buffer_list.data(), vk_device_size_list.data()
        );
    }

    void VulkanRHI::cmdBindIndexBufferPFN(
        RHICommandBuffer* command_buffer,
        RHIBuffer* buffer,
        RHIDeviceSize offset,
        RHIIndexType index_type
    ) {
        f_vkCmdBindIndexBuffer(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            ((VulkanBuffer*)buffer)->getResource(),
            (VkDeviceSize)offset,
            (VkIndexType)index_type
        );
    }

    void VulkanRHI::cmdBindDescriptorSetsPFN(
        RHICommandBuffer* command_buffer,
        RHIPipelineBindPoint pipeline_bind_point,
        RHIPipelineLayout* layout,
        uint32_t first_set,
        uint32_t descriptor_set_count,
        const RHIDescriptorSet* const* descriptor_sets,
        uint32_t dynamic_offset_count,
        const uint32_t* dynamic_offsets
    ) {
        std::vector<VkDescriptorSet> vk_descriptor_set_list(descriptor_set_count);
        for (int i = 0; i < descriptor_set_count; ++i) {
            const auto& rhi_descriptor_set_element = descriptor_sets[i];
            auto& vk_descriptor_set_element = vk_descriptor_set_list[i];
            vk_descriptor_set_element = ((VulkanDescriptorSet*)rhi_descriptor_set_element)->getResource();
        }

        std::vector<uint32_t> vk_offset_list(dynamic_offset_count);
        for (int i = 0; i < dynamic_offset_count; ++i) {
            const auto& rhi_offset_element = dynamic_offsets[i];
            auto& vk_offset_element = vk_offset_list[i];
            vk_offset_element = rhi_offset_element;
        }

        f_vkCmdBindDescriptorSets(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            (VkPipelineBindPoint)pipeline_bind_point,
            ((VulkanPipelineLayout*)layout)->getResource(),
            first_set, descriptor_set_count,
            vk_descriptor_set_list.data(),
            dynamic_offset_count,
            vk_offset_list.data()
        );
    }

    void VulkanRHI::cmdDrawIndexedPFN(
        RHICommandBuffer* command_buffer,
        uint32_t index_count,
        uint32_t instance_count,
        uint32_t first_index,
        int32_t vertex_offset,
        uint32_t first_instance
    ) {
        f_vkCmdDrawIndexed(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            index_count, instance_count, first_index, vertex_offset, first_instance
        );
    }

    void VulkanRHI::cmdClearAttachmentsPFN(
        RHICommandBuffer* command_buffer,
        uint32_t attachment_count,
        const RHIClearAttachment* attachments,
        uint32_t rect_count,
        const RHIClearRect* rects
    ) {
        std::vector<VkClearAttachment> vk_clear_attachment_list(attachment_count);
        for (int i = 0; i < attachment_count; ++i) {
            const auto& rhi_clear_attachment_element = attachments[i];
            auto& vk_clear_attachment_element = vk_clear_attachment_list[i];
            VkClearColorValue vk_clear_color_value;
            vk_clear_color_value.float32[0] = rhi_clear_attachment_element.clearValue.color.float32[0];
            vk_clear_color_value.float32[1] = rhi_clear_attachment_element.clearValue.color.float32[1];
            vk_clear_color_value.float32[2] = rhi_clear_attachment_element.clearValue.color.float32[2];
            vk_clear_color_value.float32[3] = rhi_clear_attachment_element.clearValue.color.float32[3];
            vk_clear_color_value.int32[0] = rhi_clear_attachment_element.clearValue.color.int32[0];
            vk_clear_color_value.int32[1] = rhi_clear_attachment_element.clearValue.color.int32[1];
            vk_clear_color_value.int32[2] = rhi_clear_attachment_element.clearValue.color.int32[2];
            vk_clear_color_value.int32[3] = rhi_clear_attachment_element.clearValue.color.int32[3];
            vk_clear_color_value.uint32[0] = rhi_clear_attachment_element.clearValue.color.uint32[0];
            vk_clear_color_value.uint32[1] = rhi_clear_attachment_element.clearValue.color.uint32[1];
            vk_clear_color_value.uint32[2] = rhi_clear_attachment_element.clearValue.color.uint32[2];
            vk_clear_color_value.uint32[3] = rhi_clear_attachment_element.clearValue.color.uint32[3];

            VkClearDepthStencilValue vk_clear_depth_stencil_value;
            vk_clear_depth_stencil_value.depth = rhi_clear_attachment_element.clearValue.depthStencil.depth;
            vk_clear_depth_stencil_value.stencil = rhi_clear_attachment_element.clearValue.depthStencil.stencil;
            vk_clear_attachment_element.clearValue.color = vk_clear_color_value;
            vk_clear_attachment_element.clearValue.depthStencil = vk_clear_depth_stencil_value;
            vk_clear_attachment_element.aspectMask = rhi_clear_attachment_element.aspectMask;
            vk_clear_attachment_element.colorAttachment = rhi_clear_attachment_element.colorAttachment;
        }

        std::vector<VkClearRect> vk_clear_rect_list(rect_count);
        for (int i = 0; i < rect_count; ++i) {
            const auto& rhi_clear_rect_element = rects[i];
            auto& vk_clear_rect_element = vk_clear_rect_list[i];

            VkOffset2D offset_2d{};
            offset_2d.x = rhi_clear_rect_element.rect.offset.x;
            offset_2d.y = rhi_clear_rect_element.rect.offset.y;

            VkExtent2D extent_2d{};
            extent_2d.width = rhi_clear_rect_element.rect.extent.width;
            extent_2d.height = rhi_clear_rect_element.rect.extent.height;

            vk_clear_rect_element.rect.offset = offset_2d;
            vk_clear_rect_element.rect.extent = extent_2d;
            vk_clear_rect_element.baseArrayLayer = rhi_clear_rect_element.baseArrayLayer;
            vk_clear_rect_element.layerCount = rhi_clear_rect_element.layerCount;
        }

        f_vkCmdClearAttachments(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            attachment_count,
            vk_clear_attachment_list.data(),
            rect_count,
            vk_clear_rect_list.data()
        );
    }

    bool VulkanRHI::beginCommandBuffer(RHICommandBuffer* command_buffer, const RHICommandBufferBeginInfo* begin_info) {
        VkCommandBufferInheritanceInfo command_buffer_inheritance_info{};
        const VkCommandBufferInheritanceInfo* command_buffer_inheritance_info_ptr = nullptr;
        if (begin_info->pInheritanceInfo != nullptr) {
            command_buffer_inheritance_info.sType = (VkStructureType)begin_info->pInheritanceInfo->sType;
            command_buffer_inheritance_info.pNext = (const void*)begin_info->pInheritanceInfo->pNext;
            command_buffer_inheritance_info.renderPass = ((VulkanRenderPass*)begin_info->pInheritanceInfo->renderPass)->getResource();
            command_buffer_inheritance_info.subpass = begin_info->pInheritanceInfo->subpass;
            command_buffer_inheritance_info.framebuffer = ((VulkanFramebuffer*)begin_info->pInheritanceInfo->framebuffer)->getResource();
            command_buffer_inheritance_info.occlusionQueryEnable = (VkBool32)begin_info->pInheritanceInfo->occlusionQueryEnable;
            command_buffer_inheritance_info.queryFlags = (VkQueryControlFlags)begin_info->pInheritanceInfo->queryFlags;
            command_buffer_inheritance_info.pipelineStatistics = (VkQueryPipelineStatisticFlagBits)begin_info->pInheritanceInfo->pipelineStatistics;
            command_buffer_inheritance_info_ptr = &command_buffer_inheritance_info;
        }

        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = (VkStructureType)begin_info->sType;
        command_buffer_begin_info.pNext = (const void*)begin_info->pNext;
        command_buffer_begin_info.flags = (VkCommandBufferUsageFlags)begin_info->flags;
        command_buffer_begin_info.pInheritanceInfo = command_buffer_inheritance_info_ptr;

        if (vkBeginCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource(), &command_buffer_begin_info) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkBeginCommandBuffer failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::endCommandBuffer(RHICommandBuffer* command_buffer) {
        if (vkEndCommandBuffer(((VulkanCommandBuffer*)command_buffer)->getResource()) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkEndCommandBuffer failed!");
            return RHI_FALSE;
        }
    }

    void VulkanRHI::updateDescriptorSets(
        uint32_t descriptor_write_count,
        const RHIWriteDescriptorSet* descriptor_writes,
        uint32_t descriptor_copy_count,
        const RHICopyDescriptorSet* descriptor_copies
    ) {
        std::vector<VkWriteDescriptorSet> vk_write_descriptor_set_list(descriptor_write_count);
        int image_info_count = 0;
        int buffer_info_count = 0;
        for (int i = 0; i < descriptor_write_count; ++i) {
            const auto& rhi_write_descriptor_set_element = descriptor_writes[i];
            if (rhi_write_descriptor_set_element.pImageInfo != nullptr) {
                image_info_count++;
            }
            if (rhi_write_descriptor_set_element.pBufferInfo != nullptr) {
                buffer_info_count++;
            }
        }

        std::vector<VkDescriptorImageInfo> vk_descriptor_image_info_list(image_info_count);
        std::vector<VkDescriptorBufferInfo> vk_descriptor_buffer_info_list(buffer_info_count);
        int image_info_current = 0;
        int buffer_info_current = 0;
        for (int i = 0; i < descriptor_write_count; ++i) {
            const auto& rhi_write_descriptor_set_element = descriptor_writes[i];
            auto& vk_write_descriptor_set_element = vk_write_descriptor_set_list[i];

            const VkDescriptorImageInfo* vk_descriptor_image_info_ptr = nullptr;
            if (rhi_write_descriptor_set_element.pImageInfo != nullptr) {
                auto& vk_descriptor_image_info = vk_descriptor_image_info_list[image_info_current];
                if (rhi_write_descriptor_set_element.pImageInfo->sampler == nullptr) {
                    vk_descriptor_image_info.sampler = nullptr;
                }
                else {
                    vk_descriptor_image_info.sampler = ((VulkanSampler*)rhi_write_descriptor_set_element.pImageInfo->sampler)->getResource();
                }
                vk_descriptor_image_info.imageView = ((VulkanImageView*)rhi_write_descriptor_set_element.pImageInfo->imageView)->getResource();
                vk_descriptor_image_info.imageLayout = (VkImageLayout)rhi_write_descriptor_set_element.pImageInfo->imageLayout;
                vk_descriptor_image_info_ptr = &vk_descriptor_image_info;
                image_info_current++;
            }
            
            const VkDescriptorBufferInfo* vk_descriptor_buffer_info_ptr = nullptr;
            if (rhi_write_descriptor_set_element.pBufferInfo != nullptr) {
                auto& vk_descriptor_buffer_info = vk_descriptor_buffer_info_list[buffer_info_current];
                vk_descriptor_buffer_info.buffer = ((VulkanBuffer*)rhi_write_descriptor_set_element.pBufferInfo->buffer)->getResource();
                vk_descriptor_buffer_info.offset = (VkDeviceSize)rhi_write_descriptor_set_element.pBufferInfo->offset;
                vk_descriptor_buffer_info.range = (VkDeviceSize)rhi_write_descriptor_set_element.pBufferInfo->range;
                vk_descriptor_buffer_info_ptr = &vk_descriptor_buffer_info;
                buffer_info_current++;
            }

            vk_write_descriptor_set_element.sType = (VkStructureType)rhi_write_descriptor_set_element.sType;
            vk_write_descriptor_set_element.pNext = (const void*)rhi_write_descriptor_set_element.pNext;
            vk_write_descriptor_set_element.dstSet = ((VulkanDescriptorSet*)rhi_write_descriptor_set_element.dstSet)->getResource();
            vk_write_descriptor_set_element.dstBinding = rhi_write_descriptor_set_element.dstBinding;
            vk_write_descriptor_set_element.dstArrayElement = rhi_write_descriptor_set_element.dstArrayElement;
            vk_write_descriptor_set_element.descriptorCount = rhi_write_descriptor_set_element.descriptorCount;
            vk_write_descriptor_set_element.descriptorType = (VkDescriptorType)rhi_write_descriptor_set_element.descriptorType;
            vk_write_descriptor_set_element.pImageInfo = vk_descriptor_image_info_ptr;
            vk_write_descriptor_set_element.pBufferInfo = vk_descriptor_buffer_info_ptr;
            //TODO("getResource need return const reference")
            //vk_write_descriptor_set_element.pTexelBufferView = &((VulkanBufferView*)rhi_write_descriptor_set_element.pTexelBufferView)->getResource();
        }

        if (image_info_current != image_info_count || buffer_info_current != buffer_info_count) {
            LOG_ERROR("image_info_current != image_info_count || buffer_info_current != buffer_info_count");
        }

        std::vector<VkCopyDescriptorSet> vk_copy_descriptor_set_list(descriptor_copy_count);
        for (int i = 0; i < descriptor_copy_count; ++i) {
            const auto& rhi_copy_descriptor_set_element = descriptor_copies[i];
            auto& vk_copy_descriptor_set_element = vk_copy_descriptor_set_list[i];

            vk_copy_descriptor_set_element.sType = (VkStructureType)rhi_copy_descriptor_set_element.sType;
            vk_copy_descriptor_set_element.pNext = (const void*)rhi_copy_descriptor_set_element.pNext;
            vk_copy_descriptor_set_element.srcSet = ((VulkanDescriptorSet*)rhi_copy_descriptor_set_element.srcSet)->getResource();
            vk_copy_descriptor_set_element.srcBinding = rhi_copy_descriptor_set_element.srcBinding;
            vk_copy_descriptor_set_element.srcArrayElement = rhi_copy_descriptor_set_element.srcArrayElement;
            vk_copy_descriptor_set_element.dstSet = ((VulkanDescriptorSet*)rhi_copy_descriptor_set_element.dstSet)->getResource();
            vk_copy_descriptor_set_element.dstBinding = rhi_copy_descriptor_set_element.dstBinding;
            vk_copy_descriptor_set_element.dstArrayElement = rhi_copy_descriptor_set_element.dstArrayElement;
            vk_copy_descriptor_set_element.descriptorCount = rhi_copy_descriptor_set_element.descriptorCount;
        }

        vkUpdateDescriptorSets(
            m_device, descriptor_write_count,
            vk_write_descriptor_set_list.data(),
            descriptor_copy_count,
            vk_copy_descriptor_set_list.data()
        );
    }

    bool VulkanRHI::queueSubmit(
        RHIQueue* queue,
        uint32_t submit_count,
        const RHISubmitInfo* submits,
        RHIFence* fence
    ) {
        int command_buffer_size_total = 0;
        int semaphore_size_total = 0;
        int signal_semaphore_size_total = 0;
        int pipeline_stage_flags_size_total = 0;

        for (int i = 0; i < submit_count; ++i) {
            const auto& rhi_submit_info_element = submits[i];
            command_buffer_size_total += rhi_submit_info_element.commandBufferCount;
            semaphore_size_total += rhi_submit_info_element.waitSemaphoreCount;
            signal_semaphore_size_total += rhi_submit_info_element.signalSemaphoreCount;
            pipeline_stage_flags_size_total += rhi_submit_info_element.waitSemaphoreCount;
        }

        std::vector<VkCommandBuffer> vk_command_buffer_list_external(command_buffer_size_total);
        std::vector<VkSemaphore> vk_semaphore_list_external(semaphore_size_total);
        std::vector<VkSemaphore> vk_signal_semaphore_list_external(signal_semaphore_size_total);
        std::vector<VkPipelineStageFlags> vk_pipeline_stage_flags_list_external(pipeline_stage_flags_size_total);

        int command_buffer_size_current = 0;
        int semaphore_size_current = 0;
        int signal_semaphore_size_current = 0;
        int pipeline_stage_flags_size_current = 0;

        std::vector<VkSubmitInfo> vk_submit_info_list(submit_count);
        for (int i = 0; i < submit_count; ++i) {
            const auto& rhi_submit_info_element = submits[i];
            auto& vk_submit_info_element = vk_submit_info_list[i];

            vk_submit_info_element.sType = (VkStructureType)rhi_submit_info_element.sType;
            vk_submit_info_element.pNext = (const void*)rhi_submit_info_element.pNext;

            if (rhi_submit_info_element.commandBufferCount > 0) {
                vk_submit_info_element.commandBufferCount = rhi_submit_info_element.commandBufferCount;
                vk_submit_info_element.pCommandBuffers = &vk_command_buffer_list_external[command_buffer_size_current];
                int command_buffer_size = rhi_submit_info_element.commandBufferCount;
                for (int j = 0; j < command_buffer_size; ++j) {
                    const auto& rhi_command_buffer_element = rhi_submit_info_element.pCommandBuffers[j];
                    auto& vk_command_buffer_element = vk_command_buffer_list_external[command_buffer_size_current];
                    vk_command_buffer_element = ((VulkanCommandBuffer*)rhi_command_buffer_element)->getResource();
                    command_buffer_size_current++;
                }
            }

            if (rhi_submit_info_element.waitSemaphoreCount > 0) {
                vk_submit_info_element.waitSemaphoreCount = rhi_submit_info_element.waitSemaphoreCount;
                vk_submit_info_element.pWaitSemaphores = &vk_semaphore_list_external[semaphore_size_current];
                int semaphore_size = rhi_submit_info_element.waitSemaphoreCount;
                for (int j = 0; j < semaphore_size; ++j) {
                    const auto& rhi_semaphore_element = rhi_submit_info_element.pWaitSemaphores[j];
                    auto& vk_semaphore_element = vk_semaphore_list_external[semaphore_size_current];
                    vk_semaphore_element = ((VulkanSemaphore*)rhi_semaphore_element)->getResource();
                    semaphore_size_current++;
                }
            }

            if (rhi_submit_info_element.signalSemaphoreCount > 0) {
                vk_submit_info_element.signalSemaphoreCount = rhi_submit_info_element.signalSemaphoreCount;
                vk_submit_info_element.pSignalSemaphores = &vk_signal_semaphore_list_external[signal_semaphore_size_current];
                int signal_semaphore_size = rhi_submit_info_element.signalSemaphoreCount;
                for (int j = 0; j < signal_semaphore_size; ++j) {
                    const auto& rhi_signal_semaphore_element = rhi_submit_info_element.pSignalSemaphores[j];
                    auto& vk_signal_semaphore_element = vk_signal_semaphore_list_external[signal_semaphore_size_current];
                    vk_signal_semaphore_element = ((VulkanSemaphore*)rhi_signal_semaphore_element)->getResource();
                    signal_semaphore_size_current++;
                }
            }

            if (rhi_submit_info_element.waitSemaphoreCount > 0) {
                vk_submit_info_element.pWaitDstStageMask = &vk_pipeline_stage_flags_list_external[pipeline_stage_flags_size_current];
                int pipeline_stage_flags_size = rhi_submit_info_element.waitSemaphoreCount;
                for (int j = 0; j < pipeline_stage_flags_size; ++j) {
                    const auto& rhi_pipeline_stage_flags_element = rhi_submit_info_element.pWaitDstStageMask[j];
                    auto& vk_pipeline_stage_flags_element = vk_pipeline_stage_flags_list_external[pipeline_stage_flags_size_current];
                    vk_pipeline_stage_flags_element = (VkPipelineStageFlags)rhi_pipeline_stage_flags_element;
                    pipeline_stage_flags_size_current++;
                }
            }
        }

        if(
            (command_buffer_size_current!=command_buffer_size_total)
            ||(semaphore_size_current!=semaphore_size_total)
            ||(signal_semaphore_size_current!=signal_semaphore_size_total)
            || (pipeline_stage_flags_size_current != pipeline_stage_flags_size_total)
        ) {
            LOG_ERROR("submit info is not right!");
            return RHI_FALSE;
        }

        VkFence vk_fence = VK_NULL_HANDLE;
        if (fence != nullptr) {
            vk_fence = ((VulkanFence*)fence)->getResource();
        }

        if (vkQueueSubmit(((VulkanQueue*)queue)->getResource(), submit_count, vk_submit_info_list.data(), vk_fence) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkQueueSubmit failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::queueWaitIdle(RHIQueue* queue) {
        if (vkQueueWaitIdle(((VulkanQueue*)queue)->getResource()) == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkQueueWaitIdle failed!");
            return RHI_FALSE;
        }
    }

    void VulkanRHI::cmdPipelineBarrier(
        RHICommandBuffer* command_buffer,
        RHIPipelineStageFlags src_stage_mask,
        RHIPipelineStageFlags dst_stage_mask,
        RHIDependencyFlags dependency_flags,
        uint32_t memory_barrier_count,
        const RHIMemoryBarrier* memory_barriers,
        uint32_t buffer_memory_barrier_count,
        const RHIBufferMemoryBarrier* buffer_memory_barriers,
        uint32_t image_memory_barrier_count,
        const RHIImageMemoryBarrier* image_memory_barriers
    ) {
        std::vector<VkMemoryBarrier> vk_memory_barrier_list(memory_barrier_count);
        for (int i = 0; i < memory_barrier_count; ++i) {
            const auto& rhi_memory_barrier_element = memory_barriers[i];
            auto& vk_memory_barrier_element = vk_memory_barrier_list[i];
            vk_memory_barrier_element.sType = (VkStructureType)rhi_memory_barrier_element.sType;
            vk_memory_barrier_element.pNext = (const void*)rhi_memory_barrier_element.pNext;
            vk_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_memory_barrier_element.srcAccessMask;
            vk_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_memory_barrier_element.dstAccessMask;
        }

        std::vector<VkBufferMemoryBarrier> vk_buffer_memory_barrier_list(buffer_memory_barrier_count);
        for (int i = 0; i < buffer_memory_barrier_count; ++i) {
            const auto& rhi_buffer_memory_barrier_element = buffer_memory_barriers[i];
            auto& vk_buffer_memory_barrier_element = vk_buffer_memory_barrier_list[i];
            vk_buffer_memory_barrier_element.sType = (VkStructureType)rhi_buffer_memory_barrier_element.sType;
            vk_buffer_memory_barrier_element.pNext = (const void*)rhi_buffer_memory_barrier_element.pNext;
            vk_buffer_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_buffer_memory_barrier_element.srcAccessMask;
            vk_buffer_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_buffer_memory_barrier_element.dstAccessMask;
            vk_buffer_memory_barrier_element.srcQueueFamilyIndex = rhi_buffer_memory_barrier_element.srcQueueFamilyIndex;
            vk_buffer_memory_barrier_element.dstQueueFamilyIndex = rhi_buffer_memory_barrier_element.dstQueueFamilyIndex;
            vk_buffer_memory_barrier_element.buffer = ((VulkanBuffer*)rhi_buffer_memory_barrier_element.buffer)->getResource();
            vk_buffer_memory_barrier_element.offset = (VkDeviceSize)rhi_buffer_memory_barrier_element.offset;
            vk_buffer_memory_barrier_element.size = (VkDeviceSize)rhi_buffer_memory_barrier_element.size;
        }

        std::vector<VkImageMemoryBarrier> vk_image_memory_barrier_list(image_memory_barrier_count);
        for (int i = 0; i < image_memory_barrier_count; ++i) {
            const auto& rhi_image_memory_barrier_element = image_memory_barriers[i];
            auto& vk_image_memory_barrier_element = vk_image_memory_barrier_list[i];

            VkImageSubresourceRange image_subresource_range{};
            image_subresource_range.aspectMask = (VkImageAspectFlags)rhi_image_memory_barrier_element.subresourceRange.aspectMask;
            image_subresource_range.baseMipLevel = rhi_image_memory_barrier_element.subresourceRange.baseMipLevel;
            image_subresource_range.levelCount = rhi_image_memory_barrier_element.subresourceRange.levelCount;
            image_subresource_range.baseArrayLayer = rhi_image_memory_barrier_element.subresourceRange.baseArrayLayer;
            image_subresource_range.layerCount = rhi_image_memory_barrier_element.subresourceRange.layerCount;

            vk_image_memory_barrier_element.sType = (VkStructureType)rhi_image_memory_barrier_element.sType;
            vk_image_memory_barrier_element.pNext = (const void*)rhi_image_memory_barrier_element.pNext;
            vk_image_memory_barrier_element.srcAccessMask = (VkAccessFlags)rhi_image_memory_barrier_element.srcAccessMask;
            vk_image_memory_barrier_element.dstAccessMask = (VkAccessFlags)rhi_image_memory_barrier_element.dstAccessMask;
            vk_image_memory_barrier_element.oldLayout = (VkImageLayout)rhi_image_memory_barrier_element.oldLayout;
            vk_image_memory_barrier_element.newLayout = (VkImageLayout)rhi_image_memory_barrier_element.newLayout;
            vk_image_memory_barrier_element.srcQueueFamilyIndex = rhi_image_memory_barrier_element.srcQueueFamilyIndex;
            vk_image_memory_barrier_element.dstQueueFamilyIndex = rhi_image_memory_barrier_element.dstQueueFamilyIndex;
            vk_image_memory_barrier_element.image = ((VulkanImage*)rhi_image_memory_barrier_element.image)->getResource();
            vk_image_memory_barrier_element.subresourceRange = image_subresource_range;
        }

        vkCmdPipelineBarrier(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            (RHIPipelineStageFlags)src_stage_mask,
            (RHIPipelineStageFlags)dst_stage_mask,
            (RHIDependencyFlags)dependency_flags,
            memory_barrier_count,
            vk_memory_barrier_list.data(),
            buffer_memory_barrier_count,
            vk_buffer_memory_barrier_list.data(),
            image_memory_barrier_count,
            vk_image_memory_barrier_list.data()
        );
    }

    void VulkanRHI::cmdDraw(
        RHICommandBuffer* command_buffer,
        uint32_t vertex_count,
        uint32_t instance_count,
        uint32_t first_vertex,
        uint32_t first_instance
    ) {
        vkCmdDraw(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            vertex_count, instance_count, first_vertex, first_instance
        );
    }

    void VulkanRHI::cmdDispatch(
        RHICommandBuffer* command_buffer,
        uint32_t group_count_x,
        uint32_t group_count_y,
        uint32_t group_count_z
    ) {
        vkCmdDispatch(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            group_count_x, group_count_y, group_count_z
        );
    }

    void VulkanRHI::cmdDispatchIndirect(
        RHICommandBuffer* command_buffer,
        RHIBuffer* buffer,
        RHIDeviceSize offset
    ) {
        vkCmdDispatchIndirect(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            ((VulkanBuffer*)buffer)->getResource(), offset
        );
    }

    void VulkanRHI::cmdCopyImageToBuffer(
        RHICommandBuffer* command_buffer,
        RHIImage* src_image,
        RHIImageLayout src_image_layout,
        RHIBuffer* dst_buffer,
        uint32_t region_count,
        const RHIBufferImageCopy* regions
    ) {
        std::vector<VkBufferImageCopy> vk_buffer_image_copy_list(region_count);
        for (int i = 0; i < region_count; ++i) {
            const auto& rhi_buffer_image_copy_element = regions[i];
            auto& vk_buffer_image_copy_element = vk_buffer_image_copy_list[i];

            VkImageSubresourceLayers image_subresource_layers{};
            image_subresource_layers.aspectMask = (VkImageAspectFlags)rhi_buffer_image_copy_element.imageSubresource.aspectMask;
            image_subresource_layers.mipLevel = rhi_buffer_image_copy_element.imageSubresource.mipLevel;
            image_subresource_layers.baseArrayLayer = rhi_buffer_image_copy_element.imageSubresource.baseArrayLayer;
            image_subresource_layers.layerCount = rhi_buffer_image_copy_element.imageSubresource.layerCount;

            VkOffset3D offset_3d{};
            offset_3d.x = rhi_buffer_image_copy_element.imageOffset.x;
            offset_3d.y = rhi_buffer_image_copy_element.imageOffset.y;
            offset_3d.z = rhi_buffer_image_copy_element.imageOffset.z;

            VkExtent3D extent_3d{};
            extent_3d.width = rhi_buffer_image_copy_element.imageExtent.width;
            extent_3d.height = rhi_buffer_image_copy_element.imageExtent.height;
            extent_3d.depth = rhi_buffer_image_copy_element.imageExtent.depth;

            vk_buffer_image_copy_element.bufferOffset = (VkDeviceSize)rhi_buffer_image_copy_element.bufferOffset;
            vk_buffer_image_copy_element.bufferRowLength = rhi_buffer_image_copy_element.bufferRowLength;
            vk_buffer_image_copy_element.bufferImageHeight = rhi_buffer_image_copy_element.bufferImageHeight;
            vk_buffer_image_copy_element.imageSubresource = image_subresource_layers;
            vk_buffer_image_copy_element.imageOffset = offset_3d;
            vk_buffer_image_copy_element.imageExtent = extent_3d;
        }

        vkCmdCopyImageToBuffer(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            ((VulkanImage*)src_image)->getResource(),
            (VkImageLayout)src_image_layout,
            ((VulkanBuffer*)dst_buffer)->getResource(),
            region_count,
            vk_buffer_image_copy_list.data()
        );
    }

    void VulkanRHI::cmdCopyImageToImage(
        RHICommandBuffer* command_buffer,
        RHIImage* src_image,
        RHIImageAspectFlagBits src_flags,
        RHIImage* dst_image,
        RHIImageAspectFlagBits dst_flags,
        uint32_t width,
        uint32_t height
    ) {
        VkImageCopy image_copy_region{};
        image_copy_region.srcSubresource = { (VkImageAspectFlags)src_flags,0,0,1 };
        image_copy_region.srcOffset = { 0,0,0 };
        image_copy_region.dstSubresource = { (VkImageAspectFlags)dst_flags,0,0,1 };
        image_copy_region.dstOffset = { 0,0,0 };
        image_copy_region.extent = { width,height,1 };

        vkCmdCopyImage(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            ((VulkanImage*)src_image)->getResource(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            ((VulkanImage*)dst_image)->getResource(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &image_copy_region
        );
    }

    void VulkanRHI::cmdCopyBuffer(
        RHICommandBuffer* command_buffer,
        RHIBuffer* src_buffer,
        RHIBuffer* dst_buffer,
        uint32_t region_count,
        RHIBufferCopy* regions
    ) {
        std::vector<VkBufferCopy> vk_copy_region_list(region_count);
        for (int i = 0; i < region_count; ++i) {
            const auto& rhi_copy_region_element = regions[i];
            auto& vk_copy_region_element = vk_copy_region_list[i];
            vk_copy_region_element.srcOffset = rhi_copy_region_element.srcOffset;
            vk_copy_region_element.dstOffset = rhi_copy_region_element.dstOffset;
            vk_copy_region_element.size = rhi_copy_region_element.size;
        }

        vkCmdCopyBuffer(
            ((VulkanCommandBuffer*)command_buffer)->getResource(),
            ((VulkanBuffer*)src_buffer)->getResource(),
            ((VulkanBuffer*)dst_buffer)->getResource(),
            region_count, vk_copy_region_list.data()
        );
    }

    void VulkanRHI::createCommandBuffers() {
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandBufferCount = 1U;

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i) {
            command_buffer_allocate_info.commandPool = m_command_pools[i];
            VkCommandBuffer vk_command_buffer;
            if (vkAllocateCommandBuffers(m_device, &command_buffer_allocate_info, &vk_command_buffer) != VK_SUCCESS) {
                LOG_ERROR("vkAllocateCommandBuffers failed");
            }
            m_vk_command_buffers[i] = vk_command_buffer;
            m_command_buffers[i] = new VulkanCommandBuffer();
            ((VulkanCommandBuffer*)m_command_buffers[i])->setResource(vk_command_buffer);
        }
    }

    void VulkanRHI::createDescriptorPool() {
        // Since DescriptorSet should be treated as asset in Vulkan, DescriptorPool
        // should be big enough, and thus we can sub-allocate DescriptorSet from
        // DescriptorPool merely as we sub-allocate Buffer/Image from DeviceMemory.

        VkDescriptorPoolSize pool_sizes[7];
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        pool_sizes[0].descriptorCount = 3 + 2 + 2 + 2 + 1 + 1 + 3 + 3;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[1].descriptorCount = 1 + 1 + 1 * _max_vertex_blending_mesh_count;
        pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[2].descriptorCount = 1 * _max_material_count;
        pool_sizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[3].descriptorCount = 3 + 5 * _max_material_count + 1 + 1; // ImGui_ImplVulkan_CreateDeviceObjects
        pool_sizes[4].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        pool_sizes[4].descriptorCount = 4 + 1 + 1 + 2;
        pool_sizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_sizes[5].descriptorCount = 3;
        pool_sizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        pool_sizes[6].descriptorCount = 1;

        VkDescriptorPoolCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
        create_info.pPoolSizes = pool_sizes;
        create_info.maxSets = 1 + 1 + 1 + _max_material_count + _max_vertex_blending_mesh_count + 1 + 1; // +skybox + axis descriptor set
        create_info.flags = 0U;

        if (vkCreateDescriptorPool(m_device, &create_info, nullptr, &m_vk_descriptor_pool) != VK_SUCCESS) {
            LOG_ERROR("vkCreateDescriptorPool failed!");
        }

        m_descriptor_pool = new VulkanDescriptorPool();
        ((VulkanDescriptorPool*)m_descriptor_pool)->setResource(m_vk_descriptor_pool);
    }

    void VulkanRHI::createSyncPrimitives() {
        VkSemaphoreCreateInfo semaphore_create_info{};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_create_info{};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i) {
            m_image_available_for_texturescopy_semaphores[i] = new VulkanSemaphore();
            if (
                vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_image_available_for_render_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_image_finished_for_presentation_samaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &((VulkanSemaphore*)m_image_available_for_texturescopy_semaphores[i])->getResource()) != VK_SUCCESS ||
                vkCreateFence(m_device, &fence_create_info, nullptr, &m_is_frame_in_flight_fences[i]) != VK_SUCCESS
                ) {
                LOG_ERROR("vkCreateSemaphore||vkCreateFence failed!");
            }
            m_rhi_is_frame_in_flight_fences[i] = new VulkanFence();
            ((VulkanFence*)m_rhi_is_frame_in_flight_fences[i])->setResource(m_is_frame_in_flight_fences[i]);
        }
    }

    void VulkanRHI::createFramebufferImageAndView() {
        VulkanUtil::createImage(
            m_physical_device, m_device,
            m_swapchain_extent.width, m_swapchain_extent.height,
            (VkFormat)m_depth_image_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            ((VulkanImage*)m_depth_image)->getResource(),
            m_depth_image_memory, 0, 1, 1
        );
        ((VulkanImageView*)m_depth_image_view)->setResource(
            VulkanUtil::createImageView(
                m_device, 
                ((VulkanImage*)m_depth_image)->getResource(), 
                (VkFormat)m_depth_image_format,
                VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1
            )
        );
    }

    RHISampler* VulkanRHI::getOrCreateDefaultSampler(RHIDefaultSamplerType type) {
        switch (type)
        {
        case Dao::Default_Sampler_Linear:
            if (_linear_sampler == nullptr) {
                _linear_sampler = new VulkanSampler();
                ((VulkanSampler*)_linear_sampler)->setResource(
                    VulkanUtil::getOrCreateLinearSampler(m_physical_device, m_device)
                );
            }
            return _linear_sampler;
            break;
        case Dao::Default_Sampler_Nearest:
            if (_nearest_sampler == nullptr) {
                _nearest_sampler = new VulkanSampler();
                ((VulkanSampler*)_nearest_sampler)->setResource(
                    VulkanUtil::getOrCreateNearestSampler(m_physical_device, m_device)
                );
            }
            return _nearest_sampler;
            break;
        default:
            return nullptr;
            break;
        }
    }

    RHISampler* VulkanRHI::getOrCreateMipmapSampler(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) {
            LOG_ERROR("width == 0 || height == 0");
            return nullptr;
        }
        RHISampler* sampler;
        uint32_t mip_levels = floor(log2(std::max(width, height))) + 1;
        auto find_sampler = _mipmap_sampler_map.find(mip_levels);
        if (find_sampler != _mipmap_sampler_map.end()) {
            return find_sampler->second;
        }
        else {
            sampler = new VulkanSampler();
            ((VulkanSampler*)sampler)->setResource(
                VulkanUtil::getOrCreateMipmapSampler(m_physical_device, m_device, width, height)
            );
            _mipmap_sampler_map.insert(std::make_pair(mip_levels, sampler));
            return sampler;
        }
    }

    RHIShader* VulkanRHI::createShaderModule(const std::vector<unsigned char>& shader_code) {
        RHIShader* shader = new VulkanShader();
        VkShaderModule vk_shader = VulkanUtil::createShaderModule(m_device, shader_code);
        ((VulkanShader*)shader)->setResource(vk_shader);
        return shader;
    }

    void VulkanRHI::createBuffer(
        RHIDeviceSize size,
        RHIBufferUsageFlags usage,
        RHIMemoryPropertyFlags properties,
        RHIBuffer*& buffer,
        RHIDeviceMemory*& buffer_memory
    ) {
        VkBuffer vk_buffer;
        VkDeviceMemory vk_device_memory;
        VulkanUtil::createBuffer(m_physical_device, m_device, size, usage, properties, vk_buffer, vk_device_memory);
        buffer = new VulkanBuffer();
        buffer_memory = new VulkanDeviceMemory();
        ((VulkanBuffer*)buffer)->setResource(vk_buffer);
        ((VulkanDeviceMemory*)buffer_memory)->setResource(vk_device_memory);
    }

    void VulkanRHI::createBufferAndInitialize(
        RHIBufferUsageFlags usage,
        RHIMemoryPropertyFlags properties,
        RHIBuffer*& buffer,
        RHIDeviceMemory*& buffer_memory,
        RHIDeviceSize size,
        void* data,
        int data_size
    ) {
        VkBuffer vk_buffer;
        VkDeviceMemory vk_device_memory;
        VulkanUtil::createBufferAndInitalize(
            m_device, m_physical_device, usage, properties,
            &vk_buffer, &vk_device_memory, size, data, data_size
        );
        buffer = new VulkanBuffer();
        buffer_memory = new VulkanDeviceMemory();
        ((VulkanBuffer*)buffer)->setResource(vk_buffer);
        ((VulkanDeviceMemory*)buffer_memory)->setResource(vk_device_memory);
    }

    bool VulkanRHI::createBufferVMA(
        VmaAllocator allocator,
        const RHIBufferCreateInfo* buffer_create_info,
        const VmaAllocationCreateInfo* allocation_create_info,
        RHIBuffer*& buffer,
        VmaAllocation* allocation,
        VmaAllocationInfo* allocation_info
    ) {
        VkBuffer vk_buffer;
        VkBufferCreateInfo vk_buffer_create_info{};
        vk_buffer_create_info.sType = (VkStructureType)buffer_create_info->sType;
        vk_buffer_create_info.pNext = (const void*)buffer_create_info->pNext;
        vk_buffer_create_info.flags = (VkBufferCreateFlags)buffer_create_info->flags;
        vk_buffer_create_info.size = (VkDeviceSize)buffer_create_info->size;
        vk_buffer_create_info.usage = (VkBufferUsageFlags)buffer_create_info->usage;
        vk_buffer_create_info.sharingMode = (VkSharingMode)buffer_create_info->sharingMode;
        vk_buffer_create_info.queueFamilyIndexCount = buffer_create_info->queueFamilyIndexCount;
        vk_buffer_create_info.pQueueFamilyIndices = (const uint32_t*)buffer_create_info->pQueueFamilyIndices;
        buffer = new VulkanBuffer();
        VkResult result = vmaCreateBuffer(
            allocator, &vk_buffer_create_info, allocation_create_info,
            &vk_buffer, allocation, allocation_info
        );
        ((VulkanBuffer*)buffer)->setResource(vk_buffer);
        if (result == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vmaCreateBuffer failed!");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::createBufferWithAlignmentVMA(
        VmaAllocator allocator,
        const RHIBufferCreateInfo* buffer_create_info,
        const VmaAllocationCreateInfo* allocation_create_info,
        RHIDeviceSize min_alignment,
        RHIBuffer*& buffer,
        VmaAllocation* allocation,
        VmaAllocationInfo* allocation_info
    ) {
        VkBuffer vk_buffer;
        VkBufferCreateInfo vk_buffer_create_info{};
        vk_buffer_create_info.sType = (VkStructureType)buffer_create_info->sType;
        vk_buffer_create_info.pNext = (const void*)buffer_create_info->pNext;
        vk_buffer_create_info.flags = (VkBufferCreateFlags)buffer_create_info->flags;
        vk_buffer_create_info.size = (VkDeviceSize)buffer_create_info->size;
        vk_buffer_create_info.usage = (VkBufferUsageFlags)buffer_create_info->usage;
        vk_buffer_create_info.sharingMode = (VkSharingMode)buffer_create_info->sharingMode;
        vk_buffer_create_info.queueFamilyIndexCount = buffer_create_info->queueFamilyIndexCount;
        vk_buffer_create_info.pQueueFamilyIndices = (const uint32_t*)buffer_create_info->pQueueFamilyIndices;

        buffer = new VulkanBuffer();
        VkResult result = vmaCreateBufferWithAlignment(
            allocator, &vk_buffer_create_info, allocation_create_info,
            min_alignment, &vk_buffer, allocation, allocation_info
        );
        ((VulkanBuffer*)buffer)->setResource(vk_buffer);
        if (result == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vmaCreateBufferWithAlignment failed!");
            return RHI_FALSE;
        }
    }

    void VulkanRHI::copyBuffer(
        RHIBuffer* src_buffer,
        RHIBuffer* dst_buffer,
        RHIDeviceSize src_offset,
        RHIDeviceSize dst_offset,
        RHIDeviceSize size
    ) {
        VkBuffer vk_src_buffer = ((VulkanBuffer*)src_buffer)->getResource();
        VkBuffer vk_dst_buffer = ((VulkanBuffer*)dst_buffer)->getResource();
        VulkanUtil::copyBuffer(this, vk_src_buffer, vk_dst_buffer, src_offset, dst_offset, size);
    }

    void VulkanRHI::createImage(
        uint32_t image_width,
        uint32_t image_height,
        RHIFormat format,
        RHIImageTiling image_tiling,
        RHIImageUsageFlags image_usage_flags,
        RHIMemoryPropertyFlags memory_properties,
        RHIImage*& image,
        RHIDeviceMemory*& memory,
        RHIImageCreateFlags image_create_flags,
        uint32_t array_layers,
        uint32_t mip_levels
    ) {
        VkImage vk_image;
        VkDeviceMemory vk_device_memory;
        VulkanUtil::createImage(
            m_physical_device, m_device, image_width, image_height,
            (VkFormat)format, (VkImageTiling)image_tiling,
            (VkImageUsageFlags)image_usage_flags, (VkMemoryPropertyFlags)memory_properties,
            vk_image, vk_device_memory, (VkImageCreateFlags)image_create_flags, array_layers, mip_levels
        );
        image = new VulkanImage();
        memory = new VulkanDeviceMemory();
        ((VulkanImage*)image)->setResource(vk_image);
        ((VulkanDeviceMemory*)memory)->setResource(vk_device_memory);
    }

    void VulkanRHI::createImageView(
        RHIImage* image,
        RHIFormat format,
        RHIImageAspectFlags image_aspect_flags,
        RHIImageViewType view_type,
        uint32_t layout_count,
        uint32_t mip_levels,
        RHIImageView*& image_view
    ) {
        image_view = new VulkanImageView();
        VkImage vk_image = ((VulkanImage*)image)->getResource();
        VkImageView vk_image_view;
        vk_image_view = VulkanUtil::createImageView(
            m_device, vk_image, (VkFormat)format, image_aspect_flags,
            (VkImageViewType)view_type, layout_count, mip_levels
        );
        ((VulkanImageView*)image_view)->setResource(vk_image_view);
    }

    void VulkanRHI::createGlobalImage(
        RHIImage*& image,
        RHIImageView*& image_view,
        VmaAllocation& image_allocation,
        uint32_t texture_image_width,
        uint32_t texture_image_height,
        void* textue_image_pixels,
        RHIFormat texture_image_format,
        uint32_t mip_levels
    ) {
        VkImage vk_image;
        VkImageView vk_image_view;
        VulkanUtil::createGlobalImage(
            this, vk_image, vk_image_view, image_allocation,
            texture_image_width, texture_image_height,
            textue_image_pixels, texture_image_format, mip_levels
        );
        image = new VulkanImage();
        image_view = new VulkanImageView();
        ((VulkanImage*)image)->setResource(vk_image);
        ((VulkanImageView*)image_view)->setResource(vk_image_view);
    }

    void VulkanRHI::createCubeMap(
        RHIImage*& image,
        RHIImageView*& image_view,
        VmaAllocation& image_allocation,
        uint32_t texture_image_width,
        uint32_t texture_image_height,
        std::array<void*, 6> texture_image_pixels,
        RHIFormat texture_image_format,
        uint32_t mip_levels
    ) {
        VkImage vk_image;
        VkImageView vk_image_view;
        VulkanUtil::createCubeMap(
            this, vk_image, vk_image_view, image_allocation,
            texture_image_width, texture_image_height,
            texture_image_pixels, texture_image_format, mip_levels
        );
        image = new VulkanImage();
        image_view = new VulkanImageView();
        ((VulkanImage*)image)->setResource(vk_image);
        ((VulkanImageView*)image_view)->setResource(vk_image_view);
    }

    void VulkanRHI::createSwapchainImageViews() {
        m_swapchain_imageviews.resize(m_swapchain_images.size());

        for (size_t i = 0; i < m_swapchain_images.size(); ++i) {
            VkImageView vk_image_view;
            vk_image_view = VulkanUtil::createImageView(
                m_device, m_swapchain_images[i], (VkFormat)m_swapchian_image_format,
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, 1
            );
            m_swapchain_imageviews[i] = new VulkanImageView();
            ((VulkanImageView*)m_swapchain_imageviews[i])->setResource(vk_image_view);
        }
    }

    void VulkanRHI::createAssetAllocator() {
        VmaVulkanFunctions vulkan_functions = {};
        vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocator_create_info = {};
        allocator_create_info.vulkanApiVersion = _vulkan_api_version;
        allocator_create_info.physicalDevice = m_physical_device;
        allocator_create_info.device = m_device;
        allocator_create_info.instance = m_instance;
        allocator_create_info.pVulkanFunctions = &vulkan_functions;

        vmaCreateAllocator(&allocator_create_info, &m_assets_allocator);
    }

    bool VulkanRHI::allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* allocate_info, RHIDescriptorSet*& descriptor_sets) {
        int descriptor_set_count = allocate_info->descriptorSetCount;
        std::vector<VkDescriptorSetLayout> vk_descriptor_set_layout_list(descriptor_set_count);
        for (int i = 0; i < descriptor_set_count; ++i) {
            const auto& rhi_descriptor_set_layout_element = allocate_info->pSetLayouts[i];
            auto& vk_descriptor_set_layut_element = vk_descriptor_set_layout_list[i];
            vk_descriptor_set_layut_element = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element)->getResource();
            VulkanDescriptorSetLayout* test = ((VulkanDescriptorSetLayout*)rhi_descriptor_set_layout_element);
            test = nullptr;
        }

        VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
        descriptor_set_allocate_info.sType = (VkStructureType)allocate_info->sType;
        descriptor_set_allocate_info.pNext = (const void*)allocate_info->pNext;
        descriptor_set_allocate_info.descriptorPool = ((VulkanDescriptorPool*)allocate_info->descriptorPool)->getResource();
        descriptor_set_allocate_info.descriptorSetCount = allocate_info->descriptorSetCount;
        descriptor_set_allocate_info.pSetLayouts = vk_descriptor_set_layout_list.data();

        std::vector<VkDescriptorSet> vk_descriptor_set_list(descriptor_set_count);
        descriptor_sets = new VulkanDescriptorSet[descriptor_set_count];
        VkResult result = vkAllocateDescriptorSets(m_device, &descriptor_set_allocate_info, vk_descriptor_set_list.data());
        for (int i = 0; i < descriptor_set_count; ++i) {
            const auto& vk_descriptor_set = vk_descriptor_set_list[i];
            ((VulkanDescriptorSet*)&descriptor_sets[i])->setResource(vk_descriptor_set);
        }
        if (result == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkAllocateDescriptorSets failed");
            return RHI_FALSE;
        }
    }

    bool VulkanRHI::allocateCommandBuffers(const RHICommandBufferAllocateInfo* allocate_info, RHICommandBuffer*& command_buffers) {
        VkCommandBufferAllocateInfo command_buffer_allocate_info{};
        command_buffer_allocate_info.sType = (VkStructureType)allocate_info->sType;
        command_buffer_allocate_info.pNext = (const void*)allocate_info->pNext;
        command_buffer_allocate_info.commandPool = ((VulkanCommandPool*)allocate_info->commandPool)->getResource();
        command_buffer_allocate_info.level = (VkCommandBufferLevel)allocate_info->level;
        command_buffer_allocate_info.commandBufferCount = allocate_info->commandBufferCount;

        int command_buffer_count = allocate_info->commandBufferCount;
        std::vector<VkCommandBuffer> vk_command_buffer_list(command_buffer_count);
        command_buffers = new VulkanCommandBuffer[command_buffer_count];
        VkResult result = vkAllocateCommandBuffers(m_device, &command_buffer_allocate_info, vk_command_buffer_list.data());
        for (int i = 0; i < command_buffer_count; ++i) {
            const auto& vk_command_buffer = vk_command_buffer_list[i];
            ((VulkanCommandBuffer*)&command_buffers[i])->setResource(vk_command_buffer);
        }
        if (result == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkAllocateCommandBuffers failed!");
            return RHI_FALSE;
        }
    }

    void VulkanRHI::createSwapchain() {
        SwapChainSupportDetails swapchain_support_details = querySwapChainSupport(m_physical_device);
        VkSurfaceFormatKHR chosen_surface_format = chooseSwapchainSurfaceFormatFromDetails(swapchain_support_details.formats);
        VkPresentModeKHR chosen_present_mode = chooseSwapchainPresentModeFromDetails(swapchain_support_details.presentModes);
        VkExtent2D chosen_extent = chooseSwapchainExtentFromDetails(swapchain_support_details.capabilities);
        uint32_t image_count = swapchain_support_details.capabilities.minImageCount + 1;
        if (swapchain_support_details.capabilities.maxImageCount > 0 &&
            image_count > swapchain_support_details.capabilities.maxImageCount
            ) {
            image_count = swapchain_support_details.capabilities.minImageCount;
        }

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = m_surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = chosen_surface_format.format;
        create_info.imageColorSpace = chosen_surface_format.colorSpace;
        create_info.imageExtent = chosen_extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = { m_queue_indices.graphics_family.value(),m_queue_indices.present_family.value() };
        if (m_queue_indices.graphics_family != m_queue_indices.present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.preTransform = swapchain_support_details.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = chosen_present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS) {
            LOG_ERROR("vkCreateSwapchainKHR failed");
        }
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
        m_swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());
        m_swapchian_image_format = (RHIFormat)chosen_surface_format.format;
        m_swapchain_extent.height = chosen_extent.height;
        m_swapchain_extent.width = chosen_extent.width;
        m_scissor = { {0,0},{m_swapchain_extent.width,m_swapchain_extent.height} };
    }

    void VulkanRHI::clearSwapChain() {
        for (auto image_view : m_swapchain_imageviews) {
            vkDestroyImageView(m_device, ((VulkanImageView*)image_view)->getResource(), nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }

    void VulkanRHI::destroyDefaultSampler(RHIDefaultSamplerType type) {
        switch (type)
        {
        case Dao::Default_Sampler_Linear:
            VulkanUtil::destroyLinearSampler(m_device);
            delete(_linear_sampler);
            break;
        case Dao::Default_Sampler_Nearest:
            VulkanUtil::destroyNearestSampler(m_device);
            delete(_nearest_sampler);
            break;
        default:
            break;
        }
    }

    void VulkanRHI::destroyMipmappedSampler() {
        VulkanUtil::destroyMipmappedSampler(m_device);
        for (auto sampler : _mipmap_sampler_map) {
            delete(sampler.second);
        }
        _mipmap_sampler_map.clear();
    }

    void VulkanRHI::destroyShaderModule(RHIShader* shader_module) {
        vkDestroyShaderModule(m_device, ((VulkanShader*)shader_module)->getResource(), nullptr);
        delete(shader_module);
    }

    void VulkanRHI::destroySemaphore(RHISemaphore* semaphore) {
        vkDestroySemaphore(m_device, ((VulkanSemaphore*)semaphore)->getResource(), nullptr);
    }

    void VulkanRHI::destroySampler(RHISampler* sampler) {
        vkDestroySampler(m_device, ((VulkanSampler*)sampler)->getResource(), nullptr);
    }

    void VulkanRHI::destroyInstance(RHIInstance* instance) {
        vkDestroyInstance(((VulkanInstance*)instance)->getResource(), nullptr);
    }

    void VulkanRHI::destroyImageView(RHIImageView* image_view) {
        vkDestroyImageView(m_device, ((VulkanImageView*)image_view)->getResource(), nullptr);
    }

    void VulkanRHI::destroyImage(RHIImage* image) {
        vkDestroyImage(m_device, ((VulkanImage*)image)->getResource(), nullptr);
    }

    void VulkanRHI::destroyFrameBuffer(RHIFramebuffer* frame_buffer) {
        vkDestroyFramebuffer(m_device, ((VulkanFramebuffer*)frame_buffer)->getResource(), nullptr);
    }

    void VulkanRHI::destroyFence(RHIFence* fence) {
        vkDestroyFence(m_device, ((VulkanFence*)fence)->getResource(), nullptr);
    }

    void VulkanRHI::destroyDevice() {
        vkDestroyDevice(m_device, nullptr);
    }

    void VulkanRHI::destroyCommandPool(RHICommandPool* command_pool) {
        vkDestroyCommandPool(m_device, ((VulkanCommandPool*)command_pool)->getResource(), nullptr);
    }

    void VulkanRHI::destroyBuffer(RHIBuffer*& buffer) {
        vkDestroyBuffer(m_device, ((VulkanBuffer*)buffer)->getResource(), nullptr);
        RHI_DELETE_PTR(buffer);
    }

    void VulkanRHI::freeCommandBuffers(
        RHICommandPool* command_pool, 
        uint32_t command_buffer_count, 
        RHICommandBuffer* command_buffers
    ) {
        std::vector<VkCommandBuffer> vk_command_buffer_list(command_buffer_count);
        for (uint32_t i = 0; i < command_buffer_count; ++i) {
            const auto& rhi_command_buffer = ((VulkanCommandBuffer*)&(command_buffers[i]));
            auto& vk_command_buffer = vk_command_buffer_list[i];
            vk_command_buffer = rhi_command_buffer->getResource();
        }
        vkFreeCommandBuffers(m_device, ((VulkanCommandPool*)command_pool)->getResource(), command_buffer_count, vk_command_buffer_list.data());
    }

    void VulkanRHI::freeMemory(RHIDeviceMemory*& memory) {
        vkFreeMemory(m_device, ((VulkanDeviceMemory*)memory)->getResource(), nullptr);
        RHI_DELETE_PTR(memory);
    }

    bool VulkanRHI::mapMemory(
        RHIDeviceMemory* memory,
        RHIDeviceSize offset,
        RHIDeviceSize size,
        RHIMemoryMapFlags flags,
        void** data
    ) {
        VkResult result = vkMapMemory(
            m_device, ((VulkanDeviceMemory*)memory)->getResource(),
            offset, size, (VkMemoryMapFlags)flags, data
        );
        if (result == VK_SUCCESS) {
            return RHI_SUCCESS;
        }
        else {
            LOG_ERROR("vkMapMemory failed!");
            return false;
        }
    }

    void VulkanRHI::unmapMemory(RHIDeviceMemory* memory) {
        vkUnmapMemory(m_device, ((VulkanDeviceMemory*)memory)->getResource());
    }

    void VulkanRHI::invalidateMappedMemoryRanges(
        void* next,
        RHIDeviceMemory* memory,
        RHIDeviceSize offset,
        RHIDeviceSize size
    ) {
        VkMappedMemoryRange mapped_range{};
        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.pNext = next;
        mapped_range.memory = ((VulkanDeviceMemory*)memory)->getResource();
        mapped_range.offset = offset;
        mapped_range.size = size;
        vkInvalidateMappedMemoryRanges(m_device, 1, &mapped_range);

    }

    void VulkanRHI::flushMappedMemoryRanges(
        void* next,
        RHIDeviceMemory* memory,
        RHIDeviceSize offset,
        RHIDeviceSize size
    ) {
        VkMappedMemoryRange mapped_range{};
        mapped_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mapped_range.pNext = next;
        mapped_range.memory = ((VulkanDeviceMemory*)memory)->getResource();
        mapped_range.offset = offset;
        mapped_range.size = size;
        vkFlushMappedMemoryRanges(m_device, 1, &mapped_range);
    }

    RHISemaphore*& VulkanRHI::getTextureCopySemaphore(uint32_t index) {
        return m_image_available_for_texturescopy_semaphores[index];
    }

    void VulkanRHI::recreateSwapchain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }
        VkResult result = f_vkWaitForFences(m_device, k_max_frames_in_flight, m_is_frame_in_flight_fences, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS) {
            LOG_ERROR("f_vkWaitForFences failed!");
            return;
        }
        destroyImageView(m_depth_image_view);
        destroyImage(m_depth_image);
        vkFreeMemory(m_device, m_depth_image_memory, nullptr);
        clearSwapChain();
        createSwapchain();
        createSwapchainImageViews();
        createFramebufferImageAndView();
    }

    VkResult VulkanRHI::createDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* debug_messenger
    ) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, create_info, allocator, debug_messenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void VulkanRHI::destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* allocator
    ) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debug_messenger, allocator);
        }
    }

    QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice physical_device) {
        QueueFamilyIndices indices;
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
        int i = 0;
        for (const auto& queue_family : queue_families) {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i;
            }
            if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.compute_family = i;
            }
            VkBool32 is_present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, m_surface, &is_present_support);
            if (is_present_support) {
                indices.present_family = i;
            }
            if (indices.isComplete()) {
                break;
            }
            ++i;
        }
        return indices;
    }

    bool VulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice physical_device) {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());
        std::set<std::string> required_extensions(_device_extensions.begin(), _device_extensions.end());
        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }
        return required_extensions.empty();
    }

    bool VulkanRHI::isDeviceSuitable(VkPhysicalDevice physical_device) {
        auto queue_indices = findQueueFamilies(physical_device);
        bool is_extension_supported = checkDeviceExtensionSupport(physical_device);
        bool is_swapchain_adequate = false;
        if (is_extension_supported) {
            SwapChainSupportDetails swapchian_support_detail = querySwapChainSupport(physical_device);
            is_swapchain_adequate = !swapchian_support_detail.formats.empty() && !swapchian_support_detail.presentModes.empty();
        }
        VkPhysicalDeviceFeatures physical_device_features;
        vkGetPhysicalDeviceFeatures(physical_device, &physical_device_features);
        if (!queue_indices.isComplete() || !is_swapchain_adequate || !physical_device_features.samplerAnisotropy) {
            return false;
        }
        return true;
    }

    SwapChainSupportDetails VulkanRHI::querySwapChainSupport(VkPhysicalDevice physical_device) {
        SwapChainSupportDetails details_result;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface, &details_result.capabilities);
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &format_count, nullptr);
        if (format_count != 0) {
            details_result.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, m_surface, &format_count, details_result.formats.data());
        }
        uint32_t presentmode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &presentmode_count, nullptr);
        if (presentmode_count != 0) {
            details_result.presentModes.resize(presentmode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &presentmode_count, details_result.presentModes.data());
        }
        return details_result;
    }

    VkFormat VulkanRHI::findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat VulkanRHI::findSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features
    ) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        LOG_ERROR("findSupportedFormat failed!");
        return VK_FORMAT_UNDEFINED;
    }

    VkSurfaceFormatKHR VulkanRHI::chooseSwapchainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats) {
        if (available_surface_formats.size() == 1
            && available_surface_formats[0].format == VK_FORMAT_UNDEFINED) {
            return { VK_FORMAT_B8G8R8A8_SRGB,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }
        for (const auto& surface_format : available_surface_formats) {
            if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB
                && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return surface_format;
            }
        }
        return available_surface_formats[0];
    }

    VkPresentModeKHR VulkanRHI::chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes) {
        for (const auto& present_mode : available_present_modes) {
            if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return present_mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRHI::chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(m_window, &width, &height);
            VkExtent2D actual_extent = { static_cast<uint32_t>(width),static_cast<uint32_t>(height) };
            actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return actual_extent;
        }
    }

    void VulkanRHI::pushEvent(RHICommandBuffer* command_buffer, const char* name, const float* color) {
        if (_enable_debug_utils_label) {
            VkDebugUtilsLabelEXT label_info{};
            label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label_info.pNext = nullptr;
            label_info.pLabelName = name;
            for (int i = 0; i < 4; ++i) {
                label_info.color[i] = color[i];
            }
            f_vkCmdBeginDebugUtilsLabelEXT(((VulkanCommandBuffer*)command_buffer)->getResource(), &label_info);
        }
    }

    void VulkanRHI::popEvent(RHICommandBuffer* command_buffer) {
        if (_enable_debug_utils_label) {
            f_vkCmdEndDebugUtilsLabelEXT(((VulkanCommandBuffer*)command_buffer)->getResource());
        }
    }

    bool VulkanRHI::isPointLightShadowEnabled() {
        return _enable_point_light_shadow;
    }

    RHICommandBuffer* VulkanRHI::getCurrentCommandBuffer() const {
        return m_current_command_buffer;
    }

    RHICommandBuffer* const* VulkanRHI::getCommandBufferList() const {
        return m_command_buffers;
    }

    RHICommandPool* VulkanRHI::getCommandPool() const {
        return m_rhi_command_pool;
    }

    RHIDescriptorPool* VulkanRHI::getDescriptorPool() const {
        return m_descriptor_pool;
    }

    RHIFence* const* VulkanRHI::getFenceList() const {
        return m_rhi_is_frame_in_flight_fences;
    }

    QueueFamilyIndices VulkanRHI::getQueueFamilyIndices() const {
        return m_queue_indices;
    }

    RHIQueue* VulkanRHI::getGraphicsQueue() const {
        return m_graphics_queue;
    }

    RHIQueue* VulkanRHI::getComputeQueue() const {
        return m_compute_queue;
    }

    RHISwapChainDesc VulkanRHI::getSwapchainInfo() {
        RHISwapChainDesc desc{};
        desc.image_format = m_swapchian_image_format;
        desc.extent = m_swapchain_extent;
        desc.viewport = &m_viewport;
        desc.scissor = &m_scissor;
        desc.imageViews = m_swapchain_imageviews;
        return desc;
    }

    RHIDepthImageDesc VulkanRHI::getDepthImageInfo() const {
        RHIDepthImageDesc desc{};
        desc.depth_image_format = m_depth_image_format;
        desc.depth_image_view = m_depth_image_view;
        desc.depth_image = m_depth_image;
        return desc;
    }

    uint8_t VulkanRHI::getMaxFramesInFlight() const {
        return k_max_frames_in_flight;
    }

    uint8_t VulkanRHI::getCurrentFrameIndex() const {
        return m_current_frame_index;
    }

    void VulkanRHI::setCurrentFrameIndex(uint8_t index) {
        m_current_frame_index = index;
    }
}