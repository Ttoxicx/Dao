#include "runtime/function/render/passes/ui_pass.h"

#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/function/ui/window_ui.h"
#include "runtime/core/base/macro.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace Dao {

	void UIPass::initialize(const RenderPassInitInfo* init_info) {
		RenderPass::initialize(nullptr);
		m_framebuffer.render_pass = static_cast<const UIPassInitInfo*>(init_info)->render_pass;
	}

	void UIPass::initializeUIRenderBackend(WindowUI* window_ui) {
		_window_ui = window_ui;
		ImGui_ImplGlfw_InitForVulkan(std::static_pointer_cast<VulkanRHI>(m_rhi)->m_window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = std::static_pointer_cast<VulkanRHI>(m_rhi)->m_instance;
		init_info.PhysicalDevice = std::static_pointer_cast<VulkanRHI>(m_rhi)->m_physical_device;
		init_info.Device = std::static_pointer_cast<VulkanRHI>(m_rhi)->m_device;
		init_info.QueueFamily = m_rhi->getQueueFamilyIndices().graphics_family.value();
		init_info.Queue = ((VulkanQueue*)m_rhi->getGraphicsQueue())->getResource();
		init_info.DescriptorPool = std::static_pointer_cast<VulkanRHI>(m_rhi)->m_vk_descriptor_pool;
		init_info.Subpass = main_camera_subpass_ui;
		//may be different from the real swapchain image count
		//see ImGui_ImplVulkanH_GetMinImageCountFromPresentMode
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		ImGui_ImplVulkan_Init(&init_info, ((VulkanRenderPass*)m_framebuffer.render_pass)->getResource());

		createFonts();
	}

	void UIPass::createFonts() {
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	void UIPass::destroyFonts() {
		ImGui_ImplVulkan_DestroyFontsTexture();
	}

	void UIPass::draw() {
		if (_window_ui) {
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			_window_ui->preRender();
			float color[4] = { 1.0f,1.0f,1.0f,1.0f };
			m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "ImGUI", color);
			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), std::static_pointer_cast<VulkanRHI>(m_rhi)->m_vk_current_command_buffer);
			m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
		}
	}
}