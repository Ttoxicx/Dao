#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_rhi_res.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <vector>

namespace Dao {
	class VulkanRHI final: public RHI {
	public:

		~VulkanRHI() override final;

		void initialize(RHIInitInfo init_info) override;
		void prepareContext() override;

		// allocate 
		bool allocateCommandBuffers(
			const RHICommandBufferAllocateInfo* allocate_info,
			RHICommandBuffer*& command_buffers
		) override;
		bool allocateDescriptorSets(
			const RHIDescriptorSetAllocateInfo* allocate_info,
			RHIDescriptorSet*& descriptor_sets
		) override;

		// create
		void createSwapchain() override;
		void recreateSwapchain() override;
		void createSwapchainImageViews() override;
		void createFramebufferImageView() override;
		bool createCommandPool(
			const RHICommandPoolCreateInfo* pcreate_info,
			RHICommandPool*& command_pool
		) override;
		bool createDescriptorPool(
			const RHIDescriptorPoolCreateInfo* create_info,
			RHIDescriptorPool*& descriptor_pool
		) override;
		bool createDescriptorSetLayout(
			const RHIDescriptorSetLayoutCreateInfo* create_info,
			RHIDescriptorSetLayout*& set_layout
		) override;
		bool createGraphicsPipelines(
			RHIPipelineCache* pipeline_cache,
			uint32_t create_info_count,
			const RHIGraphicsPipelineCreateInfo* create_infos,
			const RHIPipeline*& pipelines
		) override;
		bool createComputePipelines(
			RHIPipelineCache* pipeline_cache,
			uint32_t create_info_count,
			const RHIComputePipelineCreateInfo* create_infos,
			RHIPipeline* pipelines
		) override;
		bool createPipelineLayout(
			const RHIPipelineLayoutCreateInfo* create_info,
			RHIPipelineLayout*& pipeline_layout
		) override;
		bool createRenderPass(
			const RHIRenderPassCreateInfo* create_info,
			RHIRenderPass*& render_pass
		) override;
		bool createSampler(
			const RHISamplerCreateInfo* create_info,
			RHISampler*& sampler
		) override;
		bool createSemaphore(
			const RHISemaphoreCreateInfo* create_info,
			RHISemaphore*& semaphore
		) override;
		bool createFence(
			const RHIFenceCreateInfo* create_info,
			RHIFence*& fence
		) override;
		void createBuffer(
			RHIDevice size,
			RHIBufferUsageFlags usage,
			RHIMemoryPropertyFlags properties,
			RHIBuffer*& buffer,
			RHIDeviceMemory*& buffer_memory
		) override;
		void createBufferAndInitialize(
			RHIBufferUsageFlags usage,
			RHIMemoryPropertyFlags properties,
			RHIBuffer*& buffer,
			RHIDeviceMemory*& buffer_memory,
			RHIDeviceSize size,
			void* data = nullptr,
			int datasize = 0
		) override;
		bool createBufferVMA(
			VmaAllocator allocator,
			const RHIBufferCreateInfo* buffer_create_info,
			const VmaAllocationCreateInfo* allocation_create_info,
			RHIBuffer*& buffer,
			VmaAllocation* allocation,
			VmaAllocationInfo* allocation_info
		) override;
		bool createBufferWithAligmentVMA(
			VmaAllocator allocator,
			const RHIBufferCreateInfo* buffer_create_info,
			const VmaAllocationCreateInfo* allocation_create_info,
			RHIDeviceSize min_aligment,
			RHIBuffer*& buffer,
			VmaAllocation* allocation,
			VmaAllocationInfo* allocation_info
		) override;
		void copyBuffer(
			RHIBuffer* src_buffer,
			RHIBuffer* dst_buffer,
			RHIDevice src_offser,
			RHIDeviceSize dst_offset,
			RHIDeviceSize size
		) override;
		void createImage(
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
		) override;
		void createImageView(
			RHIImage* image,
			RHIFormat format,
			RHIImageAspectFlags image_aspect_flags,
			RHIImageViewType view_type,
			uint32_t layout_count,
			uint32_t miplevels,
			RHIImageView*& image_view
		) override;
		void createGlobalImage(
			RHIImage*& image,
			RHIImageView*& image_view,
			VmaAllocation& image_allocation,
			uint32_t texture_image_width,
			uint32_t texture_image_height,
			void* texture_image_pixels,
			RHIFormat texture_image_format,
			uint32_t miplevels = 0
		) override;
		void createCubeMap(
			RHIImage*& image,
			RHIImageView*& image_view,
			VmaAllocation& image_allocation,
			uint32_t texture_image_width,
			uint32_t texture_image_height,
			std::array<void*, 6> texture_image_pixels,
			RHIFormat texture_image_format,
			uint32_t miplevels
		) override;
		bool createFramebuffer(
			const RHIFramebufferCreateInfo* create_info,
			RHIFramebuffer*& frame_buffer
		) override;
		RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) override;
		RHISampler* getOrCreateDefaultSampler(RHIDefaultSamplerType type) override;
		RHISampler* getOrCreateMipmapSampler(uint32_t width, uint32_t height) override;

