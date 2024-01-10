/* Decleration of old renderer.
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

#ifndef CEE_ENGINE_RENDERER_H
#define CEE_ENGINE_RENDERER_H

#include <CeeEngine/window.h>
#include <CeeEngine/messageBus.h>
#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/camera.h>
#include <CeeEngine/assert.h>

#include <CeeEngine/platform.h>
#include <CeeEngine/renderer/context.h>
#include <CeeEngine/renderer/buffers.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CEE_RENDERER_FRAME_TIME_COUNT 100

namespace cee {
	enum Format {
		FORMAT_UNDEFINED           = 0,

		FORMAT_R8_SRGB             = 1,
		FORMAT_R8G8_SRGB           = 2,
		FORMAT_R8G8B8_SRGB         = 3,
		FORMAT_R8G8B8A8_SRGB       = 4,

		FORMAT_R8_UNORM            = 5,
		FORMAT_R8G8_UNORM          = 6,
		FORMAT_R8G8B8_UNORM        = 7,
		FORMAT_R8G8B8A8_UNORM      = 8,

		FORMAT_R8_UINT             = 9,
		FORMAT_R8G8_UINT           = 10,
		FORMAT_R8G8B8_UINT         = 11,
		FORMAT_R8G8B8A8_UINT       = 12,

		FORMAT_R16_SFLOAT          = 13,
		FORMAT_R16G16_SFLOAT       = 14,
		FORMAT_R16G16B16_SFLOAT    = 15,
		FORMAT_R16G16B16A16_SFLOAT = 16,

		FORMAT_R16_UNORM           = 17,
		FORMAT_R16G16_UNORM        = 18,
		FORMAT_R16G16B16_UNORM     = 19,
		FORMAT_R16G16B16A16_UNORM  = 20,

		FORMAT_R16_UINT            = 21,
		FORMAT_R16G16_UINT         = 22,
		FORMAT_R16G16B16_UINT      = 23,
		FORMAT_R16G16B16A16_UINT   = 24,

		FORMAT_R32_SFLOAT          = 25,
		FORMAT_R32G32_SFLOAT       = 26,
		FORMAT_R32G32B32_SFLOAT    = 27,
		FORMAT_R32G32B32A32_SFLOAT = 28,

		FORMAT_R32_UINT            = 29,
		FORMAT_R32G32_UINT         = 30,
		FORMAT_R32G32B32_UINT      = 31,
		FORMAT_R32G32B32A32_UINT   = 32,

		FORMAT_DEPTH    = 128
	};

	#define BIT(x) (1 << x)
	enum PipelineFlagsBits
	{
		RENDERER_PIPELINE_FLAG_3D      = BIT(0),
		RENDERER_PIPELINE_FLAG_QUAD    = BIT(1),
		RENDERER_PIPELINE_BASIC        = BIT(2),
		RENDERER_PIPELINE_FILL         = BIT(3),
		RENDERER_PIPELINE_RESERVED1    = BIT(4),
		RENDERER_PIPELINE_RESERVED2    = BIT(5),
		RENDERER_PIPELINE_RESERVED3    = BIT(6),
		RENDERER_PIPELINE_RESERVED4    = BIT(7)
	};
	typedef uint32_t PipelineFlags;

	enum ShaderStageFlagsBits
	{
		RENDERER_SHADER_STAGE_UNKNOWN        = BIT(0),
		RENDERER_SHADER_STAGE_VERTEX         = BIT(1),
		RENDERER_SHADER_STAGE_TESSELATION    = BIT(2),
		RENDERER_SHADER_STAGE_GEOMETRY       = BIT(3),
		RENDERER_SHADER_STAGE_FRAGMENT       = BIT(4),
		RENDERER_SHADER_STAGE_COMPUTE        = BIT(5),
		RENDERER_SHADER_STAGE_ALL            = BIT(6)
	};
	typedef uint32_t ShaderStageFlags;

	enum RendererMode {
		RENDERER_MODE_UNKNOWN = 0,
		RENDERER_MODE_2D      = 1,
		RENDERER_MODE_3D      = 2
	};

	struct Vertex2D {
		glm::vec4 position;
		glm::vec4 color;
		glm::vec2 texCoords;
	};

	struct Vertex3D {
		glm::vec4 position;
		glm::vec3 normal;
		glm::vec4 color;
		glm::vec2 texCoords;
		uint32_t texIndex;
	};

	struct RendererCapabilities {
		const char* applicationName;
		uint32_t applicationVersion;
		uint32_t maxIndices;
		uint32_t maxFramesInFlight;

		RendererMode rendererMode;

		int32_t useRenderDocLayer;
	};

	class oldRenderer;

	class Sampler {
	public:
		Sampler();
		//Sampler(/* Spec? */);
		virtual ~Sampler();

		VkSampler Get() { return m_Sampler; }

	private:
		VkSampler m_Sampler;
	};

	enum DescriptorType {
		DESCRIPTOR_TYPE_UNKNOWN = 0,
		DESCRIPTOR_TYPE_COMBINED_IMAGE_SMAPLER = 1,
		DESCRIPTOR_TYPE_SMAPLER = 2,
		DESCRIPTOR_TYPE_IMAGE = 3,
		DESCRIPTOR_TYPE_UNIFORM_BUFFER = 4,
		DESCRIPTOR_TYPE_STORAGE_BUFFER = 5
	};

	struct Descriptor {
		VkDescriptorSetLayoutBinding binding;

		Descriptor& SetDescriptorCount(uint32_t count){ binding.descriptorCount = count; return *this; }
		Descriptor& SetDescriptorType(VkDescriptorType type) { binding.descriptorType = type; return *this; }
		Descriptor& SetDescriptorBinding(uint32_t binding) { this->binding.binding = binding; return *this; }
		Descriptor& SetDescriptorStage(VkPipelineStageFlags stage) { binding.stageFlags = stage; return *this; }
		Descriptor& SetImmutableSamplers(const VkSampler* immutableSamplers)
		{
			binding.pImmutableSamplers = immutableSamplers;
			return *this;
		}

		Descriptor() : binding({ .binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM,
			.descriptorCount = 0,
			.stageFlags = VK_PIPELINE_STAGE_NONE,
			.pImmutableSamplers = VK_NULL_HANDLE })
		{}

		Descriptor(uint32_t bindingNum, VkDescriptorType type, uint32_t count,
				   VkPipelineStageFlags stage, const VkSampler* immutableSamplers = VK_NULL_HANDLE)
		: binding({ .binding = bindingNum,
			.descriptorType = type,
			.descriptorCount = count,
			.stageFlags = stage,
			.pImmutableSamplers = immutableSamplers })
		{}
	};

	class DescriptorSet {
	public:
		DescriptorSet();
		DescriptorSet(std::initializer_list<Descriptor> descriptors);
		virtual ~DescriptorSet();

		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet(DescriptorSet&& other);

		DescriptorSet& operator=(const DescriptorSet&) = delete;
		DescriptorSet& operator=(DescriptorSet&& other);

	private:
		void FreeResources();
	private:
		bool m_Initialized;
		std::vector<Descriptor> m_Descriptors;
		VkDescriptorSet m_Set;
	};

	class DescriptorManager {
	public:
		DescriptorManager();
		virtual ~DescriptorManager();

		DescriptorManager(const DescriptorManager&) = delete;
		DescriptorManager(DescriptorManager&& other);

		DescriptorManager& operator=(const DescriptorManager&) = delete;
		DescriptorManager& operator=(DescriptorManager&& other);

		void Reserve();
		void AddSet(const DescriptorSet& set);
		void RemoveSet(const DescriptorSet& set);
		void Clear();

	private:
		void FreeResources();

	private:
		VkDescriptorPool m_Pool;
		VkDescriptorSetLayout m_Layout;
		std::vector<DescriptorSet> m_Sets;
	};

	enum class PrimitiveTopology {
		None = 0,
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
		TriangleFan
	};

	struct PipelineSpecification {
		PrimitiveTopology topology;

		std::string debugName;
	};

	class Pipeline {
	public:
		Pipeline();
		virtual ~Pipeline();

		Pipeline(const Pipeline&) = delete;
		Pipeline(Pipeline&& other);

		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline& operator=(Pipeline&& other);

	private:
		BufferLayout m_InputLayout;
		DescriptorManager m_DescriptorManager;
		VkPipelineLayout m_Layout;
		VkPipeline m_Pipeline;
	};

	class oldRenderer {
	public:
		oldRenderer(const RendererCapabilities& capabilities, std::shared_ptr<cee::Window> window);
		~oldRenderer();

		int Init();
		void Shutdown();

		// Sets member variable. Should be called before StartFrame().
		void Clear(const glm::vec4& clearColor);
		// Begins Command buffer, render pass, sets viewport and scissor and binds pipeline.
		int StartFrame();
		// Ends command buffer, submits commands, then submits present.
		int EndFrame();

		// Binds the vertex buffer and index buffer and called vulkans DrawIndexedInstanced().
		int Draw(IndexBuffer& indexBuffer, VertexBuffer& vertexBuffer, uint32_t indexCount);

		int UpdateCamera(Camera& camera);
		void UpdateSkybox(CubeMapBuffer& newSkybox);

		static oldRenderer* Get() { return s_Instance; }

		VkDevice GetDevice() { return m_Device; }
		VkPhysicalDeviceMemoryProperties GetDeviceMemoryProperties() const {
			return m_PhysicalDeviceMemoryProperties;
		}
		VkFormat GetDepthFormat() const { return m_DepthFormat; }
		VkFormat GetSwapchainFormat() const { return m_SwapchainImageFormat; }
		VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }

	private:
		void InvalidateSwapchain();
		void InvalidatePipeline();

	public:
		enum CommandQueueTypeBits
		{
			QUEUE_UNKNOWN  = 0,
			QUEUE_TRANSFER = 1,
			QUEUE_GRAPHICS = 2,
			QUEUE_COMPUTE  = 3
		};
		typedef uint8_t CommandQueueType;

		struct BakedCommandBuffer {
			VkCommandBuffer commandBuffer;
			CommandQueueType queueType;
			std::vector<VkSemaphore> signalSemaphores;
			std::vector<std::pair<VkPipelineStageFlags, VkSemaphore>> waitSemaphores;
		};

		struct UsedCommandBuffer {
			VkCommandBuffer commandBuffer;
			CommandQueueType queueType;
			uint32_t age;
		};

	public:
		VkResult ImmediateSubmit(std::function<void(VkCommandBuffer&)> fn, CommandQueueType queueType);
		VkResult QueueSubmit(std::function<void(VkCommandBuffer&)> fn, CommandQueueType queueType);
		VkResult FlushQueuedSubmits();
		VkCommandBuffer StartCommandBuffer(CommandQueueType queueType);
		VkResult SubmitCommandBufferNow(VkCommandBuffer commandBuffer, CommandQueueType queueType);
		void QueueCommandBuffer(VkCommandBuffer commandBuffer, CommandQueueType queueType);
		static VkPhysicalDevice ChoosePhysicalDevice(uint32_t physicalDeviceCount, VkPhysicalDevice* physicalDevices);
		static VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(std::vector<VkSurfaceFormatKHR> availableSurfaceFormats);
		static VkPresentModeKHR ChooseSwapchainPresentMode(std::vector<VkPresentModeKHR> availablePresentModes);
		static VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
												std::shared_ptr<Window> window);
		static uint32_t ChooseMemoryType(uint32_t typeFilter,
										 const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties,
										 VkMemoryPropertyFlags requiredProperties,
										 VkMemoryPropertyFlags optimalProperties,
										 bool* optimalSelected);
		static VkShaderModule CreateShaderModule(VkDevice device, std::vector<uint8_t> shaderCode);
		static std::vector<uint8_t> AttemptPipelineCacheRead(const std::string& filePath);
		static void PipelineCacheWrite(const std::string& filePath, const std::vector<uint8_t>& cacheData);
		static VkFormat ChooseDepthFormat(VkPhysicalDevice physicalDevice,
										  const std::vector<VkFormat>& candidates,
										  VkImageTiling tilingMode,
										  VkFormatFeatureFlags features);
		VkResult CreateImageObjects(VkImage* image, VkDeviceMemory* deviceMemory, VkImageView* imageView,
									VkFormat format, VkImageUsageFlags usage,
									uint32_t* width, uint32_t* height, size_t* size,
									uint32_t mipLevels, uint32_t layers);
		VkResult CreateBufferObjects(VkBuffer* buffer, VkDeviceMemory* deviceMemory, size_t* size,
									 VkBufferUsageFlags usage,
									 VkMemoryPropertyFlags requiredMemoryPropertyFlags,
									 VkMemoryPropertyFlags optimalMemoryPropertyFlags, bool* optimalMemory);
		int TransitionImageLayout(VkImage image,
								  VkImageLayout oldLayout,
								  VkImageLayout newLayout);

	private:
		static VkBool32 VulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT* messageData,
													 void* userData);

		// **********************************
		// ** BEGIN Buffer Implementations **
		// **********************************
	public:
		ImageBuffer CreateImageBuffer(size_t width, size_t height, Format format);
		// **********************************
		// ** END   Buffer Implementations **
		// **********************************

	private:
		RendererCapabilities m_Capabilites;

		std::atomic<bool> m_Running;
		bool m_InFrame;

		std::shared_ptr<Window> m_Window;

		VkInstance m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
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
		VkFormat m_DepthFormat;

		VkSwapchainKHR m_Swapchain;
		bool m_RecreateSwapchain;
		VkExtent2D m_SwapchainExtent;
		uint32_t m_SwapchainImageCount;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		ImageBuffer m_DepthImage;

		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSetLayout m_UniformDescriptorSetLayout;
		VkDescriptorSetLayout m_ImageDescriptorSetLayout;
		std::vector<VkDescriptorSet> m_UniformDescriptorSets;
		std::vector<VkDescriptorSet> m_ImageDescriptorSets;

		VkSampler m_Sampler;

		VkRenderPass m_RenderPass;

		VkPipelineLayout m_PipelineLayout;
		VkPipelineCache m_PipelineCache;

		VkPipeline m_MainPipeline;
		VkPipeline m_LinePipeline;
		std::unordered_map<uint32_t, VkPipeline> m_PipelineMap;
		VkPipeline& m_ActivePipeline;

		std::vector<VkFramebuffer> m_Framebuffers;

		VkQueue m_PresentQueue;
		VkQueue m_GraphicsQueue;
		VkQueue m_TransferQueue;

		VkCommandPool m_GraphicsCmdPool;
		std::vector<VkCommandBuffer> m_DrawCmdBuffers;
		std::vector<VkCommandBuffer> m_GeomertyDrawCmdBuffers;

		VkCommandPool m_TransferCmdPool;

		std::vector<std::vector<BakedCommandBuffer>> m_QueuedSubmits;
		std::vector<UsedCommandBuffer> m_CommandBufferDeletionQueue;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkSemaphore> m_TransferFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		glm::vec4 m_ClearColor;
		ImageBuffer m_ImageBuffer;
		UniformBuffer m_UniformBuffer;

		CubeMapBuffer m_Skybox;
		VkDescriptorPool m_SkyboxDesriptorPool;
		VkDescriptorSetLayout m_SkyboxDescriptorSetLayout;
		std::vector<VkDescriptorSet> m_SkyboxDescriptorSets;
		VkSampler m_SkyboxSampler;
		VkPipelineLayout m_SkyboxPipelineLayout;
		VkPipeline m_SkyboxPipeline;
		std::vector<VkCommandBuffer> m_SkyboxDrawCommandBuffers;
		VertexBuffer m_SkyboxVertexBuffer;
		UniformBuffer m_SkyboxUniformBuffer;

		uint32_t m_ImageIndex;
		uint32_t m_FrameIndex;

		friend VkBool32 vulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
									  VkDebugUtilsMessageTypeFlagsEXT messageType,
									  const VkDebugUtilsMessengerCallbackDataEXT* messageData,
									  void* userData);
		VkDebugUtilsMessengerEXT m_DebugMessenger;

	private:
		static oldRenderer* s_Instance;
		static VulkanContext* s_Context;

	private:
		friend StagingBuffer;
	};

	inline glm::mat4 ConstructTransformMatrix2D(const glm::vec3& translation,
												float rotationAngle,
												const glm::vec3& scale)
	{
		glm::mat4 transform = glm::identity<glm::mat4>();
		transform = glm::translate(transform, translation);
		if (rotationAngle != 0.0f) {
			float rs = sinf(rotationAngle);
			float rc = cosf(rotationAngle);

			glm::mat4 rm = {
				{ rc,   rs,   0.0f, 0.0f },
				{ -rs,  rc,   0.0f, 0.0f },
				{ 0.0f, 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f, 0.0f, 1.0f }
			};
			transform = transform * rm;
		}
		transform = glm::scale(transform, scale);

		return transform;
	}

	inline glm::mat4 ConstructTransformMatrix3D(const glm::vec3& translation,
												float rotationAngle,
												const glm::vec3& rotationAxis,
												const glm::vec3& scale)
	{
		glm::mat4 transform = glm::identity<glm::mat4>();
		transform = glm::translate(transform, translation);
		if (rotationAngle != 0.0f) {
			transform = glm::rotate(transform, rotationAngle, rotationAxis);
		}
		transform = glm::scale(transform, scale);

		return transform;
	}
	VkFormat CeeFormatToVkFormat(::cee::Format foramt);
}

#endif
