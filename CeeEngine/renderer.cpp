#include "CeeEngine/CeeEngine.h"
#include "CeeEngine/CeeEngineRenderer.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <vector>
#include <set>
#include <algorithm>

#include <fstream>

#include "glm/glm.hpp"

#define RENDERER_MAX_FRAME_IN_FLIGHT 5u

#define RENDERER_MAX_INDICES 20000u
#define RENDERER_MIN_INDICES 500u

namespace cee {
	VkBool32 Renderer::VulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
										VkDebugUtilsMessageTypeFlagsEXT messageType,
										const VkDebugUtilsMessengerCallbackDataEXT* messageData,
										void* userData) {
		char messageTypeName[64];
		memset(messageTypeName, 0, sizeof(messageTypeName));
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
			strcat(messageTypeName, "GENERAL");
		}
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
			if (strlen(messageTypeName) != 0)
				strcat(messageTypeName, ",");
			strcat(messageTypeName, "VALIDATION");
		}
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
			if (strlen(messageTypeName) != 0)
				strcat(messageTypeName, ",");
			strcat(messageTypeName, "PERFORMANCE");
		}
		char message[4096];
		CeeErrorSeverity ceeMessageSeverity;
		switch (messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				// Do nothing;
				return VK_FALSE;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				ceeMessageSeverity = CEE_ERROR_SEVERITY_INFO;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				ceeMessageSeverity = CEE_ERROR_SEVERITY_WARNING;
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				ceeMessageSeverity = CEE_ERROR_SEVERITY_ERROR;
				break;

			default:
				sprintf(message, "[%s] Unknown error type.\tMessage: %s", messageTypeName, messageData->pMessage);
				cee::DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
				return VK_FALSE;
		}
		sprintf(message, "[%s] %s", messageTypeName, messageData->pMessage);
		cee::DebugMessenger::PostDebugMessage(ceeMessageSeverity, message);
		(void)userData;
		return VK_FALSE;
	}

	Renderer::Renderer(MessageBus* messageBus,
					   const RendererCapabilities& capabilities,
					   std::shared_ptr<Window> window)
	 : m_MessageBus(messageBus), m_Capabilites(capabilities), m_Window(window),
	   m_Instance(VK_NULL_HANDLE), m_PhysicalDevice(VK_NULL_HANDLE), m_PhysicalDeviceProperties({}),
	   m_Device(VK_NULL_HANDLE), m_Surface(VK_NULL_HANDLE), m_Swapchain(VK_NULL_HANDLE),
	   m_RenderPass(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE),
	   m_PipelineCache(VK_NULL_HANDLE), m_MainPipeline(VK_NULL_HANDLE),
	   m_LinePipeline(VK_NULL_HANDLE), m_PresentQueue(VK_NULL_HANDLE),
	   m_GraphicsQueue(VK_NULL_HANDLE), m_TransferQueue(VK_NULL_HANDLE),
	   m_GraphicsCmdPool(VK_NULL_HANDLE), m_TransferCmdPool(VK_NULL_HANDLE),
	   m_VertexBufferDeviceMemory(VK_NULL_HANDLE), m_IndexBuffer(VK_NULL_HANDLE),
	   m_IndexBufferDeviceMemory(VK_NULL_HANDLE),
	   m_DebugMessenger(VK_NULL_HANDLE)
	{
		m_MessageBus->RegisterMessageHandler([this](Event& e){ return this->MessageHandler(e); });
		m_Running = false;
	}

	Renderer::~Renderer()
	{
		this->Shutdown();
	}

	int Renderer::Init()
	{
		VkResult result = VK_SUCCESS;

		if (m_Capabilites.applicationName == NULL) {
			m_Capabilites.applicationName = "CeeEngine Application";
		}
		if (m_Capabilites.applicationVersion == 0) {
			m_Capabilites.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		}
		if (m_Capabilites.maxIndices == 0) {
			m_Capabilites.maxIndices = 10000;
		}
		if (m_Capabilites.maxFramesInFlight == 0) {
			m_Capabilites.maxFramesInFlight = 2;
		}
		m_Capabilites.maxIndices = std::clamp(m_Capabilites.maxIndices,
											  RENDERER_MIN_INDICES,
											  RENDERER_MAX_INDICES);
		m_Capabilites.maxFramesInFlight = std::clamp(m_Capabilites.maxFramesInFlight,
													 1u,
													 RENDERER_MAX_FRAME_IN_FLIGHT);
		memset(m_FrameTimes, 0, sizeof(uint64_t) * 100);
		m_FrameTimesIndex = 0;

		{
			VkLayerProperties *layerProperties = NULL;
			uint32_t layerPropertiesCount = 0;
			VkExtensionProperties *extensionProperties = NULL;
			uint32_t extensionPropertiesCount = 0;
			std::vector<const char*> enabledLayers;
			std::vector<const char*> enabledExtensions;

			result = vkEnumerateInstanceLayerProperties(&layerPropertiesCount, NULL);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate instance layer properties.");
				return -1;
			}
			layerProperties = (VkLayerProperties*)std::calloc(100, sizeof(VkLayerProperties));
			if (layerProperties == NULL) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Out of memory.");
				return -1;
			}
			result = vkEnumerateInstanceLayerProperties(&layerPropertiesCount, layerProperties);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate instance layer properties.");
				return -1;
			}

			for (uint32_t i = 0; i < layerPropertiesCount; i++) {
#ifndef NDEBUG
				if (strcmp(layerProperties[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
					enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
					char message[512];
					sprintf(message, "Using Vulkan layer %s.", layerProperties[i].layerName);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, message);
					continue;
				}
#endif
			}
			std::free(layerProperties);
			layerProperties = NULL;

			result = vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertiesCount, NULL);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate instance extension properties.");
				return -1;
			}
			extensionProperties = (VkExtensionProperties*)std::calloc(extensionPropertiesCount, sizeof(VkExtensionProperties));
			if (extensionProperties == NULL) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Out of memory.");
				return -1;
			}
			result = vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertiesCount, extensionProperties);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate instance extension properties.");
				return -1;
			}

#if defined(CEE_PLATFORM_WINDOWS)
			const char *const surfaceExtensionName = "VK_KHR_win32_surface";
#elif defined(CEE_PLATFORM_LINUX)
			const char *const surfaceExtensionName = "VK_KHR_xcb_surface";
#endif
			for (uint32_t i = 0; i < extensionPropertiesCount; i++) {
				if (strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extensionProperties[i].extensionName) == 0) {
					enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
					char message[512];
					sprintf(message, "Using Vulkan extension %s.", extensionProperties[i].extensionName);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, message);
					continue;
				}
				if (strcmp(surfaceExtensionName, extensionProperties[i].extensionName) == 0) {
					enabledExtensions.push_back(surfaceExtensionName);
					char message[512];
					sprintf(message, "Using Vulkan extension %s.", extensionProperties[i].extensionName);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, message);
					continue;
				}
#ifndef NDEBUG
				if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProperties[i].extensionName) == 0) {
					enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
					char message[512];
					sprintf(message, "Using Vulkan extension %s.", extensionProperties[i].extensionName);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, message);
					continue;
				}