		// command PFN
		bool waitForFencesPFN(
			uint32_t fence_count,
			RHIFence* const* fences,
			RHIBool32 wait_all,
			uint64_t timeout
		) override;
		bool resetFencesPFN(
			uint32_t fence_count,
			RHIFence* const* fences
		) override;
		bool resetCommandPoolPFN(
			RHICommandPool* command_pool,
			RHICommandPoolResetFlags flags
		) override;
		bool beginCommandBufferPFN(
			RHICommandBuffer* command_buffer,
			const RHICommandBufferBeginInfo* begin_info
		) override;
		bool endCommandBufferPFN(RHICommandBuffer* command_buffer) override;
		void cmdBeginRenderPassPFN(
			RHICommandBuffer* command_buffer,
			const RHIRenderPassBeginInfo* render_pass_begin,
			RHISubpassContents contents
		) override;
		void cmdNextSubpassPFN(
			RHICommandBuffer* command_buffer,
			RHISubpassContents contents
		) override;
		void cmdEndRenderPassPFN(RHICommandBuffer* command_buffer) override;
		void cmdBindPipelinePFN(
			RHICommandBuffer* command_buffer,
			RHIPipelineBindPoint* pipeline_bind_point,
			RHIPipeline* pipeline
		) override;
		void cmdSetViewportPFN(
			RHICommandBuffer* command_buffer,
			uint32_t first_viewport,
			uint32_t viewport_count,
			const RHIViewport* viewports
		) override;
		void cmdSetScissorPFN(
			RHICommandBuffer* command_buffer,
			uint32_t first_scissor,
			uint32_t scissor_count,
			const RHIRect2D* scissor
		) override;
		void cmdBindVertexBuffersPFN(
			RHICommandBuffer* command_buffer,
			uint32_t first_binding,
			uint32_t binding_count,
			RHIBuffer* const* buffers,
			const RHIDeviceSize* offsets
		) override;
		void cmdBindIndexBufferPFN(
			RHICommandBuffer* command_buffer,
			RHIBuffer* buffer,
			RHIDeviceSize offset,
			RHIIndexType index_type
		) override;
		void cmdBindDescriptorSetsPFN(
			RHICommandBuffer* command_buffer,
			RHIPipelineBindPoint pipeline_bind_point,
			RHIPipelineLayout* layout,
			uint32_t first_set,
			uint32_t descriptor_set_count,
			const RHIDescriptorSet* const* descriptor_sets,
			uint32_t dynamic_offset_count,
			const uint32_t* dynamic_offsets
		) override;
		void cmdDrawIndexedPFN(
			RHICommandBuffer* command_buffer,
			uint32_t index_count,
			uint32_t instance_count,
			uint32_t first_index,
			int32_t vertex_offset,
			uint32_t first_instance
		) override;
		void cmdClearAttachmentsPFN(
			RHICommandBuffer* command_buffer,
			uint32_t attachment_count,
			const RHIClearAttachment* attachments,
			uint32_t rect_count,
			const RHIClearRect* rects
		) override;

