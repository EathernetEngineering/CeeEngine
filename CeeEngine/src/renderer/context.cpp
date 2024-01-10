/* Implementation of device and command pool objects for Vulkan.
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

#include <CeeEngine/renderer/context.h>

#include <cstdio>
#include <cstring>

#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/assert.h>

#if !defined(VK_API_VERSION_1_2)
# error "Wrong API Version. SDK with VK_API_VERSION_1_2 required."
#endif

namespace cee {
	constexpr const char *vkDebugUtilsMessageType(VkDebugUtilsMessageTypeFlagsEXT type) {
		switch (type) {
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:     return "General";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:  return "Validation";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
			default:                                              return "Unknown";
		}
	}

	constexpr const char *vkDebugUtilsMessageSeverity(VkDebugUtilsMessageSeverityFlagsEXT severity) {
		switch(severity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   return "error";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "warning";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    return "info";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "verbose";
			default:                                              return "unknown";
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugUtilsMessengerCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "[VALIDATION][%s][%s]: %s",
										 vkDebugUtilsMessageType(messageType),
										 vkDebugUtilsMessageSeverity(messageSeverity),
										 pCallbackData->pMessage);

		(void)pUserData; // Unused parameter
		return VK_FALSE;
	}

	static void CheckDriverApiVersionSupport(uint32_t minimumVersion) {
		char msg[4096];
		uint32_t actualVersion;
		vkEnumerateInstanceVersion(&actualVersion);
		if (actualVersion < minimumVersion) {
			sprintf(msg, "Driver version incompatible.\n"
						 "\tMinimum version: %u.%u.%u\n"
						 "\tYour version: %u.%u.%u",
					VK_VERSION_MAJOR(minimumVersion), VK_VERSION_MINOR(minimumVersion),
					VK_VERSION_PATCH(minimumVersion),
					VK_VERSION_MAJOR(actualVersion), VK_VERSION_MINOR(actualVersion),
					VK_VERSION_PATCH(actualVersion));
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Driver version incompatible.\n"
											 "\tMinimum version: %u.%u.%u\n"
											 "\tYour version: %u.%u.%u",
											 VK_VERSION_MAJOR(minimumVersion),
											 VK_VERSION_MINOR(minimumVersion),
											 VK_VERSION_PATCH(minimumVersion),
											 VK_VERSION_MAJOR(actualVersion),
											 VK_VERSION_MINOR(actualVersion),
											 VK_VERSION_PATCH(actualVersion));
			CEE_ASSERT(false, "Driver version incompatible.");
		} else {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "Using driver version %u.%u.%u",
											 VK_VERSION_MAJOR(actualVersion),
											 VK_VERSION_MINOR(actualVersion),
											 VK_VERSION_PATCH(actualVersion));
		}
	}

	void VulkanContext::Init() {
		VkResult result;
		CheckDriverApiVersionSupport(VK_VERSION_1_2);

		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.pApplicationName = "CeeEngine",
			.applicationVersion = 1,
			.pEngineName = "CeeEngine",
			.engineVersion = 1,
			.apiVersion = VK_API_VERSION_1_2
		};

		std::vector<VkLayerProperties> instanceLayerProperties;
		uint32_t instanceLayerPropertyCount;

		std::vector<const char*> instanceLayers;

		result = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, nullptr);
		if (result != VK_SUCCESS) {
			CEE_ASSERT(false, "Failed to enumerate instance layer properties.");
		}
		instanceLayerProperties.resize(instanceLayerPropertyCount);
		result = vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, instanceLayerProperties.data());
		if (result != VK_SUCCESS) {
			CEE_ASSERT(false, "Failed to enumerate instance layer properties.");
		}

		for (auto& layerProperty : instanceLayerProperties) {
			if (strcmp(layerProperty.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
				instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan instance layer: %s",
												 layerProperty.layerName);
			}
		}

		std::vector<VkExtensionProperties> instanceExtensionProperties;
		uint32_t instanceExtensionPropertyCount;

		std::vector<const char*> instanceExtensions;

		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount, nullptr);
		if (result != VK_SUCCESS) {
			CEE_ASSERT(false, "Failed to enumerate instance extension properties.");
		}
		instanceExtensionProperties.resize(instanceExtensionPropertyCount);
		result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertyCount,
														instanceExtensionProperties.data());
		if (result != VK_SUCCESS) {
			CEE_ASSERT(false, "Failed to enumerate instance extension properties.");
		}

#if defined(CEE_PLATFORM_WINDOWS)
			const char *const platformSurfaceExtensionName = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(CEE_PLATFORM_LINUX)
			const char *const platformSurfaceExtensionName = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#endif
		for (auto& extensionProperty : instanceExtensionProperties) {
			if (strcmp(extensionProperty.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
				instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan instance extension: %s",
												 extensionProperty.extensionName);
			}
			if (strcmp(extensionProperty.extensionName, platformSurfaceExtensionName) == 0) {
				instanceExtensions.push_back(platformSurfaceExtensionName);
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan instance extension: %s",
												 extensionProperty.extensionName);
			}
			if (strcmp(extensionProperty.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan instance extension: %s",
												 extensionProperty.extensionName);

			}
		}

		VkInstanceCreateInfo instanceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
			.ppEnabledLayerNames = instanceLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
			.ppEnabledExtensionNames = instanceExtensions.data()
		};

		result = vkCreateInstance(&instanceCreateInfo, nullptr, &s_Instance);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to create Vulkan instance.");

		auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_Instance, "vkCreateDebugUtilsMessengerEXT");
		CEE_ASSERT(vkCreateDebugUtilsMessengerEXT != NULL, "Failed to get address for vkCreateDebugUtilsMessengerEXT");
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = NULL,
			.flags = 0,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = VulkanDebugUtilsMessengerCallback,
			.pUserData = nullptr
		};
		result = vkCreateDebugUtilsMessengerEXT(s_Instance, &debugUtilsMessengerCreateInfo, nullptr, &m_DebugUtilsMessenger);
		CEE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan debug utils messenger!");

		m_PhysicalDevice = PhysicalDevice::Select();
		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.fillModeNonSolid = true;
		enabledFeatures.pipelineStatisticsQuery = true;
		m_Device = std::make_shared<Device>(m_PhysicalDevice, enabledFeatures);
	}
}
