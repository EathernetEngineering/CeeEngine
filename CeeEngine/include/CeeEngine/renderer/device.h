/* Decleration of device objects..
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



#ifndef CEE_RENDERER_DEVICE_H
#define CEE_RENDERER_DEVICE_H

#include <string>
#include <memory>
#include <vector>
#include <limits>
#include <unordered_set>
#include <map>
#include <thread>

#include <vulkan/vulkan.h>

#if defined(CEE_USE_AFTERMATH)
# include <CeeEngine/renderer/aftermathCrashTracker.h>
#endif

namespace cee {
	class Device;

	enum class QueueFamilyType {
		GRAPHICS = 0,
		COMPUTE = 1,
		TRANSFER = 2
	};

	class PhysicalDevice {
	public:
		struct QueueFamilyIndices {
			uint32_t graphics = std::numeric_limits<uint32_t>::max();
			uint32_t transfer = std::numeric_limits<uint32_t>::max();
			uint32_t compute = std::numeric_limits<uint32_t>::max();
		};
	public:
		PhysicalDevice();
		virtual ~PhysicalDevice();

		bool CheckExtensionSupport(const std::string& extensionName) const;
		uint32_t FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		const QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }

		VkFormat GetDepthFormat() const { return m_DepthForamt; }

		static std::shared_ptr<::cee::PhysicalDevice> Select();

	private:
		VkPhysicalDevice ChooseBestDevice(const std::vector<VkPhysicalDevice>& devices);
		VkFormat FindDepthFormat() const;
		QueueFamilyIndices GetQueueFamilyIndices(int queueFlags);

	private:
		QueueFamilyIndices m_QueueFamilyIndices;

		VkPhysicalDeviceProperties m_Properties;
		VkPhysicalDeviceMemoryProperties m_MemoryProperties;
		VkPhysicalDeviceFeatures m_Features;

		VkFormat m_DepthForamt = VK_FORMAT_UNDEFINED;

		std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
		std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;

		std::unordered_set<std::string> m_SupportedExtensions;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		friend class Device;
	};

	class CommandPool {
	public:
		CommandPool();
		virtual ~CommandPool();

		VkCommandBuffer AllocateCommandBuffer(bool begin, QueueFamilyType type = QueueFamilyType::GRAPHICS);
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer);
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);

		VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsPool; }
		VkCommandPool GetComputeCommandPool() const { return m_ComputePool; }
		VkCommandPool GetTransferCommandPool() const { return m_TransferPool; }

	private:
		VkCommandPool m_GraphicsPool, m_ComputePool, m_TransferPool;
	};

	class Device {
	public:
		Device(const std::shared_ptr<PhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		virtual ~Device();

		void Destroy();

		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetComputeQueue() { return m_ComputeQueue; }
		VkQueue GetTransferQueue() { return m_TransferQueue; }

		VkCommandBuffer GetCommandBuffer(bool begin, QueueFamilyType type = QueueFamilyType::GRAPHICS);
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer);
		void FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);

		const std::shared_ptr<PhysicalDevice> GetPhysicalDevice() const  { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_LogicalDevice; }

		std::shared_ptr<CommandPool> GetThreadLocalCommandPool();
		std::shared_ptr<CommandPool> GetOrCreateThreadLocalCommandPool();

	private:
		std::shared_ptr<PhysicalDevice> m_PhysicalDevice;
		VkDevice m_LogicalDevice;
		VkPhysicalDeviceFeatures m_EnabledFeatures;

#if defined(CEE_USE_AFTERMATH)
		AftermathCrashTracker* m_AftermathCrashTracker;
#endif

	private:
		VkQueue m_GraphicsQueue;
		VkQueue m_ComputeQueue;
		VkQueue m_TransferQueue;

		std::map<std::thread::id, std::shared_ptr<CommandPool>> m_CommandPools;
	};
}

#endif