		// command
		bool beginCommandBuffer(
			RHICommandBuffer* command_buffer,
			const RHICommandBufferBeginInfo* begin_info
		) override;
		void cmdCopyImageToBuffer(
			RHICommandBuffer* command_buffer,
			RHIImage* src_image,
			RHIImageLayout* src_image_layout,
			RHIBuffer* dst_buffer,
			uint32_t region_count,
			const RHIBufferImageCopy* regions
		) override;
		void cmdCopyImageToImage(
			RHICommandBuffer* command_buffer,
			RHIImage* src_image,
			RHIImageAspectFlagBits src_flags,
			RHIImage* dest_image,
			RHIImageAspectFlagBits dst_flags,
			uint32_t width,
			uint32_t height
		) override;
		void cmdCopyBuffer(
			RHICommandBuffer* command_buffer,
			RHIBuffer* src_buffer,
			RHIBuffer* dst_buffer,
			uint32_t region_count,
			RHIBufferCopy* regions
		) override;
		void cmdDraw(
			RHICommandBuffer* command_buffer,
			uint32_t vertex_count,
			uint32_t instance_count,
			uint32_t first_vertex,
			uint32_t first_instance
		) override;
		void cmdDispatch(
			RHICommandBuffer* command_buffer,
			uint32_t group_count_x,
			uint32_t gourp_count_y,
			uint32_t group_count_z
		) override;
		void cmdDispatchIndirect(
			RHICommandBuffer* command_buffer,
			RHIBuffer* buffer,
			RHIDevice offset
		) override;
		void cmdPipelineBarrier(
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
		) override;
		bool endCommandBuffer(RHICommandBuffer* command_buffer) = 0;
		void updateDescriptorSets(
			uint32_t descriptor_write_count,
			const RHIWriteDescriptorSet* descriptor_writes,
			uint32_t descriptor_copy_count,
			const RHICopyDescriptorSet* descriptor_copies
		) override;
		bool queueSubmit(
			RHIQueue* queue,
			uint32_t submit_count,
			const RHISubmitInfo* submits,
			RHIFence* fence
		) override;
		bool queueWaitIdle(RHIQueue* queue) override;
		void resetCommandPool() override;
		void waitForFences() override;
		bool waitForFences(
			uint32_t fence_count, 
			const RHIFence* const* fences, 
			RHIBool32 wait_all, 
			uint64_t timeout
		);

