#ifndef CEE_ENGINE_RENDERER_H
#define CEE_ENGINE_RENDERER_H

#include <CeeEngine/window.h>
#include <CeeEngine/messageBus.h>
#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/camera.h>
#include <CeeEngine/assert.h>

#include <CeeEngine/platform.h>

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

	class Renderer;
	class StagingBuffer;

	enum class ShaderDataType {
		None = 0,
		Float, Float2, Float3, Float4,
		Mat3, Mat4,
		Int, Int2, Int3, Int4,
		Bool
	};

	inline size_t GetShaderDataTypeSize(ShaderDataType type) {
		switch (type) {
			case ShaderDataType::Float:  return 4 * 1;
			case ShaderDataType::Float2: return 4 * 2;
			case ShaderDataType::Float3: return 4 * 3;
			case ShaderDataType::Float4: return 4 * 4;
			case ShaderDataType::Mat3:   return 4 * 3 * 3;
			case ShaderDataType::Mat4:   return 4 * 4 * 4;
			case ShaderDataType::Int:    return 4 * 1;
			case ShaderDataType::Int2:   return 4 * 2;
			case ShaderDataType::Int3:   return 4 * 3;
			case ShaderDataType::Int4:   return 4 * 4;
			case ShaderDataType::Bool:   return 1;
			default:
			{
				CEE_ASSERT(false, "Calling GetShaderDataTypeSize() with an invald type.");
				return -1ull;
			}
		}
	}

	inline size_t GetShaderDataTypeComponentCount(ShaderDataType type) {
		switch (type) {
			case ShaderDataType::Float:  return 1;
			case ShaderDataType::Float2: return 2;
			case ShaderDataType::Float3: return 3;
			case ShaderDataType::Float4: return 4;
			case ShaderDataType::Mat3:   return 3 * 3;
			case ShaderDataType::Mat4:   return 4 * 4;
			case ShaderDataType::Int:    return 1;
			case ShaderDataType::Int2:   return 2;
			case ShaderDataType::Int3:   return 3;
			case ShaderDataType::Int4:   return 4;
			case ShaderDataType::Bool:   return 1;
			default:
			{
				CEE_ASSERT(false, "Calling GetShaderDataTypeSize() with an invald type.");
				return -1ull;
			}
		}
	}

	struct VertexBufferAttribute {
		ShaderDataType type;
		uint32_t size;
		uint32_t offset;

		bool normalized;

		VertexBufferAttribute(ShaderDataType type, bool normalized = false)
		: type(type), size(GetShaderDataTypeSize(type)), normalized(normalized)
		{ }
	};

	struct VertexBufferLayout {
		VertexBufferLayout() {}

		VertexBufferLayout(std::initializer_list<VertexBufferAttribute> attibutes)
		: m_Attributes(attibutes)
		{
			CalculateOffsetAndStride();
		}

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<VertexBufferAttribute>& GetElements() const { return m_Attributes; }

		std::vector<VertexBufferAttribute>::iterator begin() { return m_Attributes.begin(); }
		std::vector<VertexBufferAttribute>::iterator end() { return m_Attributes.end(); }
		std::vector<VertexBufferAttribute>::const_iterator cbegin() const { return m_Attributes.cbegin(); }
		std::vector<VertexBufferAttribute>::const_iterator cend() const { return m_Attributes.cend(); }

	private:
		void CalculateOffsetAndStride() {
			uint32_t offset = 0;
			m_Stride = 0;
			for (auto& attribute : m_Attributes) {
				attribute.offset = offset;
				offset += attribute.size;
				m_Stride += attribute.size;
			}
		}


	private:
		std::vector<VertexBufferAttribute> m_Attributes;
		uint32_t m_Stride;
	};

	class VertexBuffer {
	public:
		VertexBuffer();
		VertexBuffer(VertexBufferLayout layout, size_t size, bool persistantlyMapped = false);
		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&& other);
		~VertexBuffer();

		VertexBuffer& operator=(const VertexBuffer&) = delete;
		VertexBuffer& operator=(VertexBuffer&& other);

		void SetData(size_t size, size_t offset, const void* data);

		VertexBufferLayout GetLayout() const { return m_Layout; }

	private:
		void FlushMemory();
		void MapMemory();
		void UnmapMemory();

		// To use this buffer for buffer copy/usage, this function should be
		// called instead of accessing member directly as it flushes data to the GPU.
		VkBuffer& GetBuffer() { FlushMemory(); return m_Buffer; }

		void FreeResources();

	private:
		VertexBufferLayout m_Layout;
		bool m_Initialized;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		bool m_HostVisable;
		std::optional<void*> m_HostMappedAddress;

		bool m_PersistantlyMapped;

		friend Renderer;
		friend StagingBuffer;
	};

	class IndexBuffer {
	public:
		IndexBuffer();
		IndexBuffer(size_t size, bool persistantlyMapped = false);
		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer(IndexBuffer&& other);
		~IndexBuffer();

		IndexBuffer& operator=(const IndexBuffer&) = delete;
		IndexBuffer& operator=(IndexBuffer&& other);

		void SetData(size_t size, size_t offset, const void* data);

	private:
		void FlushMemory();
		void MapMemory();
		void UnmapMemory();

		VkBuffer GetBuffer() { FlushMemory(); return m_Buffer; }

		void FreeResources();

	private:
		bool m_Initialized;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		bool m_HostVisable;
		std::optional<void*> m_HostMappedAddress;

		bool m_PersistantlyMapped;

		friend Renderer;
		friend StagingBuffer;
	};

	class UniformBuffer {
	public:
		UniformBuffer();
		UniformBuffer(size_t size, bool persistantlyMapped = false);
		UniformBuffer(const UniformBuffer&) = delete;
		UniformBuffer(UniformBuffer&& other);
		~UniformBuffer();

		UniformBuffer& operator=(const UniformBuffer&) = delete;
		UniformBuffer& operator=(UniformBuffer&& other);

		void SetData(size_t size, size_t offset, const void* data);

	private:
		void FlushMemory();
		void MapMemory();
		void UnmapMemory();

		VkBuffer GetBuffer() { FlushMemory(); return m_Buffer; }

		void FreeResources();

	private:
		bool m_Initialized;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		bool m_HostVisable;
		std::optional<void*> m_HostMappedAddress;

		bool m_PersistantlyMapped;

		friend Renderer;
		friend StagingBuffer;
	};

	class ImageBuffer {
	public:
		ImageBuffer();
		ImageBuffer(const ImageBuffer&) = delete;
		ImageBuffer(ImageBuffer&& other);
		~ImageBuffer();

		ImageBuffer& operator=(const ImageBuffer&) = delete;
		ImageBuffer& operator=(ImageBuffer&& other);

		void Clear(glm::vec4 clearColor);

	private:
		void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

	private:
		bool m_Initialized;

		VkDevice m_Device;
		VkCommandPool m_CommandPool;
		VkQueue m_TransferQueue;

		size_t m_Size;
		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_DeviceMemory;

		VkImageLayout m_Layout;

		friend Renderer;
		friend StagingBuffer;
	};

	class CubeMapBuffer {
	public:
		CubeMapBuffer();
		CubeMapBuffer(uint32_t width, uint32_t height);
		// Order: front, back, up, daown, left, right
		CubeMapBuffer(std::vector<std::string> filenames);
		CubeMapBuffer(const CubeMapBuffer&) = delete;
		CubeMapBuffer(CubeMapBuffer&& other);
		~CubeMapBuffer();

		CubeMapBuffer& operator=(const CubeMapBuffer&) = delete;
		CubeMapBuffer& operator=(CubeMapBuffer&& other);

		void Clear(glm::vec4 clearColor);

	private:
		void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

	private:
		bool m_Initialized;

		size_t m_Size;
		VkExtent3D m_Extent;
		VkImage m_Image;
		VkDeviceMemory m_DeviceMemory;
		VkImageView m_ImageView;

		VkImageLayout m_Layout;

		friend Renderer;
		friend StagingBuffer;
	};

	class StagingBuffer {
	public:
		StagingBuffer();
		StagingBuffer(const StagingBuffer&) = delete;
		StagingBuffer(StagingBuffer&& other);
		~StagingBuffer();

		StagingBuffer& operator=(const StagingBuffer&) = delete;
		StagingBuffer& operator=(StagingBuffer&& other);

		int SetData(size_t size, size_t offset, const void* data);

	private:
		int BoundsCheck(size_t size, size_t srcSize, size_t dstSize, size_t srcOffset, size_t dstOddset);

		int TransferDataInternal(VkBuffer src,
								 VkBuffer dst,
								 VkBufferCopy copyRegion);
		int TransferDataInternalImmediate(VkBuffer src,
										  VkBuffer dst,
										  VkBufferCopy copyRegion);

	public:
		int TransferData(VertexBuffer& vertexBuffer, size_t srcOffset, size_t dstOffset, size_t size);
		int TransferData(IndexBuffer& indexBuffer, size_t srcOffset, size_t dstOffset, size_t size);
		int TransferData(UniformBuffer& uniformBuffer, size_t srcOffset, size_t dstOffset, size_t size);
		int TransferData(ImageBuffer& imageBuffer, size_t srcOffset, size_t dstOffset, uint32_t width, uint32_t height);
		int TransferData(CubeMapBuffer& imageBuffer, size_t srcOffset);

		int TransferDataImmediate(VertexBuffer& vertexBuffer, size_t srcOffset, size_t dstOffset, size_t size);
		int TransferDataImmediate(IndexBuffer& indexBuffer, size_t srcOffset, size_t dstOffset, size_t size);
		int TransferDataImmediate(UniformBuffer& uniformBuffer, size_t srcOffset, size_t dstOffset, size_t size);
		int TransferDataImmediate(ImageBuffer& imageBuffer, size_t srcOffset, size_t dstOffset, uint32_t width, uint32_t height);
		int TransferDataImmediate(CubeMapBuffer& imageBuffer, size_t srcOffset);

	private:
		bool m_Initialized;

		VkDevice m_Device;
		VkCommandPool m_CommandPool;
		VkQueue m_TransferQueue;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		void* m_MappedMemoryAddress;

		friend Renderer;
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

	};

	class Renderer {
	public:
		Renderer(const RendererCapabilities& capabilities, std::shared_ptr<cee::Window> window);
		~Renderer();

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

		static Renderer* Get() { return s_Instance; }

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
	private:
		int CreateCommonBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkBufferUsageFlags usage, size_t size);

	public:
		ImageBuffer CreateImageBuffer(size_t width, size_t height, Format format);
		StagingBuffer CreateStagingBuffer(size_t size);
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
		static Renderer* s_Instance;

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
		if (rotationAngle != 0.0f)
			transform = glm::rotate(transform, rotationAngle, rotationAxis);
		transform = glm::scale(transform, scale);




		return transform;
	}
}

#endif