#endif
			}
			std::free(extensionProperties);
			extensionProperties = NULL;

			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = NULL;
			appInfo.apiVersion = VK_API_VERSION_1_3;
			appInfo.pEngineName = "CeeEngine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pApplicationName = m_Capabilites.applicationName;
			appInfo.applicationVersion = m_Capabilites.applicationVersion;

			VkInstanceCreateInfo instanceCreateInfo = {};
			instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			instanceCreateInfo.pNext = NULL;
			instanceCreateInfo.flags = 0;
			instanceCreateInfo.pApplicationInfo = &appInfo;
			instanceCreateInfo.enabledLayerCount = enabledLayers.size();
			instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
			instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
			instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

			result = vkCreateInstance(&instanceCreateInfo, NULL, &m_Instance);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create instance.");
				return -1;
			}
		}
		{
			PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXTfn =
				(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

			VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
			debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugMessengerCreateInfo.pNext = NULL;
			debugMessengerCreateInfo.flags = 0;
			debugMessengerCreateInfo.pUserData = this;
			debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugMessengerCreateInfo.pfnUserCallback = this->VulkanDebugMessengerCallback;

			if (vkCreateDebugUtilsMessengerEXTfn)
				vkCreateDebugUtilsMessengerEXTfn(m_Instance, &debugMessengerCreateInfo, NULL, &m_DebugMessenger);
			else
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to get proc address for 'vkCreateDebugUtilsMessengerEXT'");
		}
		{
			uint32_t physicalDeviceCount = 0;
			VkPhysicalDevice *physicalDevices;
			result = vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, NULL);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate physical devices.");
				return -1;
			}
			physicalDevices = (VkPhysicalDevice*)std::calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
			if (physicalDevices == NULL) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Out of memory.");
				return -1;
			}
			result = vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate physical devices.");
				return -1;
			}
			m_PhysicalDevice = this->ChoosePhysicalDevice(physicalDeviceCount, physicalDevices);
			std::free(physicalDevices);

			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
			char message[4096];
			sprintf(message, "Phsysical device properties:\n"
							"\tDevice Name: %s\n"
							"\tDiscrete: %s\n"
							"\tApi Version: %u.%u.%u",
							m_PhysicalDeviceProperties.deviceName,
							&"false\0true"[6*(m_PhysicalDeviceProperties.deviceType ==
							VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)],
							(m_PhysicalDeviceProperties.apiVersion & 0x1FC00000) >> 22,
							(m_PhysicalDeviceProperties.apiVersion & 0x3FF000) >> 12,
							m_PhysicalDeviceProperties.apiVersion & 0xFFF);
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, message);
		}
		{
#if defined(CEE_PLATFORM_WINDOWS)
			VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
			surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			surfaceCreateInfo.hinstance = m_Window->GetNativeConnection();
			surfaceCreateInfo.hwnd = m_Window->GetNativeWindowHandle();
#elif defined(CEE_PLATFORM_LINUX)
			VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
			surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
			surfaceCreateInfo.connection = m_Window->GetNativeConnection();
			surfaceCreateInfo.window = m_Window->GetNativeWindowHandle();
#endif
			surfaceCreateInfo.pNext = NULL;
			surfaceCreateInfo.flags = 0;
#if defined(CEE_PLATFORM_WINDOWS)
			result = vkCreateXcbSurfaceKHR(m_Instance, &surfaceCreateInfo, NULL, &m_Surface);
#elif defined(CEE_PLATFORM_LINUX)
			result = vkCreateXcbSurfaceKHR(m_Instance, &surfaceCreateInfo, NULL, &m_Surface);
#endif
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create surface.");
				return -1;
			}
		}
		{
			uint32_t queueFamilyPropertiesCount;
			VkQueueFamilyProperties *queueFamilyProperties;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount, NULL);
			queueFamilyProperties = (VkQueueFamilyProperties*)std::calloc(queueFamilyPropertiesCount, sizeof(VkQueueFamilyProperties));
			if (queueFamilyProperties == NULL) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Out of memory.");
				return -1;
			}
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties);

			for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
				VkBool32 presentSupport = VK_FALSE;
				// Prefer queue that supports both transfer and graphics operations
				if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
					queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					m_QueueFamilyIndices.graphicsIndex = i;
					m_QueueFamilyIndices.transferIndex = i;
					// Prefer queue that supports both present and graphics operations
					if (vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport) == VK_SUCCESS)
					{
						if (presentSupport)
							m_QueueFamilyIndices.presentIndex = i;
					}
					continue;
				}
				if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
					!m_QueueFamilyIndices.graphicsIndex.has_value())
				{
					m_QueueFamilyIndices.graphicsIndex = i;
				}
				if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT &&
					!m_QueueFamilyIndices.computeIndex.has_value())
				{
					m_QueueFamilyIndices.computeIndex = i;
				}
				if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
					!m_QueueFamilyIndices.transferIndex.has_value())
				{
					m_QueueFamilyIndices.transferIndex = i;
				}
				if (!m_QueueFamilyIndices.presentIndex.has_value()) {
					if (vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport) == VK_SUCCESS)
					{
						if (presentSupport)
							m_QueueFamilyIndices.presentIndex = i;
					}

				}
			}
			std::free(queueFamilyProperties);

			if (!(m_QueueFamilyIndices.graphicsIndex.has_value() &&
				m_QueueFamilyIndices.computeIndex.has_value() &&
				m_QueueFamilyIndices.transferIndex.has_value() &&
				m_QueueFamilyIndices.presentIndex.has_value()))
			{
				char message[4096];
				sprintf(message, "Queue family without value.\n"
								 "\tPresent Queue index: %u\n"
								 "\tGraphics Queue index: %u\n"
								 "\tCompute Queue index:  %u\n"
								 "\tTransfer Queue index: %u",
								 m_QueueFamilyIndices.presentIndex.value_or(-1),
								 m_QueueFamilyIndices.graphicsIndex.value_or(-1),
								 m_QueueFamilyIndices.computeIndex.value_or(-1),
								 m_QueueFamilyIndices.transferIndex.value_or(-1));
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, message);
			}
		}
		{
			VkExtensionProperties *extensionProperties = NULL;
			uint32_t extensionPropertiesCount = 0;
			std::vector<const char*> enabledExtensions;

			result = vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionPropertiesCount, NULL);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate device extensions.");
				return -1;
			}
			extensionProperties = (VkExtensionProperties*)std::calloc(extensionPropertiesCount, sizeof(VkExtensionProperties));
			if (extensionProperties == NULL) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Out of memory.");
				return -1;
			}
			result = vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionPropertiesCount, extensionProperties);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to enumerate device extensions.");
				return -1;
			}

			for (uint32_t i = 0; i < extensionPropertiesCount; i++) {
				if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extensionProperties[i].extensionName) == 0) {
					enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
					char message[512];
					sprintf(message, "Using device extension: %s", extensionProperties[i].extensionName);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, message);
					continue;
				}
			}
			std::free(extensionProperties);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> uniqueQueueFamilies = {
				m_QueueFamilyIndices.presentIndex.value(),
				m_QueueFamilyIndices.graphicsIndex.value(),
				m_QueueFamilyIndices.transferIndex.value()
			};
			float queuePriority = 1.0f;
			for (auto family : uniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.pNext = NULL;
				queueCreateInfo.flags = 0;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.queueFamilyIndex = family;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceFeatures.fillModeNonSolid = VK_TRUE;
			VkDeviceCreateInfo deviceCreateInfo = {};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.pNext = NULL;
			deviceCreateInfo.flags = 0;
			deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceCreateInfo.enabledLayerCount = 0;
			deviceCreateInfo.ppEnabledLayerNames = NULL;
			deviceCreateInfo.enabledExtensionCount = enabledExtensions.size();
			deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

			result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, NULL, &m_Device);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create logical device.");
				return -1;
			}

			vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.presentIndex.value(), 0, &m_PresentQueue);
			vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.graphicsIndex.value(), 0, &m_GraphicsQueue);
			vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.transferIndex.value(), 0, &m_TransferQueue);
		}
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &m_SwapcahinSupportInfo.surfaceCapabilities);
			uint32_t surfaceFormatCount;
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, NULL);
			if (surfaceFormatCount != 0) {
				m_SwapcahinSupportInfo.surfaceFormats.resize(surfaceFormatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice,
													 m_Surface, &surfaceFormatCount,
													 m_SwapcahinSupportInfo.surfaceFormats.data());
			}
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, NULL);

			if (presentModeCount != 0) {
				m_SwapcahinSupportInfo.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice,
														  m_Surface,
														  &presentModeCount,
														  m_SwapcahinSupportInfo.presentModes.data());
			}

			if (m_SwapcahinSupportInfo.surfaceFormats.empty() || m_SwapcahinSupportInfo.presentModes.empty()) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Swapchain not adequate.");
				return -1;
			}

			VkSurfaceFormatKHR format = ChooseSwapchainSurfaceFormat(m_SwapcahinSupportInfo.surfaceFormats);
			VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(m_SwapcahinSupportInfo.presentModes);
			uint32_t imageCount = m_Capabilites.maxFramesInFlight;

			if (m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount > 0 &&
				imageCount > m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount)
			{
				imageCount = m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount;
				m_Capabilites.maxFramesInFlight = m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
			swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCreateInfo.pNext = NULL;
			swapchainCreateInfo.flags = 0;
			swapchainCreateInfo.surface = m_Surface;
			swapchainCreateInfo.imageExtent = ChooseSwapchainExtent(m_SwapcahinSupportInfo.surfaceCapabilities, m_Window);
			swapchainCreateInfo.imageFormat = format.format;
			swapchainCreateInfo.imageColorSpace = format.colorSpace;
			swapchainCreateInfo.presentMode = presentMode;
			swapchainCreateInfo.minImageCount = imageCount;
			swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
			swapchainCreateInfo.imageArrayLayers = 1;
			swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			uint32_t queueIndices[] = {
				m_QueueFamilyIndices.graphicsIndex.value(),
				m_QueueFamilyIndices.presentIndex.value()
			};
			if (m_QueueFamilyIndices.graphicsIndex.value() != m_QueueFamilyIndices.presentIndex.value()) {
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapchainCreateInfo.queueFamilyIndexCount = 2;
				swapchainCreateInfo.pQueueFamilyIndices = queueIndices;
			} else {
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				swapchainCreateInfo.queueFamilyIndexCount = 0;
				swapchainCreateInfo.pQueueFamilyIndices = NULL;
			}
			swapchainCreateInfo.preTransform = m_SwapcahinSupportInfo.surfaceCapabilities.currentTransform;
			swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainCreateInfo.clipped = VK_TRUE;

			result = vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, NULL, &m_Swapchain);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create swapchain.");
				return -1;
			}

			m_SwapchainExtent = swapchainCreateInfo.imageExtent;
			m_SwapchainImageFormat = swapchainCreateInfo.imageFormat;

			vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, NULL);
			m_SwapchainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

			m_SwapchainImageCount = imageCount;

			for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
				VkImageViewCreateInfo imageViewCreateInfo = {};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.pNext = NULL;
				imageViewCreateInfo.flags = 0;
				imageViewCreateInfo.format = m_SwapchainImageFormat;
				imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCreateInfo.subresourceRange.layerCount = 1;
				imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				imageViewCreateInfo.subresourceRange.levelCount = 1;
				imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
				imageViewCreateInfo.image = m_SwapchainImages[i];

				VkImageView imageView;
				result = vkCreateImageView(m_Device, &imageViewCreateInfo, NULL, &imageView);
				if (result != VK_SUCCESS) {
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_DEBUG, "Failed to create image views for the swapchain.");
					return -1;
				}
				m_SwapchainImageViews.push_back(imageView);
			}
			m_RecreateSwapchain = false;
		}
		{
			VkAttachmentDescription attachmentDescription = {};
			attachmentDescription.flags = 0;
			attachmentDescription.format = m_SwapchainImageFormat;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference attachmentReference = {};
			attachmentReference.attachment = 0;
			attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpassDescription = {};
			subpassDescription.flags = 0;
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &attachmentReference;
			subpassDescription.inputAttachmentCount = 0;
			subpassDescription.pInputAttachments = NULL;
			subpassDescription.preserveAttachmentCount = 0;
			subpassDescription.pPreserveAttachments = NULL;
			subpassDescription.pResolveAttachments = NULL;

			VkSubpassDependency subpassDependency = {};
			subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependency.dstSubpass = 0;
			subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependency.srcAccessMask = 0;
			subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo renderPassCreateInfo = {};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.pNext = NULL;
			renderPassCreateInfo.flags = 0;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &attachmentDescription;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpassDescription;
			renderPassCreateInfo.dependencyCount = 1;
			renderPassCreateInfo.pDependencies = &subpassDependency;

			result = vkCreateRenderPass(m_Device, &renderPassCreateInfo, NULL, &m_RenderPass);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create render pass.");
				return -1;
			}
		}
		{
			// TODO: Abstract file operations.
			const char* vertexShaderFilePath = "/home/chloe/dev/cpp/CeeEngineRewrite/CeeEditor/res/shaders/vertex.spv";
			const char* fragmentShaderFilePath = "/home/chloe/dev/cpp/CeeEngineRewrite/CeeEditor/res/shaders/fragment.spv";
			uint32_t vertexShaderCodeSize;
			std::vector<uint8_t> vertexShaderCode;
			uint32_t fragmentShaderCodeSize;
			std::vector<uint8_t> fragmentShaderCode;

			std::ifstream vertexShaderFile(vertexShaderFilePath, std::ios::ate | std::ios::binary);
			if (!vertexShaderFile.is_open()) {
				char message[FILENAME_MAX + 128];
				sprintf(message, "Failed to open vertex shader file \"%s\".", vertexShaderFilePath);
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
				return -1;
			}
			vertexShaderCodeSize = vertexShaderFile.tellg();
			vertexShaderCode.resize(vertexShaderCodeSize);
			vertexShaderFile.seekg(0);
			vertexShaderFile.read((char*)vertexShaderCode.data(), vertexShaderCodeSize);
			vertexShaderFile.close();

			VkShaderModule vertexShaderModule = this->CreateShaderModule(m_Device, vertexShaderCode);
			vertexShaderCode.clear();
			vertexShaderCode.shrink_to_fit();

			std::ifstream fragmentShaderFile(fragmentShaderFilePath, std::ios::ate | std::ios::binary);
			if (!fragmentShaderFile.is_open()) {
				char message[FILENAME_MAX + 128];
				sprintf(message, "Failed to open fragment shader file \"%s\".", fragmentShaderFilePath);
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
				return -1;
			}
			fragmentShaderCodeSize = fragmentShaderFile.tellg();
			fragmentShaderCode.resize(fragmentShaderCodeSize);
			fragmentShaderFile.seekg(0);
			fragmentShaderFile.read((char*)fragmentShaderCode.data(), fragmentShaderCodeSize);
			fragmentShaderFile.close();

			VkShaderModule fragmentShaderModule = this->CreateShaderModule(m_Device, fragmentShaderCode);
			fragmentShaderCode.clear();
			fragmentShaderCode.shrink_to_fit();
			VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
			vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexShaderStageCreateInfo.pNext = NULL;
			vertexShaderStageCreateInfo.flags = 0;
			vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexShaderStageCreateInfo.module = vertexShaderModule;
			vertexShaderStageCreateInfo.pName = "main";
			vertexShaderStageCreateInfo.pSpecializationInfo = NULL;

			VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
			fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentShaderStageCreateInfo.pNext = NULL;
			fragmentShaderStageCreateInfo.flags = 0;
			fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentShaderStageCreateInfo.module = fragmentShaderModule;
			fragmentShaderStageCreateInfo.pName = "main";
			fragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

			VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
				vertexShaderStageCreateInfo,
				fragmentShaderStageCreateInfo
			};

			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
			VkVertexInputAttributeDescription vertexInputAttribute = {};
			vertexInputAttribute.binding = 0;
			vertexInputAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			vertexInputAttribute.location = 0;
			vertexInputAttribute.offset = 0;
			vertexInputAttributes.push_back(vertexInputAttribute);
			vertexInputAttribute.location = 1;
			vertexInputAttribute.offset = 16;
			vertexInputAttributes.push_back(vertexInputAttribute);
			vertexInputAttribute.format = VK_FORMAT_R32G32_SFLOAT;
			vertexInputAttribute.location = 2;
			vertexInputAttribute.offset = 32;
			vertexInputAttributes.push_back(vertexInputAttribute);
			std::vector<VkVertexInputBindingDescription> vertexInputBindings;
			VkVertexInputBindingDescription vertexInputBinding = {};
			vertexInputBinding.binding = 0;
			vertexInputBinding.stride = 40;
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexInputBindings.push_back(vertexInputBinding);

			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.pNext = NULL;
			vertexInputStateCreateInfo.flags = 0;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributes.size();
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputBindings.size();
			vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindings.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
			inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyStateCreateInfo.pNext = NULL;
			inputAssemblyStateCreateInfo.flags = 0;
			inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

			std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pNext = NULL;
			dynamicStateCreateInfo.flags = 0;
			dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
			dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

			VkViewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = m_SwapchainExtent.width;
			viewport.height = m_SwapchainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			VkRect2D scissor;
			scissor.extent = m_SwapchainExtent;
			scissor.offset = { 0, 0 };

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.pNext = NULL;
			viewportStateCreateInfo.flags = 0;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = &viewport;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = &scissor;

			VkPipelineRasterizationStateCreateInfo mainRasterizationStateCreateInfo = {};
			mainRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			mainRasterizationStateCreateInfo.pNext = NULL;
			mainRasterizationStateCreateInfo.flags = 0;
			mainRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
			mainRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
			mainRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			mainRasterizationStateCreateInfo.lineWidth = 1.0f;
			mainRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			mainRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
			mainRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
			mainRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
			mainRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;

			VkPipelineRasterizationStateCreateInfo lineRasterizationStateCreateInfo = {};
			lineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			lineRasterizationStateCreateInfo.pNext = NULL;
			lineRasterizationStateCreateInfo.flags = 0;
			lineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
			lineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
			lineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
			lineRasterizationStateCreateInfo.lineWidth = 1.0f;
			lineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			lineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
			lineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
			lineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
			lineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;

			VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
			multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleStateCreateInfo.pNext = NULL;
			multisampleStateCreateInfo.flags = 0;
			multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
			multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
			multisampleStateCreateInfo.minSampleShading = 1.0f;
			multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleStateCreateInfo.pSampleMask = NULL;

			VkPipelineColorBlendAttachmentState colorBlendState;
			colorBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendState.blendEnable = VK_FALSE;
			colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
			colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

			VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
			colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendStateCreateInfo.pNext = NULL;
			colorBlendStateCreateInfo.flags = 0;
			colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
			colorBlendStateCreateInfo.attachmentCount = 1;
			colorBlendStateCreateInfo.pAttachments = &colorBlendState;
			colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
			colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
			colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
			colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.pNext = NULL;
			pipelineLayoutCreateInfo.flags = 0;
			pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pipelineLayoutCreateInfo.pPushConstantRanges = NULL;
			pipelineLayoutCreateInfo.setLayoutCount = 0;
			pipelineLayoutCreateInfo.pSetLayouts = NULL;

			result = vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, NULL, &m_PipelineLayout);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create pipeline layout.");
				return -1;

			}

			VkGraphicsPipelineCreateInfo mainPipelineCreateInfo = {};
			mainPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			mainPipelineCreateInfo.pNext = NULL;
			mainPipelineCreateInfo.flags = 0;
			mainPipelineCreateInfo.layout = m_PipelineLayout;
			mainPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			mainPipelineCreateInfo.basePipelineIndex = 0;
			mainPipelineCreateInfo.renderPass = m_RenderPass;
			mainPipelineCreateInfo.subpass = 0;
			mainPipelineCreateInfo.stageCount = 2;
			mainPipelineCreateInfo.pStages = shaderStageCreateInfos;
			mainPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
			mainPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
			mainPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			mainPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			mainPipelineCreateInfo.pRasterizationState = &mainRasterizationStateCreateInfo;
			mainPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
			mainPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
			mainPipelineCreateInfo.pDepthStencilState = NULL;
			mainPipelineCreateInfo.pTessellationState = NULL;

			VkGraphicsPipelineCreateInfo linePipelineCreateInfo = {};
			linePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			linePipelineCreateInfo.pNext = NULL;
			linePipelineCreateInfo.flags = 0;
			linePipelineCreateInfo.layout = m_PipelineLayout;
			linePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			linePipelineCreateInfo.basePipelineIndex = 0;
			linePipelineCreateInfo.renderPass = m_RenderPass;
			linePipelineCreateInfo.subpass = 0;
			linePipelineCreateInfo.stageCount = 2;
			linePipelineCreateInfo.pStages = shaderStageCreateInfos;
			linePipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
			linePipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
			linePipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			linePipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			linePipelineCreateInfo.pRasterizationState = &lineRasterizationStateCreateInfo;
			linePipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
			linePipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
			linePipelineCreateInfo.pDepthStencilState = NULL;
			linePipelineCreateInfo.pTessellationState = NULL;

			std::vector<uint8_t> pipelineCacheData = this->AttemptPipelineCacheRead(
				"/home/chloe/dev/cpp/CeeEngineRewrite/CeeEditor/res/cache/pipeline.cache");

			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			pipelineCacheCreateInfo.pNext = NULL;
			pipelineCacheCreateInfo.flags = 0;
			pipelineCacheCreateInfo.initialDataSize = pipelineCacheData.size();
			pipelineCacheCreateInfo.pInitialData = pipelineCacheData.data();

			vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, NULL, &m_PipelineCache);

			VkGraphicsPipelineCreateInfo pipelineCreateInfos[] = {
				mainPipelineCreateInfo,
				linePipelineCreateInfo
			};
			VkPipeline pipelines[2] = {
				VK_NULL_HANDLE,
				VK_NULL_HANDLE
			};
			result = vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 2, pipelineCreateInfos, NULL, pipelines);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to graphics create pipelines.");
				return -1;
			}
			m_MainPipeline = pipelines[0];
			m_LinePipeline = pipelines[1];

			size_t pipelineCacheDataSize;
			vkGetPipelineCacheData(m_Device, m_PipelineCache, &pipelineCacheDataSize, NULL);
			pipelineCacheData.resize(pipelineCacheDataSize);
			vkGetPipelineCacheData(m_Device, m_PipelineCache, &pipelineCacheDataSize, pipelineCacheData.data());

			this->PipelineCacheWrite(
				"/home/chloe/dev/cpp/CeeEngineRewrite/CeeEditor/res/cache/pipeline.cache",
				pipelineCacheData);

			vkDestroyShaderModule(m_Device, vertexShaderModule, NULL);
			vkDestroyShaderModule(m_Device, fragmentShaderModule, NULL);
		}
		{
			for (uint32_t i = 0; i < m_SwapchainImageCount; i ++) {
				VkImageView attachments[] = {
					m_SwapchainImageViews[i]
				};
				VkFramebufferCreateInfo framebufferCreateInfo = {};
				framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCreateInfo.pNext = NULL;
				framebufferCreateInfo.flags = 0;
				framebufferCreateInfo.renderPass = m_RenderPass;
				framebufferCreateInfo.layers = 1;
				framebufferCreateInfo.attachmentCount = 1;
				framebufferCreateInfo.pAttachments = attachments;
				framebufferCreateInfo.width = m_SwapchainExtent.width;
				framebufferCreateInfo.height = m_SwapchainExtent.height;

				VkFramebuffer framebuffer;
				result = vkCreateFramebuffer(m_Device, &framebufferCreateInfo, NULL, &framebuffer);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create framebuffer %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_Framebuffers.push_back(framebuffer);
			}
		}
		{
			VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolCreateInfo.pNext = NULL;
			cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			cmdPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsIndex.value();

			result = vkCreateCommandPool(m_Device, &cmdPoolCreateInfo, NULL, &m_GraphicsCmdPool);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create main command pool.");
				return -1;
			}

			cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			result = vkCreateCommandPool(m_Device, &cmdPoolCreateInfo, NULL, &m_TransferCmdPool);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create transfer command pool.");
				return -1;
			}

			VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
			cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferAllocateInfo.pNext = NULL;
			cmdBufferAllocateInfo.commandPool = m_GraphicsCmdPool;
			cmdBufferAllocateInfo.commandBufferCount = m_Capabilites.maxFramesInFlight;
			cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			m_DrawCmdBuffers.resize(m_SwapchainImageCount);
			result = vkAllocateCommandBuffers(m_Device, &cmdBufferAllocateInfo, m_DrawCmdBuffers.data());
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to allocate command buffers for draw commands.");
				return -1;
			}
		}
		{
			m_TransferFinishedSemaphores.reserve(m_Capabilites.maxFramesInFlight);
			m_ImageAvailableSemaphores.reserve(m_Capabilites.maxFramesInFlight);
			m_RenderFinishedSemaphores.reserve(m_Capabilites.maxFramesInFlight);
			m_InFlightFences.reserve(m_Capabilites.maxFramesInFlight);
			m_TransferFinishedFences.reserve(m_Capabilites.maxFramesInFlight);
			for (uint32_t i = 0; i < m_Capabilites.maxFramesInFlight; i++) {
				VkSemaphoreCreateInfo semaphoreCreateInfo = {};
				semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				semaphoreCreateInfo.pNext = NULL;
				semaphoreCreateInfo.flags = 0;

				VkSemaphore semaphore = VK_NULL_HANDLE;
				result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, NULL, &semaphore);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create transfer finished semaphore %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_TransferFinishedSemaphores.push_back(semaphore);

				semaphore = VK_NULL_HANDLE;
				result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, NULL, &semaphore);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create image available semaphore %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_ImageAvailableSemaphores.push_back(semaphore);

				semaphore = VK_NULL_HANDLE;
				result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, NULL, &semaphore);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to render finished semaphore %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_RenderFinishedSemaphores.push_back(semaphore);

				VkFenceCreateInfo fenceCreateInfo = {};
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.pNext = NULL;
				fenceCreateInfo.flags = 0;

				VkFence fence = VK_NULL_HANDLE;
				result = vkCreateFence(m_Device, &fenceCreateInfo, NULL, &fence);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create transfer finished fence %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_TransferFinishedFences.push_back(fence);

				fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				fence = VK_NULL_HANDLE;
				result = vkCreateFence(m_Device, &fenceCreateInfo, NULL, &fence);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create in flight fence %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_InFlightFences.push_back(fence);
			}
		}

		{
			uint32_t indexBufferSize;
			indexBufferSize = m_Capabilites.maxIndices - (m_Capabilites.maxIndices % 6);
			m_Capabilites.maxIndices = indexBufferSize;

			uint32_t vertexBufferSize;
			vertexBufferSize = (indexBufferSize * 4) / 6;

			indexBufferSize *= sizeof(uint32_t);
			vertexBufferSize *= sizeof(uint32_t);

			VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {};
			VkMemoryRequirements bufferMemoryRequirements = {};
			VkMemoryAllocateInfo memoryAllocateInfo = {};
			vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &deviceMemoryProperties);

			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.pNext = NULL;
			bufferCreateInfo.flags = 0;
			bufferCreateInfo.size = indexBufferSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			bufferCreateInfo.queueFamilyIndexCount = 0;
			bufferCreateInfo.pQueueFamilyIndices = NULL;

			result = vkCreateBuffer(m_Device, &bufferCreateInfo, NULL, &m_IndexBuffer);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to crate index buffer.");
				return -1;
			}

			vkGetBufferMemoryRequirements(m_Device, m_IndexBuffer, &bufferMemoryRequirements);
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocateInfo.pNext = NULL;
			memoryAllocateInfo.allocationSize = indexBufferSize;
			memoryAllocateInfo.memoryTypeIndex = this->ChooseMemoryType(
				bufferMemoryRequirements.memoryTypeBits,
				deviceMemoryProperties,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
			result = vkAllocateMemory(m_Device, &memoryAllocateInfo, NULL, &m_IndexBufferDeviceMemory);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to allocate memory for index buffer.");
				return -1;
			}

			result = vkBindBufferMemory(m_Device, m_IndexBuffer, m_IndexBufferDeviceMemory, 0);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to bind index buffer memory.");
				return -1;
			}

			VkBuffer indexStagingBuffer;
			VkDeviceMemory indexStagingBufferMemory;

			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			result = vkCreateBuffer(m_Device, &bufferCreateInfo, NULL, &indexStagingBuffer);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create index staging buffer.");
				return -1;
			}

			memoryAllocateInfo.memoryTypeIndex = this->ChooseMemoryType(
				bufferMemoryRequirements.memoryTypeBits,
				deviceMemoryProperties,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
			result = vkAllocateMemory(m_Device, &memoryAllocateInfo, NULL, &indexStagingBufferMemory);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to allocate index staging buffer memory.");
				return -1;
			}

			result = vkBindBufferMemory(m_Device, indexStagingBuffer, indexStagingBufferMemory, 0);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to bind index staging buffer memory.");
			}

			void* stagingBufferMapedAddress;
			result = vkMapMemory(m_Device,
								 indexStagingBufferMemory,
								 0, VK_WHOLE_SIZE,
								 0, &stagingBufferMapedAddress);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to map memory for index staging buffer.");
				return -1;
			}

			uint32_t index = 0;
			uint32_t *indices = (uint32_t*)std::calloc(m_Capabilites.maxIndices, sizeof(uint32_t));
			for (uint32_t i = 0; i < m_Capabilites.maxIndices; i += 6) {
				indices[i + 0] = index + 0;
				indices[i + 1] = index + 1;
				indices[i + 2] = index + 2;

				indices[i + 3] = index + 2;
				indices[i + 4] = index + 3;
				indices[i + 5] = index + 0;

				index += 4;
			}
			memcpy(stagingBufferMapedAddress, indices, memoryAllocateInfo.allocationSize);
			vkUnmapMemory(m_Device, indexStagingBufferMemory);

			VkCommandBuffer transferBuffer;
			VkCommandBufferAllocateInfo transferBufferAllocateInfo = {};
			transferBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			transferBufferAllocateInfo.pNext = NULL;
			transferBufferAllocateInfo.commandBufferCount = 1;
			transferBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			transferBufferAllocateInfo.commandPool = m_TransferCmdPool;

			result = vkAllocateCommandBuffers(m_Device, &transferBufferAllocateInfo, &transferBuffer);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to allocate command buffer for index buffer transfer");
				return -1;
			}

			VkCommandBufferBeginInfo transferBeginInfo = {};
			transferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			transferBeginInfo.pNext = NULL;
			transferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			transferBeginInfo.pInheritanceInfo = NULL;

			result = vkBeginCommandBuffer(transferBuffer, &transferBeginInfo);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to begin copy command buffer for index buffer initialisation.");
				return -1;
			}

			VkBufferCopy copyRegion;
			copyRegion.srcOffset = 0;
			copyRegion.size = indexBufferSize;
			copyRegion.dstOffset = 0;
			vkCmdCopyBuffer(transferBuffer, indexStagingBuffer, m_IndexBuffer, 1, &copyRegion);

			result = vkEndCommandBuffer(transferBuffer);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to end copy command buffer for index buffer initialisation.");
				return -1;
			}

			VkSubmitInfo transferSubmitInfo = {};
			transferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			transferSubmitInfo.pNext = NULL;
			transferSubmitInfo.commandBufferCount = 1;
			transferSubmitInfo.pCommandBuffers = &transferBuffer;
			transferSubmitInfo.signalSemaphoreCount = 0;
			transferSubmitInfo.pSignalSemaphores = NULL;
			transferSubmitInfo.waitSemaphoreCount = 0;
			transferSubmitInfo.pWaitSemaphores = NULL;
			transferSubmitInfo.pWaitDstStageMask = NULL;

			result = vkQueueSubmit(m_TransferQueue, 1, &transferSubmitInfo, m_TransferFinishedFences[0]);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to submit copy command buffer for index buffer initialisation.");
				return -1;
			}

			VkFence waitFences[] = { m_TransferFinishedFences[0] };
			result = vkWaitForFences(m_Device, 1, waitFences, VK_TRUE, UINT64_MAX);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to wait for transfer fence.");
			}
			result = vkResetFences(m_Device, 1, waitFences);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to reset transfer fence.");
			}

			std::free(indices);

			vkFreeMemory(m_Device, indexStagingBufferMemory, NULL);
			vkDestroyBuffer(m_Device, indexStagingBuffer, NULL);

			for (uint32_t i = 0; i < m_Capabilites.maxFramesInFlight; i++) {
				bufferCreateInfo.size = vertexBufferSize;
				bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

				VkBuffer buffer;
				result = vkCreateBuffer(m_Device, &bufferCreateInfo, NULL, &buffer);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create vertex buffer %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_VertexBuffers.push_back(buffer);

				bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				result = vkCreateBuffer(m_Device, &bufferCreateInfo, NULL, &buffer);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create staging buffer %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_StagingBuffers.push_back(buffer);
			}
			vkGetBufferMemoryRequirements(m_Device, m_VertexBuffers[0], &bufferMemoryRequirements);

			memoryAllocateInfo.allocationSize = vertexBufferSize * m_Capabilites.maxFramesInFlight;
			memoryAllocateInfo.memoryTypeIndex = this->ChooseMemoryType(
				bufferMemoryRequirements.memoryTypeBits,
				deviceMemoryProperties,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

			result = vkAllocateMemory(m_Device, &memoryAllocateInfo, NULL, &m_VertexBufferDeviceMemory);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to allocate memory for vertex buffer.");
				return -1;
			}

			vkGetBufferMemoryRequirements(m_Device, m_StagingBuffers[0], &bufferMemoryRequirements);
			memoryAllocateInfo.allocationSize = vertexBufferSize;
			memoryAllocateInfo.memoryTypeIndex = this->ChooseMemoryType(
				bufferMemoryRequirements.memoryTypeBits,
				deviceMemoryProperties,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
			for (uint32_t i = 0; i < m_StagingBuffers.size(); i++) {
				VkDeviceMemory memory;
				result = vkAllocateMemory(m_Device, &memoryAllocateInfo, NULL, &memory);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to allocate memory for vertex buffer %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
				m_StagingBufferDeviceMemorys.push_back(memory);

				result = vkBindBufferMemory(m_Device, m_StagingBuffers[i], m_StagingBufferDeviceMemorys[i], 0);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to bind staging buffer %u to memory.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}

				void* mappedAddress;
				result = vkMapMemory(m_Device, m_StagingBufferDeviceMemorys[i], 0, VK_WHOLE_SIZE, 0, &mappedAddress);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to map memory for staging buffer %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				} else {
					m_StagingBufferMemoryAddresses.push_back(mappedAddress);
				}
			}

			for (uint32_t i = 0; i < m_VertexBuffers.size(); i++) {
				result = vkBindBufferMemory(m_Device,
											m_VertexBuffers[i],
											m_VertexBufferDeviceMemory,
											i * vertexBufferSize);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to bind vertex buffer %u to buffer memory.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return -1;
				}
			}
			transferBufferAllocateInfo = {};
			transferBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			transferBufferAllocateInfo.pNext = NULL;
			transferBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			transferBufferAllocateInfo.commandBufferCount = m_Capabilites.maxFramesInFlight;
			transferBufferAllocateInfo.commandPool = m_TransferCmdPool;

			m_TransferCmdBuffers.resize(m_Capabilites.maxFramesInFlight);
			result = vkAllocateCommandBuffers(m_Device, &transferBufferAllocateInfo, m_TransferCmdBuffers.data());
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to allocate command buffers for transfer operations.");
				return -1;
			}
		}

		return 0;
	}

	void Renderer::Shutdown()
	{
		m_Running.store(false, std::memory_order_relaxed);
		vkDeviceWaitIdle(m_Device);
		for (auto& memory : m_StagingBufferDeviceMemorys) {
			vkUnmapMemory(m_Device, memory);
			vkFreeMemory(m_Device, memory, NULL);
		}
		vkFreeMemory(m_Device, m_IndexBufferDeviceMemory, NULL);
		vkFreeMemory(m_Device, m_VertexBufferDeviceMemory, NULL);
		for (auto& buffer :m_StagingBuffers) {
			vkDestroyBuffer(m_Device, buffer, NULL);
		}
		for (auto& buffer : m_VertexBuffers) {
			vkDestroyBuffer(m_Device, buffer, NULL);
		}
		vkDestroyBuffer(m_Device, m_IndexBuffer, NULL);
		for (auto& fence : m_InFlightFences) {
			vkDestroyFence(m_Device, fence, NULL);
		}
		for (auto& fence : m_TransferFinishedFences) {
			vkDestroyFence(m_Device, fence, NULL);
		}
		for (auto& semaphore : m_RenderFinishedSemaphores) {
			vkDestroySemaphore(m_Device, semaphore, NULL);
		}
		for (auto& semaphore : m_ImageAvailableSemaphores) {
			vkDestroySemaphore(m_Device, semaphore, NULL);
		}
		for (auto& semaphore : m_TransferFinishedSemaphores) {
			vkDestroySemaphore(m_Device, semaphore, NULL);
		}
		vkFreeCommandBuffers(m_Device, m_TransferCmdPool, m_TransferCmdBuffers.size(), m_TransferCmdBuffers.data());
		vkFreeCommandBuffers(m_Device, m_GraphicsCmdPool, m_DrawCmdBuffers.size(), m_DrawCmdBuffers.data());
		vkDestroyCommandPool(m_Device, m_TransferCmdPool, NULL);
		vkDestroyCommandPool(m_Device, m_GraphicsCmdPool, NULL);
		for (auto& framebuffer : m_Framebuffers) {
			vkDestroyFramebuffer(m_Device, framebuffer, NULL);
		}
		vkDestroyPipeline(m_Device, m_LinePipeline, NULL);
		vkDestroyPipeline(m_Device, m_MainPipeline, NULL);
		vkDestroyPipelineCache(m_Device, m_PipelineCache, NULL);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, NULL);
		vkDestroyRenderPass(m_Device, m_RenderPass, NULL);
		for (auto& imageView : m_SwapchainImageViews) {
			vkDestroyImageView(m_Device, imageView, NULL);
		}
		vkDestroySwapchainKHR(m_Device, m_Swapchain, NULL);
		vkDestroySurfaceKHR(m_Instance, m_Surface, NULL);
		vkDestroyDevice(m_Device, NULL);
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXTfn =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXTfn)
			vkDestroyDebugUtilsMessengerEXTfn(m_Instance, m_DebugMessenger, NULL);
		vkDestroyInstance(m_Instance, NULL);
	}

	void Renderer::Run() {
		m_Running.store(true, std::memory_order_relaxed);
		Timestep startTS, endTS, diffTS;
		GetTime(&startTS);
		while (m_Running.load(std::memory_order_relaxed)) {
			if (m_RecreateSwapchain) {
				InvalidateSwapchain();
			}

			m_FrameTimeStatsLock.lock();
			GetTime(&endTS);
			GetTimeStep(&startTS, &endTS, &diffTS);
			GetTime(&startTS);
			m_AverageFrameTime *= CEE_RENDERER_FRAME_TIME_COUNT;
			m_AverageFrameTime -= m_FrameTimes[m_FrameTimesIndex];
			m_FrameTimes[m_FrameTimesIndex] = diffTS.nsec + (diffTS.sec * 1000000000);
			m_AverageFrameTime += m_FrameTimes[m_FrameTimesIndex];
			m_AverageFrameTime /= CEE_RENDERER_FRAME_TIME_COUNT;
			if (++m_FrameTimesIndex == CEE_RENDERER_FRAME_TIME_COUNT)
				m_FrameTimesIndex = 0;
			m_FrameTimeStatsLock.unlock();

			VkFence waitFences[] = { m_InFlightFences[m_FrameIndex] };
			VkResult result = vkWaitForFences(m_Device, 1, waitFences, VK_TRUE, UINT64_MAX);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to wait for fence before recording relevent command buffer.");
			}

			result = vkAcquireNextImageKHR(m_Device,
										m_Swapchain,
										UINT64_MAX,
										m_ImageAvailableSemaphores[m_FrameIndex],
										VK_NULL_HANDLE,
										&m_ImageIndex);
			if (result == VK_SUBOPTIMAL_KHR) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_INFO, "Suboptimal KHR... Will recreate swapchain before next frame.");
				m_RecreateSwapchain = true;
			} else if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Out of date KHR... Recreating swapchain now...");
				this->InvalidateSwapchain();
			} else if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Swapchain could not aquire next image.");
				return;
			}

			VkFence resetFences[] = { m_InFlightFences[m_FrameIndex] };
			vkResetFences(m_Device, 1, resetFences);

			vkResetCommandBuffer(m_DrawCmdBuffers[m_FrameIndex], 0);
			VkCommandBufferBeginInfo transferBeginInfo = {};
			transferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			transferBeginInfo.pNext = NULL;
			transferBeginInfo.flags = 0;
			transferBeginInfo.pInheritanceInfo = NULL;

			m_Vertices.push_back({{ -0.5f,  0.5f, 0.0f, 1.0f },{ 0.2f, 0.0f, 0.8f, 1.0f }, { 0.0f, 0.0f }});
			m_Vertices.push_back({{  0.5f,  0.5f, 0.0f, 1.0f },{ 0.2f, 0.0f, 0.8f, 1.0f }, { 0.0f, 0.0f }});
			m_Vertices.push_back({{  0.5f, -0.5f, 0.0f, 1.0f },{ 0.2f, 0.0f, 0.8f, 1.0f }, { 0.0f, 0.0f }});
			m_Vertices.push_back({{ -0.5f, -0.5f, 0.0f, 1.0f },{ 0.2f, 0.0f, 0.8f, 1.0f }, { 0.0f, 0.0f }});

			if (!m_Vertices.empty()) {
				memcpy(m_StagingBufferMemoryAddresses[m_FrameIndex],
					m_Vertices.data(),
					m_Vertices.size() * sizeof(CeeEngineVertex));

				result = vkBeginCommandBuffer(m_TransferCmdBuffers[m_FrameIndex], &transferBeginInfo);
				if (result != VK_SUCCESS) {
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to begin transfer command buffer.");
				}

				VkBufferCopy copyInfo;
				copyInfo.srcOffset = 0;
				copyInfo.size = m_Vertices.size() * sizeof(CeeEngineVertex);
				copyInfo.dstOffset = 0;
				vkCmdCopyBuffer(m_TransferCmdBuffers[m_FrameIndex],
								m_StagingBuffers[m_FrameIndex],
								m_VertexBuffers[m_FrameIndex],
								1, &copyInfo);

				vkEndCommandBuffer(m_TransferCmdBuffers[m_FrameIndex]);
				VkSemaphore transferSubmitSignalSemaphores[] = { m_TransferFinishedSemaphores[m_FrameIndex] };
				VkCommandBuffer transferCommandBuffers[] = { m_TransferCmdBuffers[m_FrameIndex] };
				VkSubmitInfo transferSubmitInfo = {};
				transferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				transferSubmitInfo.pNext = NULL;
				transferSubmitInfo.commandBufferCount = 1;
				transferSubmitInfo.pCommandBuffers = transferCommandBuffers;
				transferSubmitInfo.signalSemaphoreCount = 1;
				transferSubmitInfo.pSignalSemaphores = transferSubmitSignalSemaphores;
				transferSubmitInfo.waitSemaphoreCount = 0;
				transferSubmitInfo.pWaitSemaphores = NULL;
				transferSubmitInfo.pWaitDstStageMask = NULL;

				result = vkQueueSubmit(m_TransferQueue, 1, &transferSubmitInfo, VK_NULL_HANDLE);
				if (result != VK_SUCCESS) {
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to submit transfer command buffer.");
				}
			}

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = NULL;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = NULL;

			result = vkBeginCommandBuffer(m_DrawCmdBuffers[m_FrameIndex], &beginInfo);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to begin command buffer.");
			}

			VkClearValue clearValue;
			clearValue.color.float32[0] = 0.0f;
			clearValue.color.float32[1] = 0.0f;
			clearValue.color.float32[2] = 0.0f;
			clearValue.color.float32[3] = 1.0f;
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = NULL;
			renderPassBeginInfo.framebuffer = m_Framebuffers[m_ImageIndex];
			renderPassBeginInfo.renderPass = m_RenderPass;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearValue;

			vkCmdBeginRenderPass(m_DrawCmdBuffers[m_FrameIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(m_DrawCmdBuffers[m_FrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_MainPipeline);

			VkViewport viewport;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = m_SwapchainExtent.width;
			viewport.height = m_SwapchainExtent.height;
			vkCmdSetViewport(m_DrawCmdBuffers[m_FrameIndex], 0, 1, &viewport);

			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = m_SwapchainExtent;
			vkCmdSetScissor(m_DrawCmdBuffers[m_FrameIndex], 0, 1, &scissor);

			vkCmdBindIndexBuffer(m_DrawCmdBuffers[m_FrameIndex], m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
			VkBuffer vertexBuffers[] = { m_VertexBuffers[m_FrameIndex] };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_DrawCmdBuffers[m_FrameIndex], 0, 1, vertexBuffers, offsets);
			vkCmdDrawIndexed(m_DrawCmdBuffers[m_FrameIndex], 6, 1, 0, 0, 0);

			vkCmdEndRenderPass(m_DrawCmdBuffers[m_FrameIndex]);

			result = vkEndCommandBuffer(m_DrawCmdBuffers[m_FrameIndex]);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to end command buffer.");
			}

			VkCommandBuffer drawCommandBuffers[] = { m_DrawCmdBuffers[m_FrameIndex] };
			VkSemaphore drawSignalSemaphores[] = { m_RenderFinishedSemaphores[m_FrameIndex] };
			VkSemaphore drawWaitSemaphores[] = {
				m_TransferFinishedSemaphores[m_FrameIndex],
				m_ImageAvailableSemaphores[m_FrameIndex]
			};
			VkPipelineStageFlags drawWaitSemaphoreStages[] = {
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT

			};

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = NULL;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = drawCommandBuffers;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = drawSignalSemaphores;
			submitInfo.waitSemaphoreCount = 2;
			submitInfo.pWaitSemaphores = drawWaitSemaphores;
			submitInfo.pWaitDstStageMask = drawWaitSemaphoreStages;

			result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_FrameIndex]);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to submit command buffer.");
			}

			VkSemaphore presentWaitSemaphores[] = { m_RenderFinishedSemaphores[m_FrameIndex] };
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = NULL;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = presentWaitSemaphores;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_Swapchain;
			presentInfo.pImageIndices = &m_ImageIndex;
			presentInfo.pResults = NULL;

			result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
			if (result == VK_SUBOPTIMAL_KHR) {
				m_RecreateSwapchain = true;
			} else if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to queue present.");
			}

			m_Vertices.clear();

			m_FrameIndex++;
			if (m_FrameIndex == m_Capabilites.maxFramesInFlight)
				m_FrameIndex = 0;
		}
	}

	void Renderer::Stop() {
		m_Running.store(false, std::memory_order_relaxed);
	}

	void Renderer::MessageHandler(Event& e) {
		switch (e.GetEventType()) {
			case EventType::WindowResize:
			{
				m_RecreateSwapchain = true;
			}
				break;

			default:
				break;
		}
	}

	void Renderer::InvalidateSwapchain() {
		VkSwapchainKHR oldSwapchain = m_Swapchain;

		VkResult result = vkWaitForFences(m_Device,
										  m_InFlightFences.size(),
										  m_InFlightFences.data(),
										  VK_TRUE, UINT64_MAX);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to wait for fences before recreating swapchain.");
		}

		for (auto& imageView : m_SwapchainImageViews) {
			vkDestroyImageView(m_Device, imageView, NULL);
		}
		for (auto& framebuffer : m_Framebuffers) {
			vkDestroyFramebuffer(m_Device, framebuffer, NULL);
		}

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &m_SwapcahinSupportInfo.surfaceCapabilities);
			uint32_t surfaceFormatCount;
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, NULL);
			if (surfaceFormatCount != 0) {
				m_SwapcahinSupportInfo.surfaceFormats.resize(surfaceFormatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice,
														m_Surface, &surfaceFormatCount,
														m_SwapcahinSupportInfo.surfaceFormats.data());
			}
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, NULL);

			if (presentModeCount != 0) {
				m_SwapcahinSupportInfo.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice,
															m_Surface,
															&presentModeCount,
															m_SwapcahinSupportInfo.presentModes.data());
			}

			if (m_SwapcahinSupportInfo.surfaceFormats.empty() || m_SwapcahinSupportInfo.presentModes.empty()) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Swapchain not adequate.");
				return;
			}

			VkSurfaceFormatKHR format = ChooseSwapchainSurfaceFormat(m_SwapcahinSupportInfo.surfaceFormats);
			VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(m_SwapcahinSupportInfo.presentModes);
			uint32_t imageCount = m_Capabilites.maxFramesInFlight;

			if (m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount > 0 &&
				imageCount > m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount)
			{
				imageCount = m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount;
				m_Capabilites.maxFramesInFlight = m_SwapcahinSupportInfo.surfaceCapabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
			swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCreateInfo.pNext = NULL;
			swapchainCreateInfo.flags = 0;
			swapchainCreateInfo.surface = m_Surface;
			swapchainCreateInfo.imageExtent = ChooseSwapchainExtent(m_SwapcahinSupportInfo.surfaceCapabilities, m_Window);
			swapchainCreateInfo.imageFormat = format.format;
			swapchainCreateInfo.imageColorSpace = format.colorSpace;
			swapchainCreateInfo.presentMode = presentMode;
			swapchainCreateInfo.minImageCount = imageCount;
			swapchainCreateInfo.oldSwapchain = oldSwapchain;
			swapchainCreateInfo.imageArrayLayers = 1;
			swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			uint32_t queueIndices[] = {
				m_QueueFamilyIndices.graphicsIndex.value(),
				m_QueueFamilyIndices.presentIndex.value()
			};
			if (m_QueueFamilyIndices.graphicsIndex.value() != m_QueueFamilyIndices.presentIndex.value()) {
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapchainCreateInfo.queueFamilyIndexCount = 2;
				swapchainCreateInfo.pQueueFamilyIndices = queueIndices;
			} else {
				swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				swapchainCreateInfo.queueFamilyIndexCount = 0;
				swapchainCreateInfo.pQueueFamilyIndices = NULL;
			}
			swapchainCreateInfo.preTransform = m_SwapcahinSupportInfo.surfaceCapabilities.currentTransform;
			swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapchainCreateInfo.clipped = VK_TRUE;

			result = vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, NULL, &m_Swapchain);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create swapchain.");
				return;
			}

			m_SwapchainExtent = swapchainCreateInfo.imageExtent;
			m_SwapchainImageFormat = swapchainCreateInfo.imageFormat;

			vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, NULL);
			m_SwapchainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

			m_SwapchainImageCount = imageCount;

			m_SwapchainImageViews.clear();
			for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
				VkImageViewCreateInfo imageViewCreateInfo = {};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.pNext = NULL;
				imageViewCreateInfo.flags = 0;
				imageViewCreateInfo.format = m_SwapchainImageFormat;
				imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCreateInfo.subresourceRange.layerCount = 1;
				imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				imageViewCreateInfo.subresourceRange.levelCount = 1;
				imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
				imageViewCreateInfo.image = m_SwapchainImages[i];

				VkImageView imageView;
				result = vkCreateImageView(m_Device, &imageViewCreateInfo, NULL, &imageView);
				if (result != VK_SUCCESS) {
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create image views for the swapchain.");
					return;
				}
				m_SwapchainImageViews.push_back(imageView);
			}

			m_Framebuffers.clear();
			for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
				VkImageView attachments[] = {
					m_SwapchainImageViews[i]
				};
				VkFramebufferCreateInfo framebufferCreateInfo = {};
				framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCreateInfo.pNext = NULL;
				framebufferCreateInfo.flags = 0;
				framebufferCreateInfo.renderPass = m_RenderPass;
				framebufferCreateInfo.layers = 1;
				framebufferCreateInfo.attachmentCount = 1;
				framebufferCreateInfo.pAttachments = attachments;
				framebufferCreateInfo.width = m_SwapchainExtent.width;
				framebufferCreateInfo.height = m_SwapchainExtent.height;

				VkFramebuffer framebuffer = VK_NULL_HANDLE;
				result = vkCreateFramebuffer(m_Device, &framebufferCreateInfo, NULL, &framebuffer);
				if (result != VK_SUCCESS) {
					char message[128];
					sprintf(message, "Failed to create framebuffer %u.", i);
					DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, message);
					return;
				}
				m_Framebuffers.push_back(framebuffer);
			}

			vkDestroySwapchainKHR(m_Device, oldSwapchain, NULL);
			m_RecreateSwapchain = false;
	}

	void Renderer::InvalidatePipeline() {

	}

	VkPhysicalDevice Renderer::ChoosePhysicalDevice(uint32_t physicalDeviceCount, VkPhysicalDevice* physicalDevices)
	{
		uint32_t *deviceRatings = (uint32_t*)alloca(physicalDeviceCount * sizeof(uint32_t));
		memset(deviceRatings, 0, physicalDeviceCount * sizeof(uint32_t));

		VkPhysicalDeviceFeatures deviceFeatures;
		VkPhysicalDeviceProperties deviceProperties;
		for (uint32_t i = 0; i < physicalDeviceCount; i++) {
			vkGetPhysicalDeviceFeatures(physicalDevices[i], &deviceFeatures);
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				deviceRatings[i] += 10000;
			deviceRatings[i] += deviceProperties.limits.maxDrawIndexedIndexValue;
			deviceRatings[i] += deviceProperties.limits.maxImageDimension2D;
			deviceRatings[i] += deviceProperties.limits.maxViewports;
			if (deviceFeatures.fullDrawIndexUint32)
				deviceRatings[i] += 1000;
			if (deviceFeatures.multiViewport)
				deviceRatings[i] += 1000;
		}

		uint32_t bestDeviceIndex = 0;
		for (uint32_t i = 0; i < physicalDeviceCount; i++) {
			if (deviceRatings[i] > deviceRatings[bestDeviceIndex]) {
				bestDeviceIndex = i;
			}
		}
		return physicalDevices[bestDeviceIndex];
	}

	VkSurfaceFormatKHR Renderer::ChooseSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableSurfaceFormats) {
		for (auto& format : availableSurfaceFormats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
				format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}
		return availableSurfaceFormats[0];
	}

	VkPresentModeKHR Renderer::ChooseSwapchainPresentMode(std::vector<VkPresentModeKHR> availablePresentModes) {
		for (auto& mode : availablePresentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Renderer::ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
											   std::shared_ptr<Window> window) {
		if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
			return surfaceCapabilities.currentExtent;
		}
		VkExtent2D extent;

		extent.width = std::clamp(window->GetWidth(),
								  surfaceCapabilities.minImageExtent.width,
								  surfaceCapabilities.maxImageExtent.width);
		extent.height = std::clamp(window->GetHeight(),
								  surfaceCapabilities.minImageExtent.height,
								  surfaceCapabilities.maxImageExtent.height);
		return extent;
	}

	uint32_t Renderer::ChooseMemoryType(uint32_t typeFilter,
										const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties,
										VkMemoryPropertyFlags properties) {
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
				(deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Unable to find suitable memory type.");
		return UINT32_MAX;
	}

	VkShaderModule Renderer::CreateShaderModule(VkDevice device, std::vector<uint8_t> shaderCode) {
		VkShaderModuleCreateInfo shaderModuleCreateInfo;
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.pNext = NULL;
		shaderModuleCreateInfo.flags = 0;
		shaderModuleCreateInfo.codeSize = shaderCode.size();
		shaderModuleCreateInfo.pCode = (uint32_t*)shaderCode.data();

		VkShaderModule shader;

		VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &shader);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_ERROR, "Failed to create shader module.");
			return VK_NULL_HANDLE;
		}
		return shader;
	}

	std::vector<uint8_t> Renderer::AttemptPipelineCacheRead(const std::string& filePath) {
		std::ifstream file(filePath, std::ios_base::binary | std::ios_base::ate);
		if (!file.is_open()) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to open pipeline cache file for reading.");
			return std::vector<uint8_t>();
		}

		size_t fileSize = file.tellg();
		file.seekg(0);
		std::vector<uint8_t> fileData;
		fileData.resize(fileSize);

		file.read((char*)fileData.data(), fileSize);
		file.close();

		return fileData;
	}

	void Renderer::PipelineCacheWrite(const std::string& filePath, const std::vector<uint8_t>& cacheData) {
		std::ofstream file(filePath, std::ios_base::binary | std::ios_base::trunc);
		if (!file.is_open()) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Failed to open pipeline cache file for writing.");
			return;
		}

		file.write((char*)cacheData.data(), cacheData.size());
		if (file.bad()) {
			DebugMessenger::PostDebugMessage(CEE_ERROR_SEVERITY_WARNING, "Error writing pipeline cache to file.");
		}
		file.close();
	}

}
