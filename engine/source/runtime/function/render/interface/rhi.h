#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <functional>

#include "rhi_class.h"

namespace Dao {

	#define RHI_DELETE_PTR(x) delete x; x = nullptr;

	class WindowSystem;

	struct RHIInitInfo
	{
		std::shared_ptr<WindowSystem> window_system;
	};

	class RHI {
	public:
		virtual ~RHI() = 0;
		virtual void initialize(RHIInitInfo initInfo) = 0;
		virtual void prepareContext() = 0;

		// ????????? Todo(this function should not be there)
		virtual void isPointLightShadowEnabled() = 0;

		// allocate 
		virtual bool allocateCommandBuffers(
			const RHICommandBufferAllocateInfo* pAllocateInfo,
			RHICommandBuffer*& pCommandBuffers
		) = 0;
		virtual bool allocateDescriptorSets(
			const RHIDescriptorSetAllocateInfo* pAllocateInfo,
			RHIDescriptorSet*& pDescriptorSets
		) = 0;

		// create
		virtual void createSwapchain() = 0;
		virtual void recreateSwapchain() = 0;
		virtual void createSwapchainImageViews() = 0;
		virtual void createFramebufferImageView() = 0;
		virtual void createCommandPool() = 0;
		virtual bool createCommandPool(
			const RHICommandPoolCreateInfo* pcreateInfo, 
			RHICommandPool*& pCommandPool
		) = 0;
		virtual bool createDescriptorPool(
			const RHIDescriptorPoolCreateInfo* pCreateInfo,
			RHIDescriptorPool*& pDescriptorPool
		) = 0;
		virtual bool createDescriptorSetLayout(
			const RHIDescriptorSetLayoutCreateInfo* pCreateInfo,
			RHIDescriptorSetLayout*& pSetLayout
		) = 0;
		virtual bool createGraphicsPipelines(
			RHIPipelineCache* pipelineCache,
			uint32_t createInfoCount,
			const RHIGraphicsPipelineCreateInfo* pCreateInfos,
			const RHIPipeline*& pPipelines
		) = 0;
		virtual bool createComputePipelines(
			RHIPipelineCache* pipelineCache,
			uint32_t createInfoCount,
			const RHIComputePipelineCreateInfo* pCreateInfos,
			RHIPipeline* pPipelines
		) = 0;
		virtual bool createPipelineLayout(
			const RHIPipelineLayoutCreateInfo* pCreateInfo,
			RHIPipelineLayout*& pPipelineLayout
		) = 0;
		virtual bool createRenderPass(
			const RHIRenderPassCreateInfo* pCreateInfo,
			RHIRenderPass*& pRenderPass
		) = 0;
		virtual bool createSampler(
			const RHISamplerCreateInfo* pCreateInfo,
			RHISampler*& pSampler
		) = 0;
		virtual bool createSemaphore(
			const RHISemaphoreCreateInfo* pCreateInfo,
			RHISemaphore*& pSemaphore
		) = 0;
		virtual bool createFence(
			const RHIFenceCreateInfo* pcreateInfo,
			RHIFence*& pFence
		) = 0;
		virtual void createBuffer(
			RHIDevice size,
			RHIBufferUsageFlags usage,
			RHIMemoryPropertyFlags properties, 
			RHIBuffer*& buffer,
			RHIDeviceMemory*& buffer_memory
		) = 0;
		virtual void createBufferAndInitialize(
			RHIBufferUsageFlags usage,
			RHIMemoryPropertyFlags properties,
			RHIBuffer*& buffer,
			RHIDeviceMemory*& buffer_memory,
			RHIDeviceSize size,
			void* data = nullptr,
			int datasize = 0
		) = 0;
		virtual void copyBuffer(
			RHIBuffer* srcBuffer,
			RHIBuffer* dstBuffer,
			RHIDevice srcOffser,
			RHIDeviceSize dstOffset,
			RHIDeviceSize size
		) = 0;
		virtual void createImage(
			uint32_t image_width,
			uint32_t image_height,
			RHIFormat format,
			RHIImageTiling image_tilling,
			RHIImageUsageFlags image_usage_flags,
			RHIMemoryPropertyFlags memory_property_flags,
			RHIImage*& image,
			RHIDeviceMemory*& memory,
			RHIImageCreateFlags image_create_flags,
			uint32_t array_layers,
			uint32_t miplevels
		) = 0;
		virtual void createImageView(
			RHIImage* image,
			RHIFormat format,
			RHIImageAspectFlags image_aspect_flags,
			RHIImageViewType view_type,
			uint32_t layout_count,
			uint32_t miplevels,
			RHIImageView*& image_view
		) = 0;
		virtual bool createFramebuffer(
			const RHIFramebufferCreateInfo* pCreateInfo,
			RHIFramebuffer*& pFramebuffer
		) = 0;
		virtual RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) = 0;
		virtual RHISampler* getOrCreateDefaultSampler(RHIDefaultSamplerType type) = 0;
		virtual RHISampler* getOrCreateMipmapSampler(uint32_t width, uint32_t height) = 0;

