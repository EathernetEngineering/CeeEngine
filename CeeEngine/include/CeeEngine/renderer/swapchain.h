/* Decleration of swapchain objects for Vulkan.
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

#ifndef CEE_RENDERER_SWAPCHAIN_H
#define CEE_RENDERER_SWAPCHAIN_H

#include <CeeEngine/renderer/device.h>
#include <CeeEngine/renderer/context.h>
#include <CeeEngine/window.h>
#include <CeeEngine/assert.h>


namespace cee {
	class Swapchain {
	public:
		VkImage GetImage() { return m_Images[m_ImageIndex].Image; }

		Swapchain() = default;
		virtual ~Swapchain() = default;

		void Init(VkInstance instance, const std::shared_ptr<Device>& device);
		void InitSurface(NativeWindowConnection* connection, NativeWindowHandle handle);
		void Create(uint32_t* width, uint32_t* height, bool VSync);
		void Destroy();

		void Resize(uint32_t newWidth, uint32_t newHeight);

		void BeginFrame();
		void Present();

		uint32_t GetImageCount() const { return m_ImageCount; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		VkRenderPass GetRenderPass() { return m_RenderPass; }

		uint32_t GetImageIndex() const { return m_ImageIndex; }
		VkFramebuffer GetFrameBuffer(uint32_t i)
		{
			CEE_ASSERT(i < m_Frambuffers.size());
			return m_Frambuffers[i];

		}
		VkFramebuffer GetCurrentFrameBuffer() { return GetFrameBuffer(m_ImageIndex); }

		uint32_t GetBufferIndex() const { return m_BufferIndex; }
		VkCommandBuffer GetDrawCommandBuffer(uint32_t i)
		{
			CEE_ASSERT(i < m_CommandBuffers.size());
			return m_CommandBuffers[i].CommandBuffer;

		}
		VkCommandBuffer GetCurrentDrawCommandBuffer() { return GetDrawCommandBuffer(m_BufferIndex); }

		VkFormat GetColorFormat() { return m_Format; }

		VkSemaphore GetRenderCompleteSemaphore() { return m_Semaphores.RenderComplete; }

		void SetVSync(bool VSync) { m_VSync = VSync; }

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		std::shared_ptr<Device> m_Device;
		bool m_VSync;

		VkFormat m_Format;
		VkColorSpaceKHR m_ColorSpace;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		uint32_t m_ImageCount;
		std::vector<VkImage> m_VulkanImages;

		struct SwapchainImage {
			VkImage Image = VK_NULL_HANDLE;
			VkImageView ImageView = VK_NULL_HANDLE;
		};
		std::vector<SwapchainImage> m_Images;

		struct SwapchainDepthStencil {
			VkImage Image = VK_NULL_HANDLE;
			VkDeviceMemory ImageMemory = VK_NULL_HANDLE;
			VkDeviceSize Size = 0;
			VkImageView ImageView = VK_NULL_HANDLE;
		};
		SwapchainDepthStencil m_DepthStencil;

		std::vector<VkFramebuffer> m_Frambuffers;

		struct SwapchainCommandBuffer {
			VkCommandPool CommandPool = VK_NULL_HANDLE;
			VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
		};
		std::vector<SwapchainCommandBuffer> m_CommandBuffers;

		struct SwapchainSemaphores {
			VkSemaphore PresentComplete = VK_NULL_HANDLE;
			VkSemaphore RenderComplete = VK_NULL_HANDLE;
		};
		SwapchainSemaphores m_Semaphores;

		VkSubmitInfo m_SubmitInfo;

		std::vector<VkFence> m_Fences;

		VkRenderPass m_RenderPass = VK_NULL_HANDLE;

		uint32_t m_BufferIndex = 0, m_ImageIndex = 0;
		uint32_t m_PresentQueueIndex;

		uint32_t m_Width = 0, m_Height = 0;

		VkSurfaceKHR m_Surface;

		friend class Context;
	};
}

#endif
