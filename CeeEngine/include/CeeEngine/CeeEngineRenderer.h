#ifndef CEE_ENGINE_RENDERER_H
#define CEE_ENGINE_RENDERER_H

#include "CeeEngine/CeeEngineWindow.h"
#include <CeeEngine/CeeEngineMessageBus.h>

#include "CeeEngine/CeeEnginePlatform.h"
#if defined(CEE_PLATFORM_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(CEE_PLATFORM_LINUX)
#define VK_USE_PLATFORM_XCB_KHR
#endif
#include <vulkan/vulkan.h>

#include <memory>
#include <optional>
#include <atomic>

#define CEE_RENDERER_FRAME_TIME_COUNT 100

namespace cee {
	struct CeeEngineVertex {
		float position[4];
		float color[4];
		float texCoords[2];
	};

	struct RendererCapabilities {
		const char* applicationName;
		uint32_t applicationVersion;
		uint32_t maxIndices;
		uint32_t maxFramesInFlight;
	};

	class Renderer {
	public:
		Renderer(MessageBus* messageBus, const RendererCapabilities& capabilities, std::shared_ptr< cee::Window > window);
		~Renderer();

		int Init();
		void Shutdown();

		void Run();
		void Stop();

		uint64_t GetAverageFrameTime() const {
			std::lock_guard lock(m_FrameTimeStatsLock);
			return m_AverageFrameTime;
		}
		uint64_t GetPreviousFrameTime() const {
			std::lock_guard lock(m_FrameTimeStatsLock);
			return m_FrameTimesIndex ? m_FrameTimes[m_FrameTimesIndex - 1] : m_FrameTimes[CEE_RENDERER_FRAME_TIME_COUNT - 1];
		}

	private:
		void MessageHandler(Event& e);
		void InvalidateSwapchain();
		void InvalidatePipeline();

	private:
		static VkPhysicalDevice ChoosePhysicalDevice(uint32_t physicalDeviceCount, VkPhysicalDevice* physicalDevices);
		static VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableSurfaceFormats);
		static VkPresentModeKHR ChooseSwapchainPresentMode(std::vector<VkPresentModeKHR> availablePresentModes);
		static VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, std::shared_ptr<Window> window);
		static uint32_t ChooseMemoryType(uint32_t typeFilter, const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties, VkMemoryPropertyFlags properties);
		static VkShaderModule CreateShaderModule(VkDevice device, std::vector<uint8_t> shaderCode);
		static std::vector<uint8_t> AttemptPipelineCacheRead(const std::string& filePath);
		static void PipelineCacheWrite(const std::string& filePath, const std::vector<uint8_t>& cacheData);

		static VkBool32 VulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT* messageData,
													 void* userData);

	private:
		MessageBus* m_MessageBus;
		RendererCapabilities m_Capabilites;

		std::atomic<bool> m_Running;

		std::shared_ptr<Window> m_Window;

		VkInstance m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		VkDevice m_Device;

		VkSurfaceKHR m_Surface;

		struct {
			std::optional<uint32_t> presentIndex;
			std::optional<uint32_t> graphicsIndex;
			std::optional<uint32_t> computeIndex;
			std::optional<uint32_t> transferIndex;
		} m_QueueFamilyIndices;

		struct {
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			std::vector<VkSurfaceFormatKHR> surfaceFormats;
			std::vector<VkPresentModeKHR> presentModes;
		} m_SwapcahinSupportInfo;

		VkFormat m_SwapchainImageFormat;

		VkSwapchainKHR m_Swapchain;
		bool m_RecreateSwapchain;
		VkExtent2D m_SwapchainExtent;
		uint32_t m_SwapchainImageCount;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		VkRenderPass m_RenderPass;
		VkPipelineLayout m_PipelineLayout;
		VkPipelineCache m_PipelineCache;
		VkPipeline m_MainPipeline;
		VkPipeline m_LinePipeline;

		std::vector<VkFramebuffer> m_Framebuffers;

		VkQueue m_PresentQueue;
		VkQueue m_GraphicsQueue;
		VkQueue m_TransferQueue;

		VkCommandPool m_GraphicsCmdPool;
		std::vector<VkCommandBuffer> m_DrawCmdBuffers;

		VkCommandPool m_TransferCmdPool;
		std::vector<VkCommandBuffer> m_TransferCmdBuffers;

		std::vector<VkSemaphore> m_TransferFinishedSemaphores;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_TransferFinishedFences;
		std::vector<VkFence> m_InFlightFences;

		std::vector<VkBuffer> m_VertexBuffers;
		VkDeviceMemory m_VertexBufferDeviceMemory;

		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferDeviceMemory;

		std::vector<VkBuffer> m_StagingBuffers;
		std::vector<VkDeviceMemory> m_StagingBufferDeviceMemorys;
		std::vector<void*> m_StagingBufferMemoryAddresses;

		std::vector<CeeEngineVertex> m_Vertices;

		uint32_t m_ImageIndex;
		uint32_t m_FrameIndex;

		friend VkBool32 vulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
									  VkDebugUtilsMessageTypeFlagsEXT messageType,
									  const VkDebugUtilsMessengerCallbackDataEXT* messageData,
									  void* userData);
		VkDebugUtilsMessengerEXT m_DebugMessenger;

		mutable std::mutex m_FrameTimeStatsLock;
		uint64_t m_FrameTimes[CEE_RENDERER_FRAME_TIME_COUNT];
		uint32_t m_FrameTimesIndex;
		uint64_t m_AverageFrameTime;
	};
}

#endif
