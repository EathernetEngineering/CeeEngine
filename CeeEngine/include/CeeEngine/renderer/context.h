/* Decleration of device and command pool objects for Vulkan.
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

#ifndef CEE_RENDERER_CONTEXT_H
#define CEE_RENDERER_CONTEXT_H

#include <CeeEngine/renderer/device.h>
#include <CeeEngine/renderer/renderer.h>

#include <vulkan/vulkan.h>
#include <memory>

namespace cee {
	class VulkanContext {
	public:
		VulkanContext()
		: m_PhysicalDevice(nullptr), m_Device(nullptr), m_DebugUtilsMessenger(VK_NULL_HANDLE),
		  m_PipelineCache(VK_NULL_HANDLE)
		{}

		void Init();

		static VkInstance GetInstance() { return s_Instance; };
		std::shared_ptr<PhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; }
		std::shared_ptr<Device> GetDevice() { return m_Device; }

		static std::shared_ptr<VulkanContext> Get() { return Renderer::GetContext(); }
		static std::shared_ptr<Device> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		std::shared_ptr<PhysicalDevice> m_PhysicalDevice;
		std::shared_ptr<Device> m_Device;

		VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
		VkPipelineCache m_PipelineCache;

	private:
		static inline VkInstance s_Instance;
	};
}

#endif