		// command PFN
		virtual bool waitForFencesPFN(
			uint32_t fenceCount,
			RHIFence* const* pFences,
			RHIBool32 waitAll,
			uint64_t timeout
		) = 0;
		virtual bool resetFencesPFN(
			uint32_t fenceCount,
			RHIFence* const* pFences
		) = 0;
		virtual bool resetCommandPoolPFN(
			RHICommandPool* commandPool,
			RHICommandPoolResetFlags flags
		) = 0;
		virtual bool beginCommandBufferPFN(
			RHICommandBuffer* commandBuffer,
			const RHICommandBufferBeginInfo* pBeginInfo
		) = 0;
		virtual bool endCommandBufferPFN(RHICommandBuffer* commandBuffer) = 0;
		virtual void cmdBeginRenderPassPFN(
			RHICommandBuffer* commandBuffer,
			const RHIRenderPassBeginInfo* pRenderPassBegin,
			RHISubpassContents contents
		) = 0;
		virtual void cmdNextSubpassPFN(
			RHICommandBuffer* commandBuffer,
			RHISubpassContents contents
		) = 0;
		virtual void cmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) = 0;
		virtual void cmdBindPipelinePFN(
			RHICommandBuffer* commandBuffer,
			RHIPipelineBindPoint* pipelineBindPoint,
			RHIPipeline* pipeline
		) = 0;
		virtual void cmdSetViewportPFN(
			RHICommandBuffer* commandBuffer,
			uint32_t firstViewport,
			uint32_t viewportCount,
			const RHIViewport* pViewports
		) = 0;
		virtual void cmdSetScissorPFN(
			RHICommandBuffer* commandBuffer,
			uint32_t firstScissor,
			uint32_t scissorCount,
			const RHIRect2D* pScissor
		) = 0;
		virtual void cmdBindVertexBuffersPFN(
			RHICommandBuffer* commandBuffer,
			uint32_t firstBinding,
			uint32_t bindingCount,
			RHIBuffer* const* pBuffers,
			const RHIDeviceSize* pOffsets
		) = 0;
		virtual void cmdBindIndexBufferPFN(
			RHICommandBuffer* commandBuffer,
			RHIBuffer* buffer,
			RHIDeviceSize offset,
			RHIIndexType indexType
		) = 0;
		virtual void cmdBindDescriptorSetsPFN(
			RHICommandBuffer* commandBuffer,
			RHIPipelineBindPoint pipelineBindPoint,
			RHIPipelineLayout* layout,
			uint32_t firstSet,
			uint32_t descriptorSetCount,
			const RHIDescriptorSet* const* pDescriptorSets,
			uint32_t dynamicOffsetCount,
			const uint32_t* pDynamicOffsets
		) = 0;
		virtual void cmdDrawIndexedPFN(
			RHICommandBuffer* commandBuffer,
			uint32_t indexCount,
			uint32_t instanceCount,
			uint32_t firstIndex,
			int32_t vertexOffset,
			uint32_t firstInstance
		) = 0;
		virtual void cmdClearAttachmentsPFN(
			RHICommandBuffer* commandBuffer,
			uint32_t attachmentCount,
			const RHIClearAttachment* pAttachments,
			uint32_t rectCount,
			const RHIClearRect* pRects
		) = 0;

		// command
		virtual bool beginCommandBuffer(
			RHICommandBuffer* commandBuffer,
			const RHICommandBufferBeginInfo* pBeginInfo
		) = 0;
		virtual void cmdCopyImageToBuffer(
			RHICommandBuffer* commandBuffer,
			RHIImage* srcImage,
			RHIImageLayout* srtImageLayout,
			RHIBuffer* dstBuffer,
			uint32_t regionCount,
			const RHIBufferImageCopy* pRegions
		) = 0;
		virtual void cmdCopyImageToImage(
			RHICommandBuffer* commandBuffer,
			RHIImage* srcImage,
			RHIImageAspectFlagBits srcFlag,
			RHIImage* destImage,
			RHIImageAspectFlagBits dstFlags,
			uint32_t width,
			uint32_t height
		) = 0;
		virtual void cmdCopyBuffer(
			RHICommandBuffer* commandBuffer,
			RHIBuffer* srcBuffer,
			RHIBuffer* dstBuffer,
			uint32_t regionCount,
			RHIBufferCopy* pRegions
		) = 0;
		virtual void cmdDraw(
			RHICommandBuffer* commandBuffer,
			uint32_t vertexCount,
			uint32_t instanceCount,
			uint32_t firstVertex,
			uint32_t firstInstance
		) = 0;
		virtual void cmdDispatch(
			RHICommandBuffer* commandBuffer,
			uint32_t groupCountX,
			uint32_t gourpCountY,
			uint32_t groupCountZ
		) = 0;
		virtual void cmdDispatchIndirect(
			RHICommandBuffer* commandBuffer,
			RHIBuffer* buffer,
			RHIDevice offset
		) = 0;
		virtual void cmdPipelineBarrier(
			RHICommandBuffer* commandBuffer,
			RHIPipelineStageFlags srcStageMask,
			RHIPipelineStageFlags dstStageMask,
			RHIDependencyFlags dependencyFlags,
			uint32_t memoryBarrierCount,
			const RHIMemoryBarrier* pMemoryBarriers,
			uint32_t bufferMemoryBarrierCount,
			const RHIBufferMemoryBarrier* pBufferMemoryBarriers,
			uint32_t imageMemoryBarrierCount,
			const RHIImageMemoryBarrier* pImageMemoryBarriers
		) = 0;
		virtual bool endCommandBuffer(RHICommandBuffer* commandBuffer) = 0;
		virtual void updateDescriptorSets(
			uint32_t descriptorWriteCount,
			const RHIWriteDescriptorSet* pDescriptorWrites,
			uint32_t descriptorCopyCount,
			const RHICopyDescriptorSet* pDescriptorCopies
		) = 0;
		virtual bool queueSubmit(
			RHIQueue* queue,
			uint32_t submitCount,
			const RHISubmitInfo* pSubmits,
			RHIFence* fence
		) = 0;
		virtual bool queueWaitIdle(RHIQueue* queue) = 0;
		virtual void resetCommandPool() = 0;
		virtual void waitForFences() = 0;
		
		// query
		virtual void getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties) = 0;
		virtual RHICommandBuffer* getCurrentCommandBuffer() const = 0;
		virtual RHICommandBuffer* const* getCommandBufferList() const = 0;
		virtual RHICommandPool* getCommandPool() const = 0;
		virtual RHIDescriptorPool* getDescriptorPool() const = 0;
		virtual RHIFence* const* getFenceList() const = 0;
		virtual QueueFamilyIndices getQueueFamilyIndices() const = 0;
		virtual RHIQueue* getGraphicsQueue() const = 0;
		virtual RHIQueue* getComputeQueue() const = 0;
		virtual RHISwapChainDesc* getSwapChainInfo() = 0;
		virtual RHIDepthImageDesc* getDepthImageInfo() const = 0;
		virtual uint8_t getMaxFramesInFlight() const = 0;
		virtual uint8_t getCurrentFrameIndex() const = 0;
		virtual void setCurrentFrameIndex(uint8_t index) = 0;

		// command write
		virtual RHICommandBuffer* beginSingleTimeCommands() = 0;
		virtual void endSingleTimeCommands(RHICommandBuffer* command_buffer) = 0;
		virtual bool prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapChain) = 0;
		virtual bool submitRendering(std::function<void()> passUpdateAfterRecreateSwapChain) = 0;
		virtual void pushEvent(RHICommandBuffer* command_buffer, const char* name, const float* color) = 0;
		virtual void popEvent(RHICommandBuffer* commond_buffer) = 0;

		// destroy
		virtual void clear() = 0;
		virtual void clearSwapChain() = 0;
		virtual void destroyDefaultSampler(RHIDefaultSamplerType type) = 0;
		virtual void destroyMipmappedSampler() = 0;
		virtual void destroyShaderModule(RHIShader* shader) = 0;
		virtual void destroySemaphore(RHISemaphore* semaphore) = 0;
		virtual void destroySampler(RHISampler* sampler) = 0;
		virtual void destroyInstance(RHIInstance* instance) = 0;
		virtual void destroyImageView(RHIImageView* imageView) = 0;
		virtual void destroyImage(RHIImage* image) = 0;
		virtual void destroyFrameBuffer(RHIFramebuffer* frameBuffer) = 0;
		virtual void destroyFence(RHIFence* fance) = 0;
		virtual void destroyDevice() = 0;
		virtual void destroyCommandPool(RHICommandPool* commandPool) = 0;
		virtual void destroyBuffer(RHIBuffer*& buffer) = 0;
		virtual void freeCommandBuffers(
			RHICommandPool* commandPool,
			uint32_t commandBufferCount,
			RHICommandBuffer* pCommandBuffers
		) = 0;

		// memory
		virtual void freeMemory(RHIDevice*& memory) = 0;
		virtual void mapMemory(
			RHIDeviceMemory* memory,
			RHIDeviceSize offset,
			RHIDeviceSize size,
			RHIMemoryMapFlags flags,
			void** ppData
		) = 0;
		virtual void unmapMemory(RHIDeviceMemory* memory) = 0;
		virtual void invalidateMappedMemoryRanges(
			void* pNext, 
			RHIDeviceMemory* memory, 
			RHIDeviceSize offset, 
			RHIDeviceSize size
		) = 0;
		virtual void flushMappedMemoryRanges(
			void* pNext,
			RHIDeviceMemory* memory,
			RHIDeviceSize offset,
			RHIDeviceSize size
		) = 0;

		// semaphores
		virtual RHISemaphore*& getTextureCopySemaphore(uint32_t index) = 0;
	};

	inline RHI::~RHI() = default;
}