/* Implementation of swapchains.
 *   Copyright (C) 2023-2024  Chloe Eather.
 *
 *   This file is part of CeeEngine.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#include <CeeEngine/renderer/swapchain.h>
#include <CeeEngine/renderer/config.h>

#include <CeeEngine/window.h>

namespace cee {
	void Swapchain::Init(VkInstance instance, const std::shared_ptr<Device>& device)
	{
		m_Instance = instance;
		m_Device = device;
	}

	void Swapchain::InitSurface(NativeWindowConnection* connection, NativeWindowHandle handle)
	{
		VkResult result = VK_SUCCESS;
		VkPhysicalDevice vkPhysicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();

#if defined(CEE_PLATFORM_WINDOWS)
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext = NULL,
			.flags = 0,
			.hinstance = connection,
			.hwnd = handle
		};
		vkCreateWin32SurfaceKHR(m_Instance, &surfaceCreateInfo, NULL, &m_Surface);
#elif defined(CEE_PLATFORM_LINUX)
		VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
			.pNext = NULL,
			.flags = 0,
			.connection = connection,
			.window = handle
		};
		vkCreateXcbSurfaceKHR(m_Instance, &surfaceCreateInfo, NULL, &m_Surface);
#endif
		uint32_t queueFamilyPropertiesCount = 0;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;

		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertiesCount, NULL);
		CEE_ASSERT(queueFamilyPropertiesCount > 0);
		queueFamilyProperties.resize(queueFamilyPropertiesCount);
		queueFamilyProperties.resize(queueFamilyPropertiesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties.data());

		std::vector<VkBool32> supportsPresent(queueFamilyPropertiesCount);
		for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
			vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, i, m_Surface, &supportsPresent[i]);
		}

		uint32_t graphicsQueueIndex = UINT32_MAX, presentQueueIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
			if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
				continue;
			}
			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueIndex = i;
				presentQueueIndex = i;
				break;
			}
			if (graphicsQueueIndex == UINT32_MAX) {
				graphicsQueueIndex = i;
			}
		}

		CEE_ASSERT(graphicsQueueIndex != UINT32_MAX, "Failed to find graphics queue.");
		CEE_ASSERT(presentQueueIndex != UINT32_MAX, "Failed to find present queue.");

		m_PresentQueueIndex = presentQueueIndex;

		uint32_t surfaceFormatCount;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_Surface, &surfaceFormatCount, NULL);
		CEE_ASSERT(surfaceFormatCount > 0);
		surfaceFormats.resize(surfaceFormatCount);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_Surface, &surfaceFormatCount, surfaceFormats.data());
		CEE_ASSERT(result == VK_SUCCESS);

		if ((surfaceFormatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
			m_Format = VK_FORMAT_B8G8R8A8_UNORM;
			m_ColorSpace = surfaceFormats[0].colorSpace;
		} else {
			bool b8g8r8a8_unormfound = false;
			for (auto&& surfaceFormat : surfaceFormats) {
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
					m_Format = surfaceFormat.format;
					m_ColorSpace = surfaceFormat.colorSpace;
					b8g8r8a8_unormfound = true;
					break;
				}
			}
			if (!b8g8r8a8_unormfound) {
				m_Format = surfaceFormats[0].format;
				m_ColorSpace = surfaceFormats[0].colorSpace;
			}
		}
	}

	void Swapchain::Create(uint32_t* width, uint32_t* height, bool VSync)
	{
		VkResult result = VK_SUCCESS;
		m_VSync = VSync;

		VkDevice vkDevice = m_Device->GetDevice();
		VkPhysicalDevice vkPhysicalDevice = m_Device->GetPhysicalDevice()->GetVulkanPhysicalDevice();

		VkSwapchainKHR oldSwapchain = m_Swapchain;

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, m_Surface, &surfaceCapabilities);
		CEE_ASSERT(result == VK_SUCCESS);

		uint32_t presentModeCount;
		std::vector<VkPresentModeKHR> supportedPresentModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_Surface, &presentModeCount, NULL);
		CEE_ASSERT(presentModeCount > 0);
		supportedPresentModes.resize(presentModeCount);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_Surface, &presentModeCount, supportedPresentModes.data());
		CEE_ASSERT(result == VK_SUCCESS);

		VkExtent2D swapchainExtent;
		if (surfaceCapabilities.currentExtent.width == static_cast<uint32_t>(-1)) {
			swapchainExtent.width = *width;
			swapchainExtent.height = *height;
		} else {
			swapchainExtent = surfaceCapabilities.currentExtent;
			*width = swapchainExtent.width;
			*height = swapchainExtent.height;
		}

		m_Width = *width;
		m_Height = *height;

		if ((m_Width == 0) || (m_Height == 0))
			return;

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

		if (!VSync) {
			for (auto&& supportedPresentMode : supportedPresentModes) {
				if (supportedPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
					presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				if ((presentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (supportedPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR))
					presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}

		uint32_t desiredSwapchainImageCount = surfaceCapabilities.minImageCount + 1;
		if ((surfaceCapabilities.maxImageCount > 0) && (desiredSwapchainImageCount > surfaceCapabilities.maxImageCount)) {
			desiredSwapchainImageCount = surfaceCapabilities.maxImageCount;
		}

		VkSurfaceTransformFlagsKHR preTransform;
		if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		} else {
			preTransform = surfaceCapabilities.currentTransform;
		}

		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
			compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
			compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
		else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
			compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
		else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
			compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		else
			CEE_ASSERT(false, "Faled to find supported compositeAlpha");

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = NULL,
			.flags = 0,
			.surface = m_Surface,
			.minImageCount = desiredSwapchainImageCount,
			.imageFormat = m_Format,
			.imageColorSpace = m_ColorSpace,
			.imageExtent = { swapchainExtent.width, swapchainExtent.height },
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = NULL,
			.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform),
			.compositeAlpha = compositeAlpha,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = oldSwapchain
		};

		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
			swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		result = vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, NULL, &m_Swapchain);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create swapchain");

		if (oldSwapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(vkDevice, oldSwapchain, NULL);

		for (auto& image : m_Images) {
			vkDestroyImageView(vkDevice, image.ImageView, NULL);
		}
		m_Images.clear();

		vkGetSwapchainImagesKHR(vkDevice, m_Swapchain, &m_ImageCount, NULL);
		CEE_ASSERT(m_ImageCount > 0);
		m_VulkanImages.resize(m_ImageCount);
		result = vkGetSwapchainImagesKHR(vkDevice, m_Swapchain, &m_ImageCount, m_VulkanImages.data());
		CEE_ASSERT(result == VK_SUCCESS, "Failed to get swapchain images");

		m_Images.resize(m_ImageCount);
		for (uint32_t i = 0; i < m_ImageCount; i++) {
			m_Images[i].Image = m_VulkanImages[i];

			VkImageViewCreateInfo imageViewCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = NULL,
				.flags = 0,
				.image = m_VulkanImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = m_Format,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_R,
					.g = VK_COMPONENT_SWIZZLE_G,
					.b = VK_COMPONENT_SWIZZLE_B,
					.a = VK_COMPONENT_SWIZZLE_A
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			result = vkCreateImageView(vkDevice, &imageViewCreateInfo, NULL, &m_Images[i].ImageView);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to create image view");
		}

		for (auto& commandBuffer : m_CommandBuffers)
			vkDestroyCommandPool(vkDevice, commandBuffer.CommandPool, NULL);

		VkCommandPoolCreateInfo commandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = NULL,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = m_PresentQueueIndex
		};

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = NULL,
			.commandPool = VK_NULL_HANDLE,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		m_CommandBuffers.resize(m_ImageCount);
		for (auto& commandBuffer : m_CommandBuffers) {
			result = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, NULL, &commandBuffer.CommandPool);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to create command pool");

			commandBufferAllocateInfo.commandPool = commandBuffer.CommandPool;
			result = vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, &commandBuffer.CommandBuffer);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to create command pool");
		}

		if (!m_Semaphores.RenderComplete || !m_Semaphores.PresentComplete) {
			VkSemaphoreCreateInfo semaphoreCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				.pNext = NULL,
				.flags = 0
			};

			result = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &m_Semaphores.RenderComplete);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to create semaphore \"RenderComplete\"");
			result = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &m_Semaphores.PresentComplete);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to create semaphore \"PresentComplete\"");
		}

		if (m_Fences.size() != m_ImageCount) {
			VkFenceCreateInfo fenceCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.pNext = NULL,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT
			};

			m_Fences.resize(m_ImageCount);
			for (auto& fence : m_Fences) {
				result = vkCreateFence(vkDevice, &fenceCreateInfo, NULL, &fence);
				CEE_ASSERT(result == VK_SUCCESS, "Failed to create fence");
			}
		}

		VkPipelineStageFlags submitInfoPipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		m_SubmitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_Semaphores.PresentComplete,
			.pWaitDstStageMask = &submitInfoPipelineStageFlags,
			.commandBufferCount = 0,
			.pCommandBuffers = NULL,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_Semaphores.RenderComplete,
		};

		// VkFormat depthFormat = m_Device->GetPhysicalDevice()->GetDepthFormat();

		VkAttachmentDescription colorAttachmentDesciption = {
			.flags = 0,
			.format = m_Format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		// VkAttachmentDescription depthAttachmentDesciption = {
		// 	.flags = 0,
		// 	.format = m_Format,
		// 	.samples = VK_SAMPLE_COUNT_1_BIT,
		// 	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		// 	.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		// 	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		// 	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		// 	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		// 	.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		// };

		VkAttachmentReference colorAttachmentReference = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		// VkAttachmentReference depthAttachmentReference = {
		// 	.attachment = 1,
		// 	.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		// };

		VkSubpassDescription colorSubpassDescription = {
			.flags = 0,
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount  =0,
			.pInputAttachments = NULL,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentReference,
			.pResolveAttachments = NULL,
			.pDepthStencilAttachment = NULL,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = NULL
		};

		VkSubpassDependency subpassDependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		};

		VkRenderPassCreateInfo renderPassCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.attachmentCount = 1,
			.pAttachments = &colorAttachmentDesciption,
			.subpassCount = 1,
			.pSubpasses = &colorSubpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};

		result = vkCreateRenderPass(vkDevice, &renderPassCreateInfo, NULL, &m_RenderPass);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create render pass");

		for (auto& framebuffer : m_Frambuffers)
			vkDestroyFramebuffer(vkDevice, framebuffer, NULL);

		m_Frambuffers.resize(m_ImageCount);
		for (uint32_t i = 0; i < m_ImageCount; i++) {
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = NULL,
				.flags = 0,
				.renderPass = m_RenderPass,
				.attachmentCount = 1,
				.pAttachments = &m_Images[i].ImageView,
				.width = m_Width,
				.height = m_Height,
				.layers = 1
			};
			result = vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, NULL, &m_Frambuffers[i]);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to create framebuffer");
		}
	}

	void Swapchain::Destroy()
	{
		VkDevice vkDevice = m_Device->GetDevice();

		vkDeviceWaitIdle(vkDevice);

		for (auto& framebuffer : m_Frambuffers)
			vkDestroyFramebuffer(vkDevice, framebuffer, NULL);

		if (m_RenderPass)
			vkDestroyRenderPass(vkDevice, m_RenderPass, NULL);

		for (auto& fence : m_Fences)
			vkDestroyFence(vkDevice, fence, NULL);

		if (m_Semaphores.PresentComplete)
			vkDestroySemaphore(vkDevice, m_Semaphores.PresentComplete, NULL);

		if (m_Semaphores.RenderComplete)
			vkDestroySemaphore(vkDevice, m_Semaphores.RenderComplete, NULL);

		for (auto& commandBuffer : m_CommandBuffers)
			vkDestroyCommandPool(vkDevice, commandBuffer.CommandPool, NULL);

		for (auto& image : m_Images)
			vkDestroyImageView(vkDevice, image.ImageView, NULL);

		if (m_Swapchain)
			vkDestroySwapchainKHR(vkDevice, m_Swapchain, NULL);

		vkDeviceWaitIdle(vkDevice);
	}

	void Swapchain::Resize(uint32_t newWidth, uint32_t newHeight)
	{
		VkDevice vkDevice = m_Device->GetDevice();

		vkDeviceWaitIdle(vkDevice);
		this->Create(&newWidth, &newHeight, m_VSync);
		vkDeviceWaitIdle(vkDevice);
	}

	void Swapchain::BeginFrame()
	{
		VkResult result = VK_SUCCESS;

		// TODO

		result = vkAcquireNextImageKHR(m_Device->GetDevice(),
									   m_Swapchain,
									   UINT64_MAX,
									   m_Semaphores.PresentComplete,
									   VK_NULL_HANDLE,
									   &m_ImageIndex);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to get next swapchain image");

		result = vkResetCommandPool(m_Device->GetDevice(), m_CommandBuffers[m_BufferIndex].CommandPool, 0);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to reset command pool");
	}

	void Swapchain::Present()
	{
		VkResult result = VK_SUCCESS;
		VkDevice vkDevice = m_Device->GetDevice();

		VkPipelineStageFlags submitWaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_Semaphores.PresentComplete,
			.pWaitDstStageMask = &submitWaitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = &m_CommandBuffers[m_BufferIndex].CommandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &m_Semaphores.RenderComplete
		};

		result = vkResetFences(vkDevice, 1, &m_Fences[m_BufferIndex]);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to reset fence");
		result = vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_Fences[m_BufferIndex]);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to submit");

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = NULL,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_Semaphores.RenderComplete,
			.swapchainCount = 1,
			.pSwapchains = &m_Swapchain,
			.pImageIndices = &m_ImageIndex,
			.pResults = NULL
		};
		result = vkQueuePresentKHR(m_Device->GetGraphicsQueue(), &presentInfo);
		if (result != VK_SUCCESS) {
			if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
				this->Resize(m_Width, m_Height);
			} else {
				CEE_VERIFY(false, "Failed to present");
			}
		}

		const RendererConfig config;
		m_BufferIndex = (m_BufferIndex + 1) % config.FramesInFlight;
		result = vkWaitForFences(vkDevice, 1, &m_Fences[m_BufferIndex], VK_TRUE, UINT64_MAX);
	}



}