		// query
		void getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* properties) override;
		RHICommandBuffer* getCurrentCommandBuffer() const override;
		RHICommandBuffer* const* getCommandBufferList() const override;
		RHICommandPool* getCommandPool() const override;
		RHIDescriptorPool* getDescriptorPool() const override;
		RHIFence* const* getFenceList() const override;
		QueueFamilyIndices getQueueFamilyIndices() const override;
		RHIQueue* getGraphicsQueue() const override;
		RHIQueue* getComputeQueue() const override;
		RHISwapChainDesc* getSwapChainInfo() override;
		RHIDepthImageDesc* getDepthImageInfo() const override;
		uint8_t getMaxFramesInFlight() const override;
		uint8_t getCurrentFrameIndex() const override;
		void setCurrentFrameIndex(uint8_t index) override;

		// command write
		RHICommandBuffer* beginSingleTimeCommands() override;
		void endSingleTimeCommands(RHICommandBuffer* command_buffer) override;
		bool prepareBeforePass(std::function<void()> pass_update_after_recreate_swapchain) override;
		void submitRendering(std::function<void()> pass_update_after_recreate_swapchain) override;
		void pushEvent(RHICommandBuffer* command_buffer, const char* name, const float* color) override;
		void popEvent(RHICommandBuffer* commond_buffer) override;

		// destroy
		void clear() override;
		void clearSwapChain() override;
		void destroyDefaultSampler(RHIDefaultSamplerType type) override;
		void destroyMipmappedSampler() override;
		void destroyShaderModule(RHIShader* shader) override;
		void destroySemaphore(RHISemaphore* semaphore) override;
		void destroySampler(RHISampler* sampler) override;
		void destroyInstance(RHIInstance* instance) override;
		void destroyImageView(RHIImageView* image_view) override;
		void destroyImage(RHIImage* image) override;
		void destroyFrameBuffer(RHIFramebuffer* frame_buffer) override;
		void destroyFence(RHIFence* fance) override;
		void destroyDevice() override;
		void destroyCommandPool(RHICommandPool* command_pool) override;
		void destroyBuffer(RHIBuffer*& buffer) override;
		void freeCommandBuffers(
			RHICommandPool* command_pool,
			uint32_t command_buffer_count,
			RHICommandBuffer* command_buffers
		) override;

		// memory
		void freeMemory(RHIDevice*& memory) override;
		void mapMemory(
			RHIDeviceMemory* memory,
			RHIDeviceSize offset,
			RHIDeviceSize size,
			RHIMemoryMapFlags flags,
			void** data
		) override;
		void unmapMemory(RHIDeviceMemory* memory) override;
		void invalidateMappedMemoryRanges(
			void* next,
			RHIDeviceMemory* memory,
			RHIDeviceSize offset,
			RHIDeviceSize size
		) override;
		void flushMappedMemoryRanges(
			void* next,
			RHIDeviceMemory* memory,
			RHIDeviceSize offset,
			RHIDeviceSize size
		) override;

		// semaphores
		RHISemaphore*& getTextureCopySemaphore(uint32_t index) override;

	public:
		void isPointLightShadowEnabled() override;

	public:
		static uint8_t	const				k_max_frames_in_flight{ 3 };

		RHIQueue*							m_graphics_queue{ nullptr };
		RHIQueue*							m_compute_queue{ nullptr };

		RHIFormat							m_swapchian_image_format{ RHI_FORMAT_UNDEFINED };
		std::vector<RHIImageView*>			m_swapchain_imageviews;
		RHIExtent2D							m_swapchain_extent;
		RHIViewport							m_viewport;
		RHIRect2D							m_scissor;

		RHIFormat							m_depth_image_format{ RHI_FORMAT_UNDEFINED };
		RHIImageView*						m_depth_image_view = new VulkanImageView();

		RHIFence*							m_rhi_is_frame_in_flight_fences[k_max_frames_in_flight];
		
		RHIDescriptorPool*					m_descriptor_pool = new VulkanDescriptorPool();
		RHICommandPool*						m_rhi_command_pool;

		RHICommandBuffer*					m_command_buffers[k_max_frames_in_flight];
		RHICommandBuffer*					m_current_command_buffer = new VulkanCommandBuffer();

		QueueFamilyIndices					m_queue_indices;

		GLFWwindow*							m_window{ nullptr };
		VkInstance							m_instance{ nullptr };
		VkSurfaceKHR						m_surface{ nullptr };
		VkPhysicalDevice					m_physical_device{ nullptr };
		VkDevice							m_device{ nullptr };
		VkQueue								m_present_queue{ nullptr };

		VkSwapchainKHR						m_swapchain{ nullptr };
		std::vector<VkImage>				m_swapchain_images;

		RHIImage*							m_depth_image = new VulkanImage();
		VkDeviceMemory						m_depth_image_memory{ nullptr };

		std::vector<VkFramebuffer>			m_swapchian_framebuffers;

		// global descriptor pool
		VkDescriptorPool					m_vk_descriptor_pool;

		// command pool and buffers
		uint8_t								m_current_frame_index{ 0 };
		VkCommandPool						m_command_pools[k_max_frames_in_flight];
		VkCommandBuffer						m_vk_command_buffers[k_max_frames_in_flight];
		VkSemaphore							m_image_available_for_render_semaphores[k_max_frames_in_flight];
		VkSemaphore							m_image_finished_for_presentation_samaphores[k_max_frames_in_flight];
		RHISemaphore*						m_image_available_for_texturescopy_semaphores[k_max_frames_in_flight];
		VkFence								m_is_frame_in_flight_fences[k_max_frames_in_flight];

		VkCommandBuffer						m_vk_current_command_buffer;
		uint32_t							m_current_swapchain_image_index;

		// asset allocator
		VmaAllocator						m_assets_allocator;

		// function pointers
		PFN_vkCmdBeginDebugUtilsLabelEXT	f_vkCmdBeginDebugUtilsLabelEXT;
		PFN_vkCmdEndDebugUtilsLabelEXT		f_vkCmdEndDebugUtilsLabelEXT;
		PFN_vkWaitForFences					f_vkWaitForFences;
		PFN_vkResetFences					f_vkResetFences;
		PFN_vkResetCommandPool				f_vkResetCommandPool;
		PFN_vkBeginCommandBuffer			f_vkBeginCommandBuffer;
		PFN_vkEndCommandBuffer				f_vkEndCommandBuffer;
		PFN_vkCmdBeginRenderPass			f_vkCmdBeginRenderPass;
		PFN_vkCmdNextSubpass				f_vkCmdNextSubpass;
		PFN_vkCmdEndRenderPass				f_vkCmdEndRenderPass;
		PFN_vkCmdBindPipeline				f_vkCmdBindPipeline;
		PFN_vkCmdSetViewport				f_vkCmdSetViewport;
		PFN_vkCmdSetScissor					f_vkCmdSetScissor;
		PFN_vkCmdBindVertexBuffers			f_vkCmdBindVertexBuffers;
		PFN_vkCmdBindIndexBuffer			f_vkCmdBindIndexBuffer;
		PFN_vkCmdBindDescriptorSets			f_vkCmdBindDescriptorSets;
		PFN_vkCmdDrawIndexed				f_vkCmdDrawIndexed;
		PFN_vkCmdClearAttachments			f_vkCmdClearAttachments;
		
	private:
		const std::vector<char const*>		_validation_layers{ "VK_LAYER_KHRONOS_validation" };
		uint32_t							_vulkan_api_version{ VK_API_VERSION_1_0 };
		std::vector<char const*>			_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		
		// default sampler cache
		RHISampler*							_linear_sampler = nullptr;
		RHISampler*							_nearest_sampler = nullptr;
		std::map<uint32_t, RHISampler*>		_mipmap_sampler_map;

	private:
		bool								_enable_validation_layers{ true };
		bool								_enable_debug_utils_label{ true };
		bool								_enable_point_light_shadow{ true };

		// used in descriptor pool creation
		uint32_t							_max_vertex_blending_mesh_count{ 256 };
		uint32_t							_max_material_count{ 256 };

		VkDebugUtilsMessengerEXT			_debug_messenger = nullptr;

	private:
		void createInstance();
		void initializeDebugMessenger();
		void createWindowSurface();
		void initializePhysicalDevice();
		void createLogicalDevice();
		void createCommandPool() override;
		void createCommandBuffers();
		void createDescriptorPool();
		void createSyncPrimitives();
		void createAssetAllocator();

	private:
		bool checkValidationLayerSupport();
		std::vector<const char*> getRequredExtensions();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
		VkResult createDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* create_info,
			const VkAllocationCallbacks* allocator,
			VkDebugUtilsMessengerEXT* debug_messenger
		);
		void destroyDebugUtilsMessengerEXT(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debug_messenger,
			const VkAllocationCallbacks* allocator
		);
		void findQueueFamilies(VkPhysicalDevice physical_device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice physical_device);
		bool isDeviceSuitable(VkPhysicalDevice physical_device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physical_device);
		VkFormat findDepthFormat();
		VkFormat findSupportedFormat(
			const std::vector<VkFormat>& candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features
		);
		VkSurfaceFormatKHR chooseSwapchainSurfaceFormatDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats);
		VkPresentModeKHR chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_presents_modes);
		VkExtent2D chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}