#pragma once

#include "runtime/function/render/interface/rhi.h"

#include <vulkan/vulkan.h>
#include <optional>

namespace Dao {
	class VulkanBuffer :public RHIBuffer {
	public:
		void setResource(VkBuffer res) {
			_resource = res;
		}
		VkBuffer getResource() const {
			return _resource;
		}
	private:
		VkBuffer _resource;
	};
	class VulkanBufferView :public RHIBufferView {
	public:
		void setResource(VkBufferView res) {
			_resource = res;
		}
		VkBufferView getResource() const {
			return _resource;
		}
	private:
		VkBufferView _resource;
	};
	class VulkanCommandBuffer :public RHICommandBuffer {
	public:
		void setResource(VkCommandBuffer res) {
			_resource = res;
		}
		VkCommandBuffer getResource() const {
			return _resource;
		}
	private:
		VkCommandBuffer _resource;
	};
	class VulkanCommandPool :public RHICommandPool {
	public:
		void setResource(VkCommandPool res) {
			_resource = res;
		}
		VkCommandPool getResource() const {
			return _resource;
		}
	private:
		VkCommandPool _resource;
	};
	class VulkanDescriptorPool :public RHIDescriptorPool {
	public:
		void setResource(VkDescriptorPool res) {
			_resource = res;
		}
		VkDescriptorPool getResource() const {
			return _resource;
		}
	private:
		VkDescriptorPool _resource;
	};
	class VulkanDescriptorSet :public RHIDescriptorSet {
	public:
		void setResource(VkDescriptorSet res) {
			_resource = res;
		}
		VkDescriptorSet getResource() const {
			return _resource;
		}
	private:
		VkDescriptorSet _resource;
	};
	class VulkanDescriptorSetLayout :public RHIDescriptorSetLayout {
	public:
		void setResource(VkDescriptorSetLayout res) {
			_resource = res;
		}
		VkDescriptorSetLayout getResource() const {
			return _resource;
		}
	private:
		VkDescriptorSetLayout _resource;
	};
	class VulkanDevice :public RHIDevice {
	public:
		void setResource(VkDevice res) {
			_resource = res;
		}
		VkDevice getResource() const {
			return _resource;
		}
	private:
		VkDevice _resource;
	};
	class VulkanDeviceMemory :public RHIDeviceMemory {
	public:
		void setResource(VkDeviceMemory res) {
			_resource = res;
		}
		VkDeviceMemory getResource() const {
			return _resource;
		}
	private:
		VkDeviceMemory _resource;
	};
	class VulkanEvent :public RHIEvent {
	public:
		void setResource(VkEvent res) {
			_resource = res;
		}
		VkEvent getResource() const {
			return _resource;
		}
	private:
		VkEvent _resource;
	};
	class VulkanFence :public RHIFence {
	public:
		void setResource(VkFence res) {
			_resource = res;
		}
		VkFence getResource() const {
			return _resource;
		}
	private:
		VkFence _resource;
	};
	class VulkanFramebuffer :public RHIFramebuffer {
	public:
		void setResource(VkFramebuffer res) {
			_resource = res;
		}
		VkFramebuffer getResource() const {
			return _resource;
		}
	private:
		VkFramebuffer _resource;
	};
	class VulkanImage :public RHIImage {
	public:
		void setResource(VkImage res) {
			_resource = res;
		}
		VkImage& getResource() {
			return _resource;
		}
	private:
		VkImage _resource;
	};
	class VulkanImageView :public RHIImageView {
	public:
		void setResource(VkImageView res) {
			_resource = res;
		}
		VkImageView getResource() const {
			return _resource;
		}
	private:
		VkImageView _resource;
	};
	class VulkanInstance :public RHIInstance {
	public:
		void setResource(VkInstance res) {
			_resource = res;
		}
		VkInstance getResource() const {
			return _resource;
		}
	private:
		VkInstance _resource;
	};
	class VulkanQueue :public RHIQueue {
	public:
		void setResource(VkQueue res) {
			_resource = res;
		}
		VkQueue getResource() const {
			return _resource;
		}
	private:
		VkQueue _resource;
	};
	class VulkanPhysicalDevice :public RHIPhysicalDevice {
	public:
		void setResource(VkPhysicalDevice res) {
			_resource = res;
		}
		VkPhysicalDevice getResource() const {
			return _resource;
		}
	private:
		VkPhysicalDevice _resource;
	};
	class VulkanPipeline :public RHIPipeline {
	public:
		void setResource(VkPipeline res) {
			_resource = res;
		}
		VkPipeline getResource() const {
			return _resource;
		}
	private:
		VkPipeline _resource;
	};
	class VulkanPipelineCache :public RHIPipelineCache {
	public:
		void setResource(VkPipelineCache res) {
			_resource = res;
		}
		VkPipelineCache getResource() const {
			return _resource;
		}
	private:
		VkPipelineCache _resource;
	};
	class VulkanPipelineLayout :public RHIPipelineLayout {
	public:
		void setResource(VkPipelineLayout res) {
			_resource = res;
		}
		VkPipelineLayout getResource() const {
			return _resource;
		}
	private:
		VkPipelineLayout _resource;
	};
	class VulkanRenderPass :public RHIRenderPass {
	public:
		void setResource(VkRenderPass res) {
			_resource = res;
		}
		VkRenderPass getResource() const {
			return _resource;
		}
	private:
		VkRenderPass _resource;
	};
	class VulkanSampler :public RHISampler {
	public:
		void setResource(VkSampler res) {
			_resource = res;
		}
		VkSampler getResource() const {
			return _resource;
		}
	private:
		VkSampler _resource;
	};
	class VulkanSemaphore :public RHISemaphore {
	public:
		void setResource(VkSemaphore res) {
			_resource = res;
		}
		VkSemaphore& getResource() {
			return _resource;
		}
	private:
		VkSemaphore _resource;
	};
	class VulkanShader :public RHIShader {
	public:
		void setResource(VkShaderModule res) {
			_resource = res;
		}
		VkShaderModule getResource() const {
			return _resource;
		}
	private:
		VkShaderModule _resource;
	};
}