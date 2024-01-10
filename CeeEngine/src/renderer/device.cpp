/* Implementation of device objects.
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

#include <CeeEngine/renderer/device.h>

#include <algorithm>
#include <thread>

#include <CeeEngine/assert.h>
#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/renderer/context.h>

#if defined(CEE_USE_AFTERMATH)
# include <CeeEngine/renderer/aftermathCrashTracker.h>
# include <GFSDK_Aftermath.h>
#endif

namespace cee {
	PhysicalDevice::PhysicalDevice() {
		VkResult result = VK_SUCCESS;
		VkInstance instance = VulkanContext::GetInstance();

		// Get physical devices
		{
			uint32_t physicalDeviceCount = 0;
			std::vector<VkPhysicalDevice> physicalDevices;

			vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
			CEE_VERIFY(physicalDeviceCount > 0, "No GPU found");

			physicalDevices.resize(physicalDeviceCount);
			result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
			CEE_VERIFY(result == VK_SUCCESS, "Failed to get physical devices");

			m_PhysicalDevice = ChooseBestDevice(physicalDevices);
			CEE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to select a GPU.");

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "Using GPU \"%s\"\n\tdiscrete: %s\n\tAPI version: %u.%u.%u",
											 deviceProperties.deviceName,
											 &"false\0true"[(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)*6],
											 VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
											 VK_API_VERSION_MINOR(deviceProperties.apiVersion),
											 VK_API_VERSION_PATCH(deviceProperties.apiVersion));
		} // Get physical devices

		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_Features);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

		// Set up queue family info
		{
			uint32_t queueFamilyPropertiesCount;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount, nullptr);
			CEE_ASSERT(queueFamilyPropertiesCount > 0, "Failed to get queue family properties.");

			m_QueueFamilyProperties.resize(queueFamilyPropertiesCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount, m_QueueFamilyProperties.data());

			static const float defaultQueuePriority = 1.0f;

			m_QueueFamilyIndices = GetQueueFamilyIndices(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

			VkDeviceQueueCreateInfo queueCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = m_QueueFamilyIndices.graphics,
				.queueCount = 1,
				.pQueuePriorities = &defaultQueuePriority
			};
			m_QueueCreateInfos.push_back(queueCreateInfo);

			if (m_QueueFamilyIndices.compute != m_QueueFamilyIndices.graphics) {
				queueCreateInfo = {
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.queueFamilyIndex = m_QueueFamilyIndices.compute,
					.queueCount = 1,
					.pQueuePriorities = &defaultQueuePriority
				};
				m_QueueCreateInfos.push_back(queueCreateInfo);
			}

			if ((m_QueueFamilyIndices.transfer != m_QueueFamilyIndices.graphics) && (m_QueueFamilyIndices.transfer != m_QueueFamilyIndices.compute)) {
				queueCreateInfo = {
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.queueFamilyIndex = m_QueueFamilyIndices.transfer,
					.queueCount = 1,
					.pQueuePriorities = &defaultQueuePriority
				};
				m_QueueCreateInfos.push_back(queueCreateInfo);
			}

			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using queue family indices: \n"
											 "\tGraphics = %u\n"
											 "\tCompute  = %u\n"
											 "\tTransfer = %u\n",
											 m_QueueFamilyIndices.graphics,
											 m_QueueFamilyIndices.compute,
											 m_QueueFamilyIndices.transfer);
		} // Set up queue family info

		// Find supported extensions
		{
			uint32_t extensionCount;
			std::vector<VkExtensionProperties> extensions;

			vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
			if (extensionCount > 0) {
				extensions.resize(extensionCount);
				result = vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, extensions.data());
				CEE_ASSERT(result == VK_SUCCESS, "Failed to get device extensions");

				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "Found %u device extensions.", extensionCount);

				std::for_each(extensions.begin(), extensions.end(), [this](VkExtensionProperties extensionProperties){
					m_SupportedExtensions.emplace(extensionProperties.extensionName);
				});
			}
		} // Find supported extensions

		m_DepthForamt = FindDepthFormat();
		CEE_ASSERT(m_DepthForamt, "Failed to find depth format.");
	}

	PhysicalDevice::~PhysicalDevice() {

	}

	VkPhysicalDevice PhysicalDevice::ChooseBestDevice(const std::vector<VkPhysicalDevice>& devices)
	{
		VkPhysicalDeviceProperties properties;
		uint32_t score = 0;
		uint32_t maxScore = 0;
		uint32_t maxScoreIndex = 0;

		for (uint32_t i = 0; i < devices.size(); i++) {
			score = 0;

			vkGetPhysicalDeviceProperties(devices[i], &properties);
			score += (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) * 1000000;
			score += properties.limits.maxDrawIndexedIndexValue;
			score += properties.limits.maxPushConstantsSize;
			score += properties.limits.maxViewports * 1000;
			score += properties.limits.maxFramebufferWidth * 10;
			score += properties.limits.maxFramebufferHeight * 10;

			maxScore = std::max(score, maxScore);
			if (score == maxScore) {
				maxScoreIndex = i;
			}
		}

		return devices[maxScoreIndex];
	}


	bool PhysicalDevice::CheckExtensionSupport(const std::string& extensionName) const {
		return m_SupportedExtensions.contains(extensionName);
	}

	uint32_t PhysicalDevice::FindMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const {
		for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++) {
			if ((typeBits & 1) == 1) {
				if ((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					return i;
			}
			typeBits >>= 1;
		}

		CEE_ASSERT(false, "Failed to find suitable memory type.");
		return std::numeric_limits<uint32_t>::max();
	}

	std::shared_ptr<PhysicalDevice> PhysicalDevice::Select() {
		return std::make_shared<PhysicalDevice>();
	}

	VkFormat PhysicalDevice::FindDepthFormat() const {
		static const VkFormat formats[] {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};

		for (uint32_t i = 0; i < sizeof(formats)/sizeof(formats[0]); i++) {
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, formats[i], &formatProperties);
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				return formats[i];
		}

		return VK_FORMAT_UNDEFINED;
	}

	PhysicalDevice::QueueFamilyIndices PhysicalDevice::GetQueueFamilyIndices(int queueFlags) {
		QueueFamilyIndices indices;

		// Find dedicated compute queue that doesn't support graphics
		if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++) {
				if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
					((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)) {
						indices.compute = i;
						break;
				}
			}
		}

		// Find dedicated transfer queue that doesn't support compute or graphics
		if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
			for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++) {
				if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
					((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
					((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
					indices.transfer = i;
					break;
				}
			}
		}

		for (uint32_t i = 0; i < m_QueueFamilyProperties.size(); i++) {
			if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
				indices.compute == std::numeric_limits<uint32_t>::max()) {
					indices.compute = i;
			}
			if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
				indices.transfer == std::numeric_limits<uint32_t>::max()) {
				indices.transfer = i;
			}
			if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
				indices.graphics == std::numeric_limits<uint32_t>::max()) {
				indices.graphics = i;
				if ((indices.compute != std::numeric_limits<uint32_t>::max()) &&
					(indices.transfer != std::numeric_limits<uint32_t>::max())) {
						break;
				}
			}
		}

		return indices;
	}

	CommandPool::CommandPool() {
		std::shared_ptr<Device> device = VulkanContext::GetCurrentDevice();
		VkResult result = VK_SUCCESS;

		VkCommandPoolCreateInfo poolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().graphics
		};

		result = vkCreateCommandPool(device->GetDevice(), &poolCreateInfo, nullptr, &m_GraphicsPool);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create graphics command pool.");

		poolCreateInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().compute;
		result = vkCreateCommandPool(device->GetDevice(), &poolCreateInfo, nullptr, &m_ComputePool);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create compute command pool.");

		poolCreateInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().transfer;
		result = vkCreateCommandPool(device->GetDevice(), &poolCreateInfo, nullptr, &m_TransferPool);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create transfer command pool.");
	}

	CommandPool::~CommandPool() {
		VkDevice device = VulkanContext::GetCurrentDevice()->GetDevice();

		vkDestroyCommandPool(device, m_GraphicsPool, nullptr);
		vkDestroyCommandPool(device, m_ComputePool, nullptr);
		vkDestroyCommandPool(device, m_TransferPool, nullptr);
	}

	VkCommandBuffer CommandPool::AllocateCommandBuffer(bool begin, QueueFamilyType type) {
		VkDevice device = VulkanContext::GetCurrentDevice()->GetDevice();
		VkResult result = VK_SUCCESS;

		VkCommandPool selectedPool = VK_NULL_HANDLE;
		switch (type) {
			case QueueFamilyType::GRAPHICS:
				selectedPool = m_GraphicsPool;
				break;
			case QueueFamilyType::COMPUTE:
				selectedPool = m_ComputePool;
				break;
			case QueueFamilyType::TRANSFER:
				selectedPool = m_TransferPool;
				break;
		}

		VkCommandBufferAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = selectedPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		result = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to allocate command buffer.");

		if (begin) {
			VkCommandBufferBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pInheritanceInfo = nullptr
			};
			result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
			CEE_ASSERT(result == VK_SUCCESS, "Failed to start command buffer");
		}

		return commandBuffer;
	}

	void CommandPool::FlushCommandBuffer(VkCommandBuffer cmdBuffer) {
		std::shared_ptr<Device> device = VulkanContext::GetCurrentDevice();
		this->FlushCommandBuffer(cmdBuffer, device->GetGraphicsQueue());
	}

	void CommandPool::FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) {
		std::shared_ptr<Device> device = VulkanContext::GetCurrentDevice();
		VkResult result = VK_SUCCESS;
		static std::mutex submissionLock;

		CEE_ASSERT(cmdBuffer != VK_NULL_HANDLE);

		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		VkFence fence;
		result = vkCreateFence(device->GetDevice(), &fenceCreateInfo, nullptr, &fence);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create fence for command pool flush.");

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmdBuffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		result = vkQueueSubmit(queue, 1, &submitInfo, fence);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to submit command buffer for command pool flush.");

		result = vkWaitForFences(device->GetDevice(), 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		CEE_ASSERT(result == VK_SUCCESS, "Wait for fences error or timeout.");

		vkDestroyFence(device->GetDevice(), fence, nullptr);
		// TODO Choose correct pool. unordered_map?
		vkFreeCommandBuffers(device->GetDevice(), m_GraphicsPool, 1, &cmdBuffer);
	}

	Device::Device(const std::shared_ptr<PhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures)
	 : m_PhysicalDevice(physicalDevice), m_EnabledFeatures(enabledFeatures)
	{
		VkResult result = VK_SUCCESS;
		std::vector<const char*> extensions;

		if (physicalDevice->CheckExtensionSupport(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
			extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = static_cast<uint32_t>(physicalDevice->m_QueueCreateInfos.size()),
			.pQueueCreateInfos = physicalDevice->m_QueueCreateInfos.data(),
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = 0,
			.ppEnabledExtensionNames = nullptr,
			.pEnabledFeatures = &enabledFeatures
		};

#if defined(CEE_USE_AFTERMATH)
		VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo;
		if (m_PhysicalDevice->CheckExtensionSupport(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME) &&
			m_PhysicalDevice->CheckExtensionSupport(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME)) {

			VkDeviceDiagnosticsConfigFlagsNV aftermathFlags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
				VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV;

			aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
			aftermathInfo.pNext = nullptr;
			aftermathInfo.flags = aftermathFlags;

			m_AftermathCrashTracker = new AftermathCrashTracker;
			m_AftermathCrashTracker->Initialize();

			extensions.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);
			extensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

			deviceCreateInfo.pNext = &aftermathInfo;
		} else {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to enable afternath, extension not supported on device.");
		}
#endif

		for (auto extensionName : extensions) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using device extension %s", extensionName);
		}
		deviceCreateInfo.enabledExtensionCount = extensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

		result = vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &m_LogicalDevice);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan logical device.");

		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->GetQueueFamilyIndices().graphics, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->GetQueueFamilyIndices().compute, 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->GetQueueFamilyIndices().transfer, 0, &m_TransferQueue);
	}

	Device::~Device() {
#if defined(CEE_USE_AFTERMATH)
		if (m_AftermathCrashTracker)
			free(m_AftermathCrashTracker);
#endif
	}

	void Device::Destroy() {
		m_CommandPools.clear();
		vkDeviceWaitIdle(m_LogicalDevice);
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	VkCommandBuffer Device::GetCommandBuffer(bool begin, QueueFamilyType type) {
		return this->GetOrCreateThreadLocalCommandPool()->AllocateCommandBuffer(begin, type);
	}

	void Device::FlushCommandBuffer(VkCommandBuffer cmdBuffer) {
		this->GetThreadLocalCommandPool()->FlushCommandBuffer(cmdBuffer);
	}

	void Device::FlushCommandBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) {
		this->GetThreadLocalCommandPool()->FlushCommandBuffer(cmdBuffer, queue);
	}

	std::shared_ptr<CommandPool> Device::GetThreadLocalCommandPool() {
		std::thread::id threadID = std::this_thread::get_id();
		CEE_ASSERT(m_CommandPools.find(threadID) != m_CommandPools.end());

		return m_CommandPools.at(threadID);
	}

	std::shared_ptr<CommandPool> Device::GetOrCreateThreadLocalCommandPool() {
		std::thread::id threadID = std::this_thread::get_id();
		if (m_CommandPools.find(threadID) != m_CommandPools.end())
			return m_CommandPools.at(threadID);

		std::shared_ptr<CommandPool> commandPool = std::make_shared<CommandPool>();
		m_CommandPools.emplace(threadID, commandPool);
		return commandPool;
	}
}
