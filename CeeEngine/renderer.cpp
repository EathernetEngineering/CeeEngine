#include "CeeEngine/assetManager.h"
#include <CeeEngine/renderer.h>
#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/assert.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <limits>
#include <vector>
#include <set>
#include <algorithm>

#include <filesystem>

#include <stb/stb_image.h>

#include <glm/gtc/type_ptr.hpp>

#include <Tracy.hpp>
#include <vulkan/vulkan_core.h>

#define RENDERER_MAX_FRAME_IN_FLIGHT 5u

#define RENDERER_MAX_INDICES 20000u
#define RENDERER_MIN_INDICES 500u

#define BIT(x) (1 << x)
enum PipelineFlagsBits
{
	RENDERER_PIPELINE_FLAG_3D = BIT(0),
	RENDERER_PIPELINE_FLAG_QUAD = BIT(1),
	RENDERER_PIPELINE_BASIC = BIT(2),
	RENDERER_PIPELINE_FILL = BIT(3),
	RENDERER_PIPELINE_RESERVED1 = BIT(4),
	RENDERER_PIPELINE_RESERVED2 = BIT(5),
	RENDERER_PIPELINE_RESERVED3 = BIT(6),
	RENDERER_PIPELINE_RESERVED4 = BIT(7),
};
typedef uint32_t PipelineFlags;

namespace cee {
VkFormat CeeFormatToVkFormat(::cee::ImageFormat foramt) {
	switch (foramt) {
		case IMAGE_FORMAT_R8_SRGB:
			return VK_FORMAT_R8_SRGB;
		case IMAGE_FORMAT_R8G8_SRGB:
			return VK_FORMAT_R8G8_SRGB;
		case IMAGE_FORMAT_R8G8B8_SRGB:
			return VK_FORMAT_R8G8B8_SRGB;
		case IMAGE_FORMAT_R8G8B8A8_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case IMAGE_FORMAT_R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case IMAGE_FORMAT_R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case IMAGE_FORMAT_R8G8B8_UNORM:
			return VK_FORMAT_R8G8B8_UNORM;
		case IMAGE_FORMAT_R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case IMAGE_FORMAT_R8_UINT:
			return VK_FORMAT_R8_UINT;
		case IMAGE_FORMAT_R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;
		case IMAGE_FORMAT_R8G8B8_UINT:
			return VK_FORMAT_R8G8B8_UINT;
		case IMAGE_FORMAT_R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case IMAGE_FORMAT_R16_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case IMAGE_FORMAT_R16G16_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case IMAGE_FORMAT_R16G16B16_SFLOAT:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case IMAGE_FORMAT_R16G16B16A16_SFLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case IMAGE_FORMAT_R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case IMAGE_FORMAT_R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
		case IMAGE_FORMAT_R16G16B16_UNORM:
			return VK_FORMAT_R16G16B16_UNORM;
		case IMAGE_FORMAT_R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case IMAGE_FORMAT_R16_UINT:
			return VK_FORMAT_R16_UINT;
		case IMAGE_FORMAT_R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case IMAGE_FORMAT_R16G16B16_UINT:
			return VK_FORMAT_R16G16B16_UINT;
		case IMAGE_FORMAT_R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case IMAGE_FORMAT_R32_SFLOAT:
			return VK_FORMAT_R32_SFLOAT;
		case IMAGE_FORMAT_R32G32_SFLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		case IMAGE_FORMAT_R32G32B32_SFLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case IMAGE_FORMAT_R32G32B32A32_SFLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case IMAGE_FORMAT_R32_UINT:
			return VK_FORMAT_R32_UINT;
		case IMAGE_FORMAT_R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case IMAGE_FORMAT_R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
		case IMAGE_FORMAT_R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case IMAGE_FORMAT_DEPTH:
			return Renderer::Get()->GetDepthFormat();
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

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
	CeeErrorSeverity ceeMessageSeverity;
	switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			// Do nothing;
			return VK_FALSE;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			ceeMessageSeverity = ERROR_SEVERITY_INFO;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			ceeMessageSeverity = ERROR_SEVERITY_WARNING;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			ceeMessageSeverity = ERROR_SEVERITY_ERROR;
			break;

		default:
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "[%s] Unknown error severity.\tMessage: %s", messageTypeName, messageData->pMessage);
			return VK_FALSE;
	}
	DebugMessenger::PostDebugMessage(ceeMessageSeverity, "[%s] %s", messageTypeName, messageData->pMessage);
	(void)userData;
	return VK_FALSE;
}

VertexBuffer::VertexBuffer()
: m_Initialized(false), m_Device(VK_NULL_HANDLE), m_Size(0),
  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE)
{
}

VertexBuffer::VertexBuffer(VertexBuffer&& other)
: VertexBuffer()
{
	*this = std::move(other);
}

VertexBuffer::~VertexBuffer() {
	if (m_Initialized) {
		vkDeviceWaitIdle(m_Device);
		vkDestroyBuffer(m_Device, m_Buffer, NULL);
		vkFreeMemory(m_Device, m_DeviceMemory, NULL);
	}
}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) {
	if (this->m_Initialized) {
		this->~VertexBuffer();
	}

	this->m_Device = other.m_Device;
	this->m_Size = other.m_Size;
	this->m_Buffer = other.m_Buffer;
	this->m_DeviceMemory = other.m_DeviceMemory;

	this->m_Initialized = other.m_Initialized;
	other.m_Initialized = false;

	other.m_Device = VK_NULL_HANDLE;
	other.m_Buffer = VK_NULL_HANDLE;
	other.m_DeviceMemory = VK_NULL_HANDLE;
	other.m_Size = 0;

	return *this;
}

IndexBuffer::IndexBuffer()
: m_Initialized(false), m_Device(VK_NULL_HANDLE), m_Size(0),
  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE)
{
}

IndexBuffer::IndexBuffer(IndexBuffer&& other)
: IndexBuffer()
{
	*this = std::move(other);
}

IndexBuffer::~IndexBuffer() {
	if (m_Initialized) {
		vkDeviceWaitIdle(m_Device);
		vkDestroyBuffer(m_Device, m_Buffer, NULL);
		vkFreeMemory(m_Device, m_DeviceMemory, NULL);
	}
}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) {
	if (this->m_Initialized) {
		this->~IndexBuffer();
	}

	this->m_Device = other.m_Device;
	this->m_Size = other.m_Size;
	this->m_Buffer = other.m_Buffer;
	this->m_DeviceMemory = other.m_DeviceMemory;

	this->m_Initialized = other.m_Initialized;
	other.m_Initialized = false;

	other.m_Device = VK_NULL_HANDLE;
	other.m_Buffer = VK_NULL_HANDLE;
	other.m_DeviceMemory = VK_NULL_HANDLE;
	other.m_Size = 0;

	return *this;
}

UniformBuffer::UniformBuffer()
: m_Initialized(false), m_Device(VK_NULL_HANDLE), m_Size(0),
  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE)
{
}

UniformBuffer::UniformBuffer(UniformBuffer&& other)
: UniformBuffer()
{
	*this = std::move(other);
}

UniformBuffer::~UniformBuffer() {
	if (m_Initialized) {
		vkDeviceWaitIdle(m_Device);
		vkDestroyBuffer(m_Device, m_Buffer, NULL);
		vkFreeMemory(m_Device, m_DeviceMemory, NULL);
	}
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) {
	if (this->m_Initialized) {
		this->~UniformBuffer();
	}

	this->m_Device = other.m_Device;
	this->m_Size = other.m_Size;
	this->m_Buffer = other.m_Buffer;
	this->m_DeviceMemory = other.m_DeviceMemory;

	this->m_Initialized = other.m_Initialized;
	other.m_Initialized = false;

	other.m_Device = VK_NULL_HANDLE;
	other.m_Buffer = VK_NULL_HANDLE;
	other.m_DeviceMemory = VK_NULL_HANDLE;
	other.m_Size = 0;

	return *this;
}

ImageBuffer::ImageBuffer()
: m_Initialized(false), m_Device(VK_NULL_HANDLE), m_CommandPool(VK_NULL_HANDLE),
  m_TransferQueue(VK_NULL_HANDLE), m_Size(0), m_Image(VK_NULL_HANDLE),
  m_ImageView(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_Layout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

ImageBuffer::ImageBuffer(ImageBuffer&& other)
: ImageBuffer()
{
	*this = std::move(other);
}

ImageBuffer::~ImageBuffer() {
	if (m_Initialized) {
		//vkDeviceWaitIdle(m_Device);
		vkDestroyImageView(m_Device, m_ImageView, NULL);
		vkDestroyImage(m_Device, m_Image, NULL);
		vkFreeMemory(m_Device, m_DeviceMemory, NULL);
	}
}

ImageBuffer& ImageBuffer::operator=(ImageBuffer&& other) {
	if (this->m_Initialized) {
		this->~ImageBuffer();
	}

	this->m_Device = other.m_Device;
	this->m_CommandPool = other.m_CommandPool;
	this->m_TransferQueue = other.m_TransferQueue;
	this->m_Size = other.m_Size;
	this->m_Image = other.m_Image;
	this->m_ImageView = other.m_ImageView;
	this->m_DeviceMemory = other.m_DeviceMemory;
	this->m_Layout = other.m_Layout;

	this->m_Initialized = other.m_Initialized;
	other.m_Initialized = false;

	other.m_Device = VK_NULL_HANDLE;
	other.m_CommandPool = VK_NULL_HANDLE;
	other.m_TransferQueue = VK_NULL_HANDLE;
	other.m_ImageView = VK_NULL_HANDLE;
	other.m_Image = VK_NULL_HANDLE;
	other.m_DeviceMemory = VK_NULL_HANDLE;
	other.m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
	other.m_Size = 0;

	return *this;
}

void ImageBuffer::Clear(glm::vec4 clearColor) {
	Renderer::Get()->ImmediateSubmit([this, clearColor](RawCommandBuffer& cmdBuffer){
		VkClearColorValue clearValue = {};
		clearValue.float32[0] = clearColor.r;
		clearValue.float32[1] = clearColor.g;
		clearValue.float32[2] = clearColor.b;
		clearValue.float32[3] = clearColor.a;
		VkImageSubresourceRange range = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
		vkCmdClearColorImage(cmdBuffer.commandBuffer, m_Image, m_Layout, &clearValue, 1, &range);
	}, QUEUE_GRAPHICS);
}

void ImageBuffer::TransitionLayout(RawCommandBuffer& commandBuffer, VkImageLayout newLayout) {
	if (newLayout == m_Layout) {
		return;
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	imageMemoryBarrier.oldLayout = m_Layout;
	imageMemoryBarrier.newLayout = newLayout;
	// TODO check queue families.
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = m_Image;
	imageMemoryBarrier.subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	VkPipelineStageFlags srcStage, dstStage;

	if (m_Layout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (m_Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (m_Layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else
	{
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Unsupported image layout transition");
		return;
	}


	vkCmdPipelineBarrier(commandBuffer.commandBuffer, srcStage, dstStage, 0,
						 0, NULL,
						 0, NULL,
						 1, &imageMemoryBarrier);

	m_Layout = newLayout;
}

CubeMapBuffer::CubeMapBuffer()
: m_Initialized(false), m_Size(0u), m_Extent({ 0u, 0u, 0u }), m_Image(VK_NULL_HANDLE),
  m_DeviceMemory(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE), m_Layout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

CubeMapBuffer::CubeMapBuffer(uint32_t width, uint32_t height)
: m_Initialized(false), m_Size(0u), m_Extent({ width, height, 1u }), m_Image(VK_NULL_HANDLE),
  m_DeviceMemory(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE), m_Layout(VK_IMAGE_LAYOUT_UNDEFINED)
{
	Renderer::Get()->CreateImageObjects(&m_Image, &m_DeviceMemory, &m_ImageView,
										VK_FORMAT_R8G8B8A8_SRGB,
										VK_IMAGE_USAGE_TRANSFER_DST_BIT |
										VK_IMAGE_USAGE_SAMPLED_BIT,
										&width, &height, &m_Size,
										1, 6);

	m_Initialized = true;
}

CubeMapBuffer::CubeMapBuffer(std::vector<std::shared_ptr<Image>> images)
: m_Initialized(false), m_Size(0u), m_Extent({ 0u, 0u, 1u }), m_Image(VK_NULL_HANDLE),
  m_DeviceMemory(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE), m_Layout(VK_IMAGE_LAYOUT_UNDEFINED)
{
	if (images[0]->width != images[0]->height || images[0]->width == 0) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Image must be square and non-zero size. size: %ix%i");
	}
	m_Extent.height = m_Extent.width = images[0]->width;
	Renderer::Get()->CreateImageObjects(&m_Image, &m_DeviceMemory, &m_ImageView,
										VK_FORMAT_R8G8B8A8_SRGB,
										VK_IMAGE_USAGE_TRANSFER_DST_BIT |
										VK_IMAGE_USAGE_SAMPLED_BIT,
										&m_Extent.width, &m_Extent.height, &m_Size,
										1, 6);
	m_Initialized = true;

	size_t singleImageSize = m_Extent.width * m_Extent.height * 4;
	StagingBuffer sb = Renderer::Get()->CreateStagingBuffer(m_Size);
	for (uint32_t i = 0; i < 6; i++) {
		CEE_ASSERT(images[i]->width == static_cast<int>(m_Extent.width) &&
				   images[i]->height == static_cast<int>(m_Extent.height),
				   "Image sizes do not match");
		sb.SetData(singleImageSize, i * singleImageSize, images[i]->pixels);
	}
	sb.TransferData(*this, 0);
}

CubeMapBuffer::CubeMapBuffer(CubeMapBuffer&& other) {
	*this = std::move(other);
}

CubeMapBuffer::~CubeMapBuffer() {
	if (m_Initialized) {
		vkDestroyImageView(Renderer::Get()->GetDevice(), m_ImageView, NULL);
		vkFreeMemory(Renderer::Get()->GetDevice(), m_DeviceMemory, NULL);
		vkDestroyImage(Renderer::Get()->GetDevice(), m_Image, NULL);
	}
}

CubeMapBuffer& CubeMapBuffer::operator=(CubeMapBuffer&& other) {
	if (this->m_Initialized) {
		this->~CubeMapBuffer();
	}

	this->m_Initialized = other.m_Initialized;
	other.m_Initialized = false;

	this->m_Size = other.m_Size;
	this->m_Extent = other.m_Extent;
	this->m_Image = other.m_Image;
	this->m_DeviceMemory = other.m_DeviceMemory;
	this->m_ImageView = other.m_ImageView;
	this->m_Layout = other.m_Layout;

	other.m_Size = 0;
	other.m_Extent = { 0u, 0u, 0u };
	other.m_Image = VK_NULL_HANDLE;
	other.m_DeviceMemory = VK_NULL_HANDLE;
	other.m_ImageView = VK_NULL_HANDLE;
	other.m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;

	return *this;
}

void CubeMapBuffer::Clear(glm::vec4 clearColor) {
	Renderer::Get()->ImmediateSubmit([this, &clearColor](RawCommandBuffer& cmdBuffer){
		this->TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkClearColorValue vkClearColor;
		vkClearColor.float32[0] = clearColor.r;
		vkClearColor.float32[1] = clearColor.g;
		vkClearColor.float32[2] = clearColor.b;
		vkClearColor.float32[3] = clearColor.a;
		VkImageSubresourceRange range = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 6
		};
		vkCmdClearColorImage(cmdBuffer.commandBuffer, m_Image, m_Layout, &vkClearColor, 1, &range);
		this->TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}, QUEUE_GRAPHICS);
}

void CubeMapBuffer::TransitionLayout(RawCommandBuffer& cmdBuffer, VkImageLayout newLayout) {
	if (newLayout == m_Layout) {
		return;
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	imageMemoryBarrier.oldLayout = m_Layout;
	imageMemoryBarrier.newLayout = newLayout;
	// TODO check queue families.
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = m_Image;
	imageMemoryBarrier.subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 6
	};

	VkPipelineStageFlags srcStage, dstStage;

	if (m_Layout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (m_Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (m_Layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else
	{
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Unsupported image layout transition");
		return;
	}


	vkCmdPipelineBarrier(cmdBuffer.commandBuffer, srcStage, dstStage, 0,
						 0, NULL,
						 0, NULL,
						 1, &imageMemoryBarrier);

	m_Layout = newLayout;
}

StagingBuffer::StagingBuffer()
: m_Initialized(false), m_Device(VK_NULL_HANDLE), m_CommandPool(VK_NULL_HANDLE),
  m_TransferQueue(VK_NULL_HANDLE), m_Size(0), m_Buffer(VK_NULL_HANDLE),
  m_DeviceMemory(VK_NULL_HANDLE), m_MappedMemoryAddress(NULL)
{
}

StagingBuffer::StagingBuffer(StagingBuffer&& other)
: StagingBuffer()
{
	*this = std::move(other);
}

StagingBuffer::~StagingBuffer() {
	if (m_Initialized) {
		vkDeviceWaitIdle(m_Device);
		vkUnmapMemory(m_Device, m_DeviceMemory);
		vkDestroyBuffer(m_Device, m_Buffer, NULL);
		vkFreeMemory(m_Device, m_DeviceMemory, NULL);
	}
}

StagingBuffer& StagingBuffer::operator=(StagingBuffer&& other) {
	if (this->m_Initialized) {
		this->~StagingBuffer();
	}

	this->m_Device = other.m_Device;
	this->m_CommandPool = other.m_CommandPool;
	this->m_TransferQueue = other.m_TransferQueue;
	this->m_Size = other.m_Size;
	this->m_Buffer = other.m_Buffer;
	this->m_DeviceMemory = other.m_DeviceMemory;
	this->m_MappedMemoryAddress = other.m_MappedMemoryAddress;

	this->m_Initialized = other.m_Initialized;
	other.m_Initialized = false;

	other.m_Device = VK_NULL_HANDLE;
	other.m_CommandPool = VK_NULL_HANDLE;
	other.m_TransferQueue = VK_NULL_HANDLE;
	other.m_Buffer = VK_NULL_HANDLE;
	other.m_DeviceMemory = VK_NULL_HANDLE;
	other.m_Size = 0;
	other.m_MappedMemoryAddress = NULL;

	return *this;
}

int StagingBuffer::SetData(size_t size, size_t offset, const void* data) {
	// Check to ensure buffer is initialized.
	if (!m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	// Do bounds checking. Note no way to check bounds of user data provided
	if (size + offset > m_Size) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Overflow! Trying to copy more data than buffer has capacity for.");
		return -1;
	}
	if (size == 0) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Trying to copy 0 bytes into buffer.");
		return -1;
	}

	// Copy data.
	memcpy(static_cast<uint8_t*>(m_MappedMemoryAddress) + offset, data, size);
	return 0;
}

int StagingBuffer::BoundsCheck(size_t size, size_t srcSize, size_t dstSize, size_t srcOffset, size_t dstOffset) {
	if (size + srcOffset > srcSize) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Overflow. Trying to copy more data than src can hold.");
		return -1;
	}
	if (size + dstOffset > dstSize) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Overflow. Trying to copy more data than dst can hold.");
		return -1;
	}
	return 0;
}

int StagingBuffer::TransferDataInternal(VkBuffer src,
										VkBuffer dst,
										VkBufferCopy copyRegion)
{
	Renderer::Get()->QueueSubmit([src, dst, copyRegion](RawCommandBuffer& cmdBuffer){
		vkCmdCopyBuffer(cmdBuffer.commandBuffer, src, dst, 1, &copyRegion);
	}, QUEUE_TRANSFER);

	return 0;
}

int StagingBuffer::TransferDataInternalImmediate(VkBuffer src,
												 VkBuffer dst,
												 VkBufferCopy copyRegion)
{
	Renderer::Get()->ImmediateSubmit([src, dst, copyRegion](RawCommandBuffer& cmdBuffer) {
		vkCmdCopyBuffer(cmdBuffer.commandBuffer, src, dst, 1, &copyRegion);
	}, QUEUE_TRANSFER);

	return 0;
}

int StagingBuffer::TransferData(VertexBuffer& vertexBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
	// Check that buffers are valid
	if (!m_Initialized || !vertexBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	// Do bounds checking.
	if (BoundsCheck(size, m_Size, vertexBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	// Do copy.
	return TransferDataInternal(this->m_Buffer,
								vertexBuffer.m_Buffer,
								{ srcOffset, dstOffset, size });
}

int StagingBuffer::TransferData(IndexBuffer& indexBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
	if (!m_Initialized || !indexBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	if (BoundsCheck(size, m_Size, indexBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	return TransferDataInternal(m_Buffer,
								indexBuffer.m_Buffer,
								{ srcOffset, dstOffset, size });
}

int StagingBuffer::TransferData(UniformBuffer& uniformBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
	if (!m_Initialized || !uniformBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	if (BoundsCheck(size, m_Size, uniformBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	return TransferDataInternal(m_Buffer,
								uniformBuffer.m_Buffer,
								{ srcOffset, dstOffset, size });
}

int StagingBuffer::TransferData(ImageBuffer& imageBuffer, size_t srcOffset, size_t dstOffset, uint32_t width, uint32_t height) {
	if (!m_Initialized || !imageBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	if (BoundsCheck(width * height * 4, m_Size, imageBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	VkResult result;

	VkBuffer src = this->m_Buffer;
	VkImage dst = imageBuffer.m_Image;
	result = Renderer::Get()->QueueSubmit([src, dst, width, height, &imageBuffer](RawCommandBuffer& cmdBuffer) {
		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy imageCopy;
		imageCopy.bufferOffset = 0;
		imageCopy.bufferImageHeight = 0;
		imageCopy.bufferRowLength = 0;
		imageCopy.imageExtent = { width, height, 1 };
		imageCopy.imageOffset = { 0, 0, 0 };
		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.layerCount = 1;

		vkCmdCopyBufferToImage(cmdBuffer.commandBuffer,
							src,
							dst,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1,
							&imageCopy);

		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}, QUEUE_GRAPHICS); // TODO: Deal with layout transitions VUID-vkCmdPipelineBarrier-dstStageMask-06462 and switch back to QUEUE_TRANSFER


	return result == VK_SUCCESS ? 0 : -1;
}
int StagingBuffer::TransferData(CubeMapBuffer& imageBuffer, size_t srcOffset) {
	if (!this->m_Initialized || !imageBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	VkResult result = Renderer::Get()->ImmediateSubmit([this, &imageBuffer, srcOffset](RawCommandBuffer& cmdBuffer){
		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		std::array<VkBufferImageCopy, 6> imageCopyRanges;
		for (uint32_t i = 0; i < 6; i++) {
			VkImageSubresourceLayers subresourceLayers = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = i,
				.layerCount = 1
			};
			VkBufferImageCopy range = {
				.bufferOffset = (imageBuffer.m_Extent.width * imageBuffer.m_Extent.height * 4 * i) + srcOffset,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = subresourceLayers,
				.imageOffset = { 0u, 0u, 0u },
				.imageExtent = imageBuffer.m_Extent
			};
			imageCopyRanges[i] = range;
		}
		vkCmdCopyBufferToImage(cmdBuffer.commandBuffer, m_Buffer,
							   imageBuffer.m_Image, imageBuffer.m_Layout,
							   6, imageCopyRanges.data());

		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}, QUEUE_GRAPHICS); // TODO: Deal with layout transitions VUID-vkCmdPipelineBarrier-dstStageMask-06462 and switch back to QUEUE_TRANSFER


	return result == VK_SUCCESS ? 0 : -1;
}

int StagingBuffer::TransferDataImmediate(VertexBuffer& vertexBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
	// Check that buffers are valid
	if (!m_Initialized || !vertexBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	// Do bounds checking.
	if (BoundsCheck(size, m_Size, vertexBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	// Do copy.
	return TransferDataInternalImmediate(m_Buffer,
										 vertexBuffer.m_Buffer,
										 { srcOffset, dstOffset, size });
}

int StagingBuffer::TransferDataImmediate(IndexBuffer& indexBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
	if (!m_Initialized || !indexBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	if (BoundsCheck(size, m_Size, indexBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	return TransferDataInternalImmediate(m_Buffer,
										 indexBuffer.m_Buffer,
										 { srcOffset, dstOffset, size });
}

int StagingBuffer::TransferDataImmediate(UniformBuffer& uniformBuffer, size_t srcOffset, size_t dstOffset, size_t size) {
	if (!m_Initialized || !uniformBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	if (BoundsCheck(size, m_Size, uniformBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	return TransferDataInternalImmediate(m_Buffer,
										 uniformBuffer.m_Buffer,
										 { srcOffset, dstOffset, size });
}

int StagingBuffer::TransferDataImmediate(ImageBuffer& imageBuffer, size_t srcOffset, size_t dstOffset, uint32_t width, uint32_t height) {
	if (!m_Initialized || !imageBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	if (BoundsCheck(width * height * 4, m_Size, imageBuffer.m_Size, srcOffset, dstOffset) != 0) {
		return -1;
	}

	VkResult result;

	VkBuffer src = this->m_Buffer;
	VkImage dst = imageBuffer.m_Image;
	result = Renderer::Get()->ImmediateSubmit([src, dst, width, height, &imageBuffer](RawCommandBuffer& cmdBuffer) {
		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy imageCopy;
		imageCopy.bufferOffset = 0;
		imageCopy.bufferImageHeight = 0;
		imageCopy.bufferRowLength = 0;
		imageCopy.imageExtent = { width, height, 1 };
		imageCopy.imageOffset = { 0, 0, 0 };
		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.layerCount = 1;

		vkCmdCopyBufferToImage(cmdBuffer.commandBuffer,
							src,
							dst,
							VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1,
							&imageCopy);
		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}, QUEUE_GRAPHICS); // TODO: Deal with layout transitions VUID-vkCmdPipelineBarrier-dstStageMask-06462 and switch back to QUEUE_TRANSFER


	return result == VK_SUCCESS ? 0 : -1;
}

int StagingBuffer::TransferDataImmediate(CubeMapBuffer& imageBuffer, size_t srcOffset) {
	if (!this->m_Initialized || !imageBuffer.m_Initialized) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Trying to copy data using an uninitialized buffer.");
		return -1;
	}
	VkResult result = Renderer::Get()->ImmediateSubmit([this, &imageBuffer, srcOffset](RawCommandBuffer& cmdBuffer){
		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		std::array<VkBufferImageCopy, 6> imageCopyRanges;
		for (uint32_t i = 0; i < 6; i++) {
			VkImageSubresourceLayers subresourceLayers = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = i,
				.layerCount = 1
			};
			VkBufferImageCopy range = {
				.bufferOffset = (imageBuffer.m_Extent.width * imageBuffer.m_Extent.height * 4) + srcOffset,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = subresourceLayers,
				.imageOffset = { 0u, 0u, 0u },
				.imageExtent = imageBuffer.m_Extent
			};
			imageCopyRanges[i] = range;
		}
		vkCmdCopyBufferToImage(cmdBuffer.commandBuffer, m_Buffer,
							   imageBuffer.m_Image, imageBuffer.m_Layout,
							   6, imageCopyRanges.data());

		imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}, QUEUE_GRAPHICS); // TODO: Deal with layout transitions VUID-vkCmdPipelineBarrier-dstStageMask-06462 and switch back to QUEUE_TRANSFER
	return result == VK_SUCCESS ? 0 : -1;
}



Renderer* Renderer::s_Instance = NULL;

Renderer::Renderer(const RendererSpec& spec, const RendererCapabilities& capabilities)
 : m_Capabilites(capabilities), m_EnableValidationLayers(spec.enableValidationLayers), m_Window(spec.window),
   m_Instance(VK_NULL_HANDLE), m_PhysicalDevice(VK_NULL_HANDLE), m_PhysicalDeviceProperties({}),
   m_Device(VK_NULL_HANDLE), m_Surface(VK_NULL_HANDLE), m_Swapchain(VK_NULL_HANDLE),
   m_DepthImage(ImageBuffer()), m_RenderPass(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE),
   m_PipelineCache(VK_NULL_HANDLE), m_MainPipeline(VK_NULL_HANDLE),
   m_LinePipeline(VK_NULL_HANDLE), m_ActivePipeline(m_MainPipeline), m_PresentQueue(VK_NULL_HANDLE),
   m_GraphicsQueue(VK_NULL_HANDLE), m_TransferQueue(VK_NULL_HANDLE),
   m_GraphicsCmdPool(VK_NULL_HANDLE), m_TransferCmdPool(VK_NULL_HANDLE),
   m_ImageIndex(0), m_FrameIndex(0), m_QueueSubmissionIndex(0), m_DebugMessenger(VK_NULL_HANDLE)
{
	m_Running = false;
}

Renderer::~Renderer()
{
	//vkDeviceWaitIdle(m_Device);
	vkQueueWaitIdle(m_GraphicsQueue);
	vkQueueWaitIdle(m_TransferQueue);
	this->Shutdown();
}

int Renderer::Init()
{
	if (s_Instance != NULL) {
		CEE_ASSERT(false, "Only allowed one renderer instace.");
	}
	s_Instance = this;

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
	if (m_Capabilites.rendererMode != RENDERER_MODE_2D &&
		m_Capabilites.rendererMode != RENDERER_MODE_3D)
	{
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Must choose renderer mode. Given RENDERER_MODE_UNKNOWN.");
		return -1;
	}

	{
		VkLayerProperties *layerProperties = NULL;
		uint32_t layerPropertiesCount = 0;
		VkExtensionProperties *extensionProperties = NULL;
		uint32_t extensionPropertiesCount = 0;
		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions;

		result = vkEnumerateInstanceLayerProperties(&layerPropertiesCount, NULL);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate instance layer properties.");
			return -1;
		}
		layerProperties = (VkLayerProperties*)std::calloc(100, sizeof(VkLayerProperties));
		if (layerProperties == NULL) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Out of memory.");
			return -1;
		}
		result = vkEnumerateInstanceLayerProperties(&layerPropertiesCount, layerProperties);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate instance layer properties.");
			return -1;
		}

		for (uint32_t i = 0; i < layerPropertiesCount; i++) {
			if (m_EnableValidationLayers) {
				if (strcmp(layerProperties[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
					enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
					DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan layer %s.", layerProperties[i].layerName);
					continue;

				}
			}
		}
		std::free(layerProperties);
		layerProperties = NULL;

		result = vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertiesCount, NULL);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate instance extension properties.");
			return -1;
		}
		extensionProperties = (VkExtensionProperties*)std::calloc(extensionPropertiesCount, sizeof(VkExtensionProperties));
		if (extensionProperties == NULL) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Out of memory.");
			return -1;
		}
		result = vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertiesCount, extensionProperties);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate instance extension properties.");
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
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan extension %s.", extensionProperties[i].extensionName);
				continue;
			}
			if (strcmp(surfaceExtensionName, extensionProperties[i].extensionName) == 0) {
				enabledExtensions.push_back(surfaceExtensionName);
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan extension %s.", extensionProperties[i].extensionName);
				continue;
			}
#ifndef NDEBUG
			if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProperties[i].extensionName) == 0) {
				enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using Vulkan extension %s.", extensionProperties[i].extensionName);
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

#ifndef NDEBUG
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

		instanceCreateInfo.pNext = &debugMessengerCreateInfo;
#endif

		result = vkCreateInstance(&instanceCreateInfo, NULL, &m_Instance);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create instance.");
			return -1;
		}
#ifndef NDEBUG
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXTfn =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

		if (vkCreateDebugUtilsMessengerEXTfn) {
			vkCreateDebugUtilsMessengerEXTfn(m_Instance, &debugMessengerCreateInfo, NULL, &m_DebugMessenger);
		} else {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to get proc address for 'vkCreateDebugUtilsMessengerEXT'");
		}
#endif
	}
	{
		uint32_t physicalDeviceCount = 0;
		VkPhysicalDevice *physicalDevices;
		result = vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, NULL);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate physical devices.");
			return -1;
		}
		physicalDevices = (VkPhysicalDevice*)std::calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
		if (physicalDevices == NULL) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Out of memory.");
			return -1;
		}
		result = vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate physical devices.");
			return -1;
		}
		m_PhysicalDevice = this->ChoosePhysicalDevice(physicalDeviceCount, physicalDevices);
		std::free(physicalDevices);

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_PhysicalDeviceMemoryProperties);

		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Phsysical device properties:");
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tDevice Name: %s", m_PhysicalDeviceProperties.deviceName);
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tVendor Id: %u", m_PhysicalDeviceProperties.vendorID);
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tDiscrete: %s", &"false\0true"[6*(m_PhysicalDeviceProperties.deviceType ==
				VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)]);
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tAPI Version: %u.%u.%u",
				(m_PhysicalDeviceProperties.apiVersion & 0x1FC00000) >> 22,
				(m_PhysicalDeviceProperties.apiVersion & 0x3FF000) >> 12,
				(m_PhysicalDeviceProperties.apiVersion & 0xFFF));

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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create surface.");
			return -1;
		}
	}
	{
		uint32_t queueFamilyPropertiesCount;
		VkQueueFamilyProperties *queueFamilyProperties;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount, NULL);
		queueFamilyProperties = (VkQueueFamilyProperties*)std::calloc(queueFamilyPropertiesCount, sizeof(VkQueueFamilyProperties));
		if (queueFamilyProperties == NULL) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Out of memory.");
			return -1;
		}
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyPropertiesCount, queueFamilyProperties);

		for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
			VkBool32 presentSupport = VK_FALSE;
			// Prefer queue that supports both transfer and graphics operations
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
			if (m_QueueFamilyIndices.transferIndex.value_or(-1) == m_QueueFamilyIndices.graphicsIndex.value_or(-1) &&
				m_QueueFamilyIndices.transferIndex.value_or(-1) != (uint32_t)(-1)) {
				for (uint32_t j = 0; j < queueFamilyPropertiesCount; j++) {
					if (m_QueueFamilyIndices.transferIndex.value() == j) {
						continue;
					}
					if (queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) {
						m_QueueFamilyIndices.transferIndex = j;
						break;
					}
				}
			}
		}
		std::free(queueFamilyProperties);

		if (!(m_QueueFamilyIndices.graphicsIndex.has_value() &&
			m_QueueFamilyIndices.computeIndex.has_value() &&
			m_QueueFamilyIndices.transferIndex.has_value() &&
			m_QueueFamilyIndices.presentIndex.has_value()))
		{
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Queue family without value.");
			DebugMessenger::PostDebugMessage(m_QueueFamilyIndices.presentIndex.has_value() ? ERROR_SEVERITY_DEBUG : ERROR_SEVERITY_ERROR,
									"\tPresent Queue index: %u", m_QueueFamilyIndices.presentIndex.value_or(-1));
			DebugMessenger::PostDebugMessage(m_QueueFamilyIndices.graphicsIndex.has_value() ? ERROR_SEVERITY_DEBUG : ERROR_SEVERITY_ERROR,
									"\tGraphics Queue index: %u", m_QueueFamilyIndices.graphicsIndex.value_or(-1));
			DebugMessenger::PostDebugMessage(m_QueueFamilyIndices.computeIndex.has_value() ? ERROR_SEVERITY_DEBUG : ERROR_SEVERITY_ERROR,
									"\tCompute Queue index: %u", m_QueueFamilyIndices.computeIndex.value_or(-1));
			DebugMessenger::PostDebugMessage(m_QueueFamilyIndices.transferIndex.has_value() ? ERROR_SEVERITY_DEBUG : ERROR_SEVERITY_ERROR,
									"\tTransfer Queue index: %u", m_QueueFamilyIndices.transferIndex.value_or(-1));
		} else {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Using queue families:");
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tPresent Queue index: %u",
					m_QueueFamilyIndices.presentIndex.value_or(VK_QUEUE_FAMILY_IGNORED));
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tGraphics Queue index: %u",
					m_QueueFamilyIndices.graphicsIndex.value_or(VK_QUEUE_FAMILY_IGNORED));
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tCompute Queue index: %u",
					m_QueueFamilyIndices.computeIndex.value_or(VK_QUEUE_FAMILY_IGNORED));
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "\tTransfer Queue index: %u",
					m_QueueFamilyIndices.transferIndex.value_or(VK_QUEUE_FAMILY_IGNORED));
		}
	}
	{
		VkExtensionProperties *extensionProperties = NULL;
		uint32_t extensionPropertiesCount = 0;
		std::vector<const char*> enabledExtensions;

		result = vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionPropertiesCount, NULL);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate device extensions.");
			return -1;
		}
		extensionProperties = (VkExtensionProperties*)std::calloc(extensionPropertiesCount, sizeof(VkExtensionProperties));
		if (extensionProperties == NULL) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Out of memory.");
			return -1;
		}
		result = vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionPropertiesCount, extensionProperties);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to enumerate device extensions.");
			return -1;
		}

		for (uint32_t i = 0; i < extensionPropertiesCount; i++) {
			if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extensionProperties[i].extensionName) == 0) {
				enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "Using device extension: %s", extensionProperties[i].extensionName);
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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create logical device.");
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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Swapchain not adequate.");
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
		CEE_VERIFY(result == VK_SUCCESS, "Failed to initialise swapchain.");

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
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Failed to create image views for the swapchain.");
				return -1;
			}
			m_SwapchainImageViews.push_back(imageView);
		}
		m_RecreateSwapchain = false;

		m_DepthFormat = ChooseDepthFormat(m_PhysicalDevice,
													   { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
													   VK_IMAGE_TILING_OPTIMAL,
													   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		m_DepthImage = this->CreateImageBuffer(m_SwapchainExtent.width,
											   m_SwapchainExtent.height,
											   IMAGE_FORMAT_DEPTH);
	}
	{

		VkAttachmentDescription colorAttachmentDescription = {};
		colorAttachmentDescription.flags = 0;
		colorAttachmentDescription.format = m_SwapchainImageFormat;
		colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachmentDescription = {};
		depthAttachmentDescription.flags = 0;
		depthAttachmentDescription.format = m_DepthFormat;
		depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.flags = 0;
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentReference;
		subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = NULL;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = NULL;
		subpassDescription.pResolveAttachments = NULL;

		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments {
			colorAttachmentDescription,
			depthAttachmentDescription
		};

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext = NULL;
		renderPassCreateInfo.flags = 0;
		renderPassCreateInfo.attachmentCount = attachments.size();
		renderPassCreateInfo.pAttachments = attachments.data();
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

		result = vkCreateRenderPass(m_Device, &renderPassCreateInfo, NULL, &m_RenderPass);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create render pass.");
			return -1;
		}
	}
	{
		VkDescriptorSetLayoutBinding uniformDescriptorSetLayoutBinding {};
		uniformDescriptorSetLayoutBinding.binding = 0;
		uniformDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformDescriptorSetLayoutBinding.descriptorCount = 1;
		uniformDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uniformDescriptorSetLayoutBinding.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding {};
		samplerDescriptorSetLayoutBinding.binding = 0;
		samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;;
		samplerDescriptorSetLayoutBinding.descriptorCount = 1;
		samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerDescriptorSetLayoutBinding.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutBinding imageDescriptorSetLayoutBinding {};
		imageDescriptorSetLayoutBinding.binding = 1;
		imageDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		imageDescriptorSetLayoutBinding.descriptorCount = 32;
		imageDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageDescriptorSetLayoutBinding.pImmutableSamplers = NULL;

		std::array<VkDescriptorSetLayoutBinding, 1> uniformDescriptorSetLayoutBindings = {
			uniformDescriptorSetLayoutBinding
		};

		std::array<VkDescriptorSetLayoutBinding, 2> imageDescriptorSetLayoutBindings = {
			samplerDescriptorSetLayoutBinding,
			imageDescriptorSetLayoutBinding
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.bindingCount = uniformDescriptorSetLayoutBindings.size();
		descriptorSetLayoutCreateInfo.pBindings = uniformDescriptorSetLayoutBindings.data();

		result = vkCreateDescriptorSetLayout(m_Device,
											 &descriptorSetLayoutCreateInfo,
											 NULL,
											 &m_UniformDescriptorSetLayout);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to create uniform descriptor set layout.");
			return -1;
		}

		descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.bindingCount = imageDescriptorSetLayoutBindings.size();
		descriptorSetLayoutCreateInfo.pBindings = imageDescriptorSetLayoutBindings.data();

		result = vkCreateDescriptorSetLayout(m_Device,
											 &descriptorSetLayoutCreateInfo,
											 NULL,
											 &m_ImageDescriptorSetLayout);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to create image and sampler descriptor set layout.");
			return -1;
		}

		VkDescriptorPoolSize uniformDescriptorPoolSize = {};
		uniformDescriptorPoolSize.descriptorCount = m_Capabilites.maxFramesInFlight;
		uniformDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolSize samplerDescriptorPoolSize = {};
		samplerDescriptorPoolSize.descriptorCount = m_Capabilites.maxFramesInFlight;
		samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;

		VkDescriptorPoolSize imageDescriptorPoolSize = {};
		imageDescriptorPoolSize.descriptorCount = m_Capabilites.maxFramesInFlight * 32;
		imageDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

		std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes = {
			uniformDescriptorPoolSize,
			samplerDescriptorPoolSize,
			imageDescriptorPoolSize
		};

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.pNext = NULL;
		descriptorPoolCreateInfo.flags = 0;
		descriptorPoolCreateInfo.maxSets = m_Capabilites.maxFramesInFlight + 32;
		descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

		result = vkCreateDescriptorPool(m_Device, &descriptorPoolCreateInfo, NULL, &m_DescriptorPool);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to create descriptor pool.");
			return -1;
		}
	}
	{
		m_UniformDescriptorSets.resize(m_Capabilites.maxFramesInFlight);
		std::vector<VkDescriptorSetLayout> descriptorLayouts(m_Capabilites.maxFramesInFlight,
															 m_UniformDescriptorSetLayout);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = NULL;
		descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = m_Capabilites.maxFramesInFlight;
		descriptorSetAllocateInfo.pSetLayouts = descriptorLayouts.data();

		result = vkAllocateDescriptorSets(m_Device, &descriptorSetAllocateInfo, m_UniformDescriptorSets.data());
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to allocate descriptor sets for uniform.");
			return -1;
		}

		m_ImageDescriptorSets.resize(m_Capabilites.maxFramesInFlight);
		descriptorLayouts.clear();
		descriptorLayouts.insert(descriptorLayouts.end(),
								 m_Capabilites.maxFramesInFlight,
								 m_ImageDescriptorSetLayout);
		descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = NULL;
		descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = m_Capabilites.maxFramesInFlight;
		descriptorSetAllocateInfo.pSetLayouts = descriptorLayouts.data();

		result = vkAllocateDescriptorSets(m_Device, &descriptorSetAllocateInfo, m_ImageDescriptorSets.data());
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to allocate descriptor sets for image/sampler.");
			return -1;
		}
	}
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.pNext = NULL;
		samplerCreateInfo.flags = 0;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_GREATER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		result = vkCreateSampler(m_Device, &samplerCreateInfo, NULL, &m_Sampler);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to create sampler.");
		}
	}
	{
		// TODO: Abstract file operations.
		const char* quad2DVertexShaderFilePath = "shaders/renderer2DQuadVertex.spv";
		const char* quad2DFragmentShaderFilePath = "shaders/renderer2DQuadFragment.spv";
		const char* basic3DVertexShaderFilePath = "shaders/renderer3DBasicVertex.spv";
		const char* basic3DFragmentShaderFilePath = "shaders/renderer3DBasicFragment.spv";

		auto quad2DVertexShaderCode = m_AssetManager.LoadAsset<ShaderBinary>(quad2DVertexShaderFilePath);
		auto quad2DFragmentShaderCode = m_AssetManager.LoadAsset<ShaderBinary>(quad2DFragmentShaderFilePath);
		auto basic3DVertexShaderCode = m_AssetManager.LoadAsset<ShaderBinary>(basic3DVertexShaderFilePath);
		auto basic3DFragmentShaderCode = m_AssetManager.LoadAsset<ShaderBinary>(basic3DFragmentShaderFilePath);

		VkShaderModule quad2DVertexShaderModule = this->CreateShaderModule(m_Device, quad2DVertexShaderCode);
		VkShaderModule quad2DFragmentShaderModule = this->CreateShaderModule(m_Device, quad2DFragmentShaderCode);
		VkShaderModule basic3DVertexShaderModule = this->CreateShaderModule(m_Device, basic3DVertexShaderCode);
		VkShaderModule basic3DFragmentShaderModule = this->CreateShaderModule(m_Device, basic3DFragmentShaderCode);

		quad2DVertexShaderCode.reset();
		quad2DFragmentShaderCode.reset();
		basic3DVertexShaderCode.reset();
		basic3DFragmentShaderCode.reset();

		VkPipelineShaderStageCreateInfo quad2DVertexShaderStageCreateInfo = {};
		quad2DVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		quad2DVertexShaderStageCreateInfo.pNext = NULL;
		quad2DVertexShaderStageCreateInfo.flags = 0;
		quad2DVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		quad2DVertexShaderStageCreateInfo.module = quad2DVertexShaderModule;
		quad2DVertexShaderStageCreateInfo.pName = "main";
		quad2DVertexShaderStageCreateInfo.pSpecializationInfo = NULL;

		VkPipelineShaderStageCreateInfo quad2DFragmentShaderStageCreateInfo = {};
		quad2DFragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		quad2DFragmentShaderStageCreateInfo.pNext = NULL;
		quad2DFragmentShaderStageCreateInfo.flags = 0;
		quad2DFragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		quad2DFragmentShaderStageCreateInfo.module = quad2DFragmentShaderModule;
		quad2DFragmentShaderStageCreateInfo.pName = "main";
		quad2DFragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

		VkPipelineShaderStageCreateInfo basic3DVertexShaderStageCreateInfo = {};
		basic3DVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		basic3DVertexShaderStageCreateInfo.pNext = NULL;
		basic3DVertexShaderStageCreateInfo.flags = 0;
		basic3DVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		basic3DVertexShaderStageCreateInfo.module = basic3DVertexShaderModule;
		basic3DVertexShaderStageCreateInfo.pName = "main";
		basic3DVertexShaderStageCreateInfo.pSpecializationInfo = NULL;

		VkPipelineShaderStageCreateInfo basic3DFragmentShaderStageCreateInfo = {};
		basic3DFragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		basic3DFragmentShaderStageCreateInfo.pNext = NULL;
		basic3DFragmentShaderStageCreateInfo.flags = 0;
		basic3DFragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		basic3DFragmentShaderStageCreateInfo.module = basic3DFragmentShaderModule;
		basic3DFragmentShaderStageCreateInfo.pName = "main";
		basic3DFragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

		VkPipelineShaderStageCreateInfo quad2DShaderStageCreateInfos[] = {
			quad2DVertexShaderStageCreateInfo,
			quad2DFragmentShaderStageCreateInfo
		};

		VkPipelineShaderStageCreateInfo basic3DShaderStageCreateInfos[] = {
			basic3DVertexShaderStageCreateInfo,
			basic3DFragmentShaderStageCreateInfo
		};

		std::vector<VkVertexInputAttributeDescription> quad2DVertexInputAttributes;
		std::vector<VkVertexInputAttributeDescription> basic3DVertexInputAttributes;
		VkVertexInputAttributeDescription vertexInputAttribute = {};
		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexInputAttribute.location = 0;
		vertexInputAttribute.offset = 0;
		quad2DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexInputAttribute.location = 1;
		vertexInputAttribute.offset = 16;
		quad2DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32_SFLOAT;
		vertexInputAttribute.location = 2;
		vertexInputAttribute.offset = 32;
		quad2DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexInputAttribute.location = 0;
		vertexInputAttribute.offset = 0;
		basic3DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttribute.location = 1;
		vertexInputAttribute.offset = 16;
		basic3DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertexInputAttribute.location = 2;
		vertexInputAttribute.offset = 28;
		basic3DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32G32_SFLOAT;
		vertexInputAttribute.location = 3;
		vertexInputAttribute.offset = 44;
		basic3DVertexInputAttributes.push_back(vertexInputAttribute);

		vertexInputAttribute.binding = 0;
		vertexInputAttribute.format = VK_FORMAT_R32_UINT;
		vertexInputAttribute.location = 4;
		vertexInputAttribute.offset = 52;
		basic3DVertexInputAttributes.push_back(vertexInputAttribute);

		std::vector<VkVertexInputBindingDescription> quad2DVertexInputBindings;
		std::vector<VkVertexInputBindingDescription> basic3DVertexInputBindings;
		VkVertexInputBindingDescription vertexInputBinding = {};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = 40;
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		quad2DVertexInputBindings.push_back(vertexInputBinding);

		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = 56;
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		basic3DVertexInputBindings.push_back(vertexInputBinding);

		VkPipelineVertexInputStateCreateInfo quad2DVertexInputStateCreateInfo = {};
		quad2DVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		quad2DVertexInputStateCreateInfo.pNext = NULL;
		quad2DVertexInputStateCreateInfo.flags = 0;
		quad2DVertexInputStateCreateInfo.vertexAttributeDescriptionCount = quad2DVertexInputAttributes.size();
		quad2DVertexInputStateCreateInfo.pVertexAttributeDescriptions = quad2DVertexInputAttributes.data();
		quad2DVertexInputStateCreateInfo.vertexBindingDescriptionCount = quad2DVertexInputBindings.size();
		quad2DVertexInputStateCreateInfo.pVertexBindingDescriptions = quad2DVertexInputBindings.data();

		VkPipelineVertexInputStateCreateInfo basic3DVertexInputStateCreateInfo = {};
		basic3DVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		basic3DVertexInputStateCreateInfo.pNext = NULL;
		basic3DVertexInputStateCreateInfo.flags = 0;
		basic3DVertexInputStateCreateInfo.vertexAttributeDescriptionCount = basic3DVertexInputAttributes.size();
		basic3DVertexInputStateCreateInfo.pVertexAttributeDescriptions = basic3DVertexInputAttributes.data();
		basic3DVertexInputStateCreateInfo.vertexBindingDescriptionCount = basic3DVertexInputBindings.size();
		basic3DVertexInputStateCreateInfo.pVertexBindingDescriptions = basic3DVertexInputBindings.data();

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
		mainRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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

		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
		pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineDepthStencilStateCreateInfo.pNext = NULL;
		pipelineDepthStencilStateCreateInfo.flags = 0;
		pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		pipelineDepthStencilStateCreateInfo.front = {};
		pipelineDepthStencilStateCreateInfo.back = {};
		pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
		pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;

		std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
			m_UniformDescriptorSetLayout,
			m_ImageDescriptorSetLayout
		};
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = NULL;
		pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

		result = vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, NULL, &m_PipelineLayout);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create pipeline layout.");
			return -1;

		}

		VkGraphicsPipelineCreateInfo quad2DPipelineCreateInfo = {};
		quad2DPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		quad2DPipelineCreateInfo.pNext = NULL;
		quad2DPipelineCreateInfo.flags = 0;
		quad2DPipelineCreateInfo.layout = m_PipelineLayout;
		quad2DPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		quad2DPipelineCreateInfo.basePipelineIndex = 0;
		quad2DPipelineCreateInfo.renderPass = m_RenderPass;
		quad2DPipelineCreateInfo.subpass = 0;
		quad2DPipelineCreateInfo.stageCount = 2;
		quad2DPipelineCreateInfo.pStages = quad2DShaderStageCreateInfos;
		quad2DPipelineCreateInfo.pVertexInputState = &quad2DVertexInputStateCreateInfo;
		quad2DPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		quad2DPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		quad2DPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		quad2DPipelineCreateInfo.pRasterizationState = &mainRasterizationStateCreateInfo;
		quad2DPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		quad2DPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		quad2DPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
		quad2DPipelineCreateInfo.pTessellationState = NULL;

		VkGraphicsPipelineCreateInfo basic3DPipelineCreateInfo = quad2DPipelineCreateInfo;
		basic3DPipelineCreateInfo.pStages = basic3DShaderStageCreateInfos;
		basic3DPipelineCreateInfo.pVertexInputState = &basic3DVertexInputStateCreateInfo;

		VkGraphicsPipelineCreateInfo lineQuad2DPipelineCreateInfo = quad2DPipelineCreateInfo;
		lineQuad2DPipelineCreateInfo.pRasterizationState = &lineRasterizationStateCreateInfo;

		VkGraphicsPipelineCreateInfo lineBasic3DPipelineCreateInfo = basic3DPipelineCreateInfo;
		lineBasic3DPipelineCreateInfo.pRasterizationState = &lineRasterizationStateCreateInfo;

		auto pipelineCacheData = m_AssetManager.LoadAsset<PipelineCache>("cache/pipeline.cache");

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipelineCacheCreateInfo.pNext = NULL;
		pipelineCacheCreateInfo.flags = 0;
		pipelineCacheCreateInfo.initialDataSize = pipelineCacheData->data.size();
		pipelineCacheCreateInfo.pInitialData = pipelineCacheData->data.data();

		vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, NULL, &m_PipelineCache);

		VkGraphicsPipelineCreateInfo pipelineCreateInfos[] = {
			quad2DPipelineCreateInfo,
			basic3DPipelineCreateInfo,
			lineQuad2DPipelineCreateInfo,
			lineBasic3DPipelineCreateInfo
		};
		VkPipeline pipelines[4] = {
			VK_NULL_HANDLE,
			VK_NULL_HANDLE,
			VK_NULL_HANDLE,
			VK_NULL_HANDLE
		};
		result = vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 4, pipelineCreateInfos, NULL, pipelines);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to graphics create pipelines.");
			return -1;
		}
		m_MainPipeline = pipelines[0];
		m_LinePipeline = pipelines[1];
		PipelineFlags pipelineFlags = 0;
		m_PipelineMap[pipelineFlags] = pipelines[0];
		pipelineFlags = RENDERER_PIPELINE_FLAG_3D;
		m_PipelineMap[pipelineFlags] = pipelines[1];
		pipelineFlags = RENDERER_PIPELINE_FILL;
		m_PipelineMap[pipelineFlags] = pipelines[2];
		pipelineFlags = RENDERER_PIPELINE_FILL | RENDERER_PIPELINE_FLAG_3D;
		m_PipelineMap[pipelineFlags] = pipelines[3];
		pipelineFlags = 0;
		m_ActivePipeline = m_PipelineMap[pipelineFlags];

		size_t pipelineCacheDataSize;
		vkGetPipelineCacheData(m_Device, m_PipelineCache, &pipelineCacheDataSize, NULL);
		pipelineCacheData->data.resize(pipelineCacheDataSize);
		vkGetPipelineCacheData(m_Device, m_PipelineCache, &pipelineCacheDataSize, pipelineCacheData->data.data());

		m_AssetManager.SaveAsset("cache/pipeline.cache", pipelineCacheData);

		vkDestroyShaderModule(m_Device, quad2DVertexShaderModule, NULL);
		vkDestroyShaderModule(m_Device, quad2DFragmentShaderModule, NULL);
		vkDestroyShaderModule(m_Device, basic3DVertexShaderModule, NULL);
		vkDestroyShaderModule(m_Device, basic3DFragmentShaderModule, NULL);
	}
	{
		for (uint32_t i = 0; i < m_SwapchainImageCount; i ++) {
			VkImageView attachments[] = {
				m_SwapchainImageViews[i],
				m_DepthImage.m_ImageView
			};
			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = NULL;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = m_RenderPass;
			framebufferCreateInfo.layers = 1;
			framebufferCreateInfo.attachmentCount = 2;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = m_SwapchainExtent.width;
			framebufferCreateInfo.height = m_SwapchainExtent.height;

			VkFramebuffer framebuffer;
			result = vkCreateFramebuffer(m_Device, &framebufferCreateInfo, NULL, &framebuffer);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create framebuffer %u.", i);
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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create main command pool.");
			return -1;
		}

		cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.transferIndex.value();
		result = vkCreateCommandPool(m_Device, &cmdPoolCreateInfo, NULL, &m_TransferCmdPool);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create transfer command pool.");
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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to allocate command buffers for draw commands.");
			return -1;
		}
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		m_GeomertyDrawCmdBuffers.resize(m_SwapchainImageCount);
		result = vkAllocateCommandBuffers(m_Device, &cmdBufferAllocateInfo, m_GeomertyDrawCmdBuffers.data());
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to allocate command buffers for geomerty draw commands.");
			return -1;
		}
	}
	{
		m_ImageAvailableSemaphores.reserve(m_Capabilites.maxFramesInFlight);
		m_RenderFinishedSemaphores.reserve(m_Capabilites.maxFramesInFlight);
		m_InFlightFences.reserve(m_Capabilites.maxFramesInFlight);
		for (uint32_t i = 0; i < m_Capabilites.maxFramesInFlight; i++) {
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphoreCreateInfo.pNext = NULL;
			semaphoreCreateInfo.flags = 0;

			VkSemaphore semaphore = VK_NULL_HANDLE;
			result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, NULL, &semaphore);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create image available semaphore %u.", i);
				return -1;
			}
			m_ImageAvailableSemaphores.push_back(semaphore);

			semaphore = VK_NULL_HANDLE;
			result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, NULL, &semaphore);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to render finished semaphore %u.", i);
				return -1;
			}
			m_RenderFinishedSemaphores.push_back(semaphore);

			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.pNext = NULL;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkFence inFlightFence = VK_NULL_HANDLE;
			VkFence graphicsQueueFence = VK_NULL_HANDLE;
			VkFence transferQueueFence = VK_NULL_HANDLE;
			result = vkCreateFence(m_Device, &fenceCreateInfo, NULL, &inFlightFence);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create in flight fence %u.", i);
				return -1;
			}
			m_InFlightFences.push_back(inFlightFence);

			result = vkCreateFence(m_Device, &fenceCreateInfo, NULL, &graphicsQueueFence);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create in flight fence %u.", i);
				return -1;
			}
			m_GraphicsQueueFences.push_back(graphicsQueueFence);

			result = vkCreateFence(m_Device, &fenceCreateInfo, NULL, &transferQueueFence);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create in flight fence %u.", i);
				return -1;
			}
			m_TransferQueueFences.push_back(transferQueueFence);
		}
	}
	{
		auto image = m_AssetManager.LoadAsset<Image>("textures/SVT-ECG.jpg");
		if (image == nullptr) {
			return -1;
		}
		StagingBuffer stagingBuffer = this->CreateStagingBuffer(
			image->width * image->height * 4);

		m_ImageBuffer = this->CreateImageBuffer(image->width,
												image->height,
												IMAGE_FORMAT_R8G8B8A8_SRGB);

		stagingBuffer.SetData(image->width * image->height * 4, 0, image->pixels);

		stagingBuffer.TransferDataImmediate(m_ImageBuffer, 0, 0, image->width, image->height);
		free(image->pixels);
		image.reset();

		m_UniformBuffer = this->CreateUniformBuffer(2 * sizeof(glm::mat4));
		glm::mat4 view(1.0f);

		glm::mat4 perspeciveViewMatrix = glm::perspective(glm::radians(90.0f),
															 float(m_SwapchainExtent.width) /
															 float(m_SwapchainExtent.height),
															 0.001f,
															 256.0f);
		perspeciveViewMatrix[1][1] *= -1.0f;
		stagingBuffer.SetData(sizeof(glm::mat4), 0, glm::value_ptr(view));
		stagingBuffer.SetData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(perspeciveViewMatrix));
		stagingBuffer.TransferDataImmediate(m_UniformBuffer, 0, 0, 2 * sizeof(glm::mat4));
		m_UniformStagingBuffer = this->CreateStagingBuffer(sizeof(glm::mat4) * 2);
		m_UniformStagingBuffer.SetData(sizeof(glm::mat4), 0, glm::value_ptr(view));
		m_UniformStagingBuffer.SetData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(perspeciveViewMatrix));
	}
	// Skybox resources
	{
		// Use width twice to create a cube
		m_Skybox = CubeMapBuffer(m_SwapchainExtent.width, m_SwapchainExtent.width);
		m_Skybox.Clear({ 0.2f, 0.0f, 0.8f, 1.0f });

		m_SkyboxUniformBuffer = CreateUniformBuffer(sizeof(glm::mat4) * 2);

		m_SkyboxVertexBuffer = CreateVertexBuffer(6 * sizeof(glm::vec3));
		StagingBuffer skyboxStagingBuffer = CreateStagingBuffer(6 * sizeof(glm::vec3));

		constexpr glm::vec3 skyboxVertices[] = {
			{ -1.0f,  1.0f,  1.0f }, {  1.0f,  1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f }, // font face
			{  1.0f, -1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f },
		};
		skyboxStagingBuffer.SetData(6 * sizeof(glm::vec3), 0, skyboxVertices);
		skyboxStagingBuffer.TransferDataImmediate(m_SkyboxVertexBuffer, 0, 0, 6 * sizeof(glm::vec3));
		skyboxStagingBuffer = CreateStagingBuffer(sizeof(glm::mat4) * 2);
		glm::mat4 identity(1.0f);
		skyboxStagingBuffer.SetData(sizeof(glm::mat4), 0, glm::value_ptr(identity));
		skyboxStagingBuffer.SetData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(identity));
		skyboxStagingBuffer.TransferDataImmediate(m_SkyboxUniformBuffer, 0, 0, sizeof(glm::mat4) * 2);
		skyboxStagingBuffer = StagingBuffer();


		std::array<VkDescriptorSetLayoutBinding, 2> skyboxDescriptorSetLayoutBidnings;
		skyboxDescriptorSetLayoutBidnings[0].binding = 0;
		skyboxDescriptorSetLayoutBidnings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		skyboxDescriptorSetLayoutBidnings[0].descriptorCount = 1;
		skyboxDescriptorSetLayoutBidnings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		skyboxDescriptorSetLayoutBidnings[0].pImmutableSamplers = NULL;
		skyboxDescriptorSetLayoutBidnings[1].binding = 1;
		skyboxDescriptorSetLayoutBidnings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		skyboxDescriptorSetLayoutBidnings[1].descriptorCount = 1;
		skyboxDescriptorSetLayoutBidnings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		skyboxDescriptorSetLayoutBidnings[1].pImmutableSamplers = NULL;

		VkDescriptorSetLayoutCreateInfo skyboxDescriptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.bindingCount = 2,
			.pBindings = skyboxDescriptorSetLayoutBidnings.data(),
		};

		result = vkCreateDescriptorSetLayout(m_Device,
											 &skyboxDescriptorSetLayoutCreateInfo,
											 NULL,
											 &m_SkyboxDescriptorSetLayout);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to create skybox descriptor set layout.");

		std::array<VkDescriptorPoolSize, 2> skyboxDescriptorPoolSizes;
		skyboxDescriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		skyboxDescriptorPoolSizes[0].descriptorCount = m_Capabilites.maxFramesInFlight;
		skyboxDescriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		skyboxDescriptorPoolSizes[1].descriptorCount = m_Capabilites.maxFramesInFlight;


		VkDescriptorPoolCreateInfo skyboxDescriptorPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.maxSets = 2 * m_Capabilites.maxFramesInFlight,
			.poolSizeCount = skyboxDescriptorPoolSizes.size(),
			.pPoolSizes = skyboxDescriptorPoolSizes.data()
		};

		result = vkCreateDescriptorPool(m_Device, &skyboxDescriptorPoolCreateInfo, NULL, &m_SkyboxDesriptorPool);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to create skybox descriptor pool.");

		std::vector<VkDescriptorSetLayout> skyboxDescriptorSetLayouts(m_Capabilites.maxFramesInFlight, m_SkyboxDescriptorSetLayout);
		m_SkyboxDescriptorSets.resize(m_Capabilites.maxFramesInFlight);

		VkDescriptorSetAllocateInfo skyboxDescriptorSetAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = NULL,
			.descriptorPool = m_SkyboxDesriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(skyboxDescriptorSetLayouts.size()),
			.pSetLayouts = skyboxDescriptorSetLayouts.data()
		};

		result = vkAllocateDescriptorSets(m_Device,
										  &skyboxDescriptorSetAllocateInfo,
										  m_SkyboxDescriptorSets.data());
		CEE_VERIFY(result == VK_SUCCESS, "Failed to allocate skybox descriptor sets.");

		VkSamplerCreateInfo skyboxSamplerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 0.0f,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 1.0f,
			.maxLod = 1.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			.unnormalizedCoordinates = VK_FALSE
		};
		result = vkCreateSampler(m_Device, &skyboxSamplerCreateInfo, NULL, &m_SkyboxSampler);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to create sampler for skybox.");

		VkPipelineLayoutCreateInfo skyboxPipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &m_SkyboxDescriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = NULL
		};

		result = vkCreatePipelineLayout(m_Device,
										&skyboxPipelineLayoutCreateInfo,
										NULL,
										&m_SkyboxPipelineLayout);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to create pipeline layout for skybox");

		VkShaderModule vertexShaderModule = VK_NULL_HANDLE, fragmentShaderModule = VK_NULL_HANDLE;
		std::string skyboxVertexShaderFilePath = "shaders/renderer3DSkyboxVertex.spv";
		std::string skyboxFragmentShaderFilePath = "shaders/renderer3DSkyboxFragment.spv";

		auto skyboxVertexShaderCode = m_AssetManager.LoadAsset<ShaderBinary>(skyboxVertexShaderFilePath);
		auto skyboxFragmentShaderCode = m_AssetManager.LoadAsset<ShaderBinary>(skyboxFragmentShaderFilePath);

		vertexShaderModule = CreateShaderModule(m_Device, skyboxVertexShaderCode);
		fragmentShaderModule = CreateShaderModule(m_Device, skyboxFragmentShaderCode);

		skyboxVertexShaderCode.reset();
		skyboxFragmentShaderCode.reset();

		std::vector<VkPipelineShaderStageCreateInfo> skyboxPipelineShaderStageCrateInfos;
		skyboxPipelineShaderStageCrateInfos.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName = "main",
			.pSpecializationInfo = NULL
		});
		skyboxPipelineShaderStageCrateInfos.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName = "main",
			.pSpecializationInfo = NULL
		});

		VkVertexInputBindingDescription skyboxVertexBindingDescription = {
			.binding = 0,
			.stride = 12,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		VkVertexInputAttributeDescription skyboxVertexInputDescription = {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = 0
		};

		VkPipelineVertexInputStateCreateInfo skyboxVertexInputStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &skyboxVertexBindingDescription,
			.vertexAttributeDescriptionCount = 1,
			.pVertexAttributeDescriptions = &skyboxVertexInputDescription
		};

		VkPipelineInputAssemblyStateCreateInfo skyboxInputAssemblyStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		VkViewport viewport = {
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(m_SwapchainExtent.width),
			.height = static_cast<float>(m_SwapchainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkRect2D scissor = {
			.offset = { 0u, 0u },
			.extent = m_SwapchainExtent
		};

		VkPipelineViewportStateCreateInfo skyboxViewportStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		VkPipelineRasterizationStateCreateInfo skyboxRasterizationStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
		};

		VkPipelineMultisampleStateCreateInfo skyboxMultisampleStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = NULL,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		VkPipelineDepthStencilStateCreateInfo skyboxDepthStencilStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 0.0f
		};

		VkPipelineColorBlendAttachmentState skyboxColorBlendAttachmentState;
		skyboxColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		skyboxColorBlendAttachmentState.blendEnable = VK_FALSE;
		skyboxColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		skyboxColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		skyboxColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		skyboxColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		skyboxColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		skyboxColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		VkPipelineColorBlendStateCreateInfo skyboxColorBlendStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &skyboxColorBlendAttachmentState,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
		};


		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo skyboxDynamicStateCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates
		};

		VkGraphicsPipelineCreateInfo skyboxPipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.stageCount = static_cast<uint32_t>(skyboxPipelineShaderStageCrateInfos.size()),
			.pStages = skyboxPipelineShaderStageCrateInfos.data(),
			.pVertexInputState = &skyboxVertexInputStateCreateInfo,
			.pInputAssemblyState = &skyboxInputAssemblyStateCreateInfo,
			.pTessellationState = NULL,
			.pViewportState = &skyboxViewportStateCreateInfo,
			.pRasterizationState = &skyboxRasterizationStateCreateInfo,
			.pMultisampleState = &skyboxMultisampleStateCreateInfo,
			.pDepthStencilState = &skyboxDepthStencilStateCreateInfo,
			.pColorBlendState = &skyboxColorBlendStateCreateInfo,
			.pDynamicState = &skyboxDynamicStateCreateInfo,
			.layout = m_SkyboxPipelineLayout,
			.renderPass = m_RenderPass,
			.subpass = 0,
			.basePipelineHandle = NULL,
			.basePipelineIndex = 0
		};
		result = vkCreateGraphicsPipelines(m_Device,
										   m_PipelineCache,
										   1,
										   &skyboxPipelineCreateInfo,
										   NULL,
										   &m_SkyboxPipeline);
		CEE_VERIFY(result == VK_SUCCESS, "Failed to create pipeline for skybox.");

		VkDescriptorBufferInfo uniformDescriptor = {
			.buffer = m_SkyboxUniformBuffer.m_Buffer,
			.offset = 0,
			.range = m_SkyboxUniformBuffer.m_Size
		};
		VkDescriptorImageInfo imageSamplerDescriptor = {
			.sampler = m_SkyboxSampler,
			.imageView = m_Skybox.m_ImageView,
			.imageLayout = m_Skybox.m_Layout
		};
		for (uint32_t i = 0; i < m_SkyboxDescriptorSets.size(); i++) {
			VkWriteDescriptorSet writeDescriptorSets[2];
			writeDescriptorSets[0] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = NULL,
				.dstSet = m_SkyboxDescriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pImageInfo = NULL,
				.pBufferInfo = &uniformDescriptor,
				.pTexelBufferView = NULL
			};
			writeDescriptorSets[1] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = NULL,
				.dstSet = m_SkyboxDescriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageSamplerDescriptor,
				.pBufferInfo = NULL,
				.pTexelBufferView = NULL
			};
			vkUpdateDescriptorSets(m_Device, 2, writeDescriptorSets, 0, NULL);
		}
		vkDestroyShaderModule(m_Device, vertexShaderModule, NULL);
		vkDestroyShaderModule(m_Device, fragmentShaderModule, NULL);

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = NULL,
			.commandPool = m_GraphicsCmdPool,
			.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
			.commandBufferCount = m_Capabilites.maxFramesInFlight,
		};
		m_SkyboxDrawCommandBuffers.resize(m_Capabilites.maxFramesInFlight);
		result = vkAllocateCommandBuffers(m_Device,
										  &commandBufferAllocateInfo,
										  m_SkyboxDrawCommandBuffers.data());
		CEE_VERIFY(result == VK_SUCCESS, "Failed to allocate skybox command buffers.");
	}
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_UniformBuffer.m_Buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;


		VkDescriptorImageInfo SVTImageInfo = {};
		SVTImageInfo.sampler = VK_NULL_HANDLE;
		SVTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SVTImageInfo.imageView = m_ImageBuffer.m_ImageView;

		VkDescriptorImageInfo defImageInfo = {};
		defImageInfo.sampler = VK_NULL_HANDLE;
		defImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		defImageInfo.imageView = m_ImageBuffer.m_ImageView;

		std::array<VkDescriptorImageInfo, 32> imageInfos;
		imageInfos[0] = SVTImageInfo;
		imageInfos[1] = defImageInfo;
		imageInfos[2] = defImageInfo;
		imageInfos[3] = defImageInfo;
		imageInfos[4] = defImageInfo;
		imageInfos[5] = defImageInfo;
		imageInfos[6] = defImageInfo;
		imageInfos[7] = defImageInfo;
		imageInfos[8] = defImageInfo;
		imageInfos[9] = defImageInfo;
		imageInfos[10] = defImageInfo;
		imageInfos[11] = defImageInfo;
		imageInfos[12] = defImageInfo;
		imageInfos[13] = defImageInfo;
		imageInfos[14] = defImageInfo;
		imageInfos[15] = defImageInfo;
		imageInfos[16] = defImageInfo;
		imageInfos[17] = defImageInfo;
		imageInfos[18] = defImageInfo;
		imageInfos[19] = defImageInfo;
		imageInfos[20] = defImageInfo;
		imageInfos[21] = defImageInfo;
		imageInfos[22] = defImageInfo;
		imageInfos[23] = defImageInfo;
		imageInfos[24] = defImageInfo;
		imageInfos[25] = defImageInfo;
		imageInfos[26] = defImageInfo;
		imageInfos[27] = defImageInfo;
		imageInfos[28] = defImageInfo;
		imageInfos[29] = defImageInfo;
		imageInfos[30] = defImageInfo;
		imageInfos[31] = defImageInfo;

		VkDescriptorImageInfo samplerInfo = {};
		samplerInfo.sampler = m_Sampler;
		samplerInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		samplerInfo.imageView = VK_NULL_HANDLE;

		VkWriteDescriptorSet writeDescriptorSet = {};
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		for (uint32_t i = 0; i < m_Capabilites.maxFramesInFlight; i++) {
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.pNext = NULL;
			writeDescriptorSet.dstSet = m_UniformDescriptorSets[i];
			writeDescriptorSet.dstBinding = 0;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.pImageInfo = NULL;
			writeDescriptorSet.pBufferInfo = &bufferInfo;
			writeDescriptorSet.pTexelBufferView = NULL;
			writeDescriptorSets.push_back(writeDescriptorSet);

			vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
			writeDescriptorSets.clear();

			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.pNext = NULL;
			writeDescriptorSet.dstSet = m_ImageDescriptorSets[i];
			writeDescriptorSet.dstBinding = 0;
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			writeDescriptorSet.pImageInfo = &samplerInfo;
			writeDescriptorSet.pBufferInfo = NULL;
			writeDescriptorSet.pTexelBufferView = NULL;
			writeDescriptorSets.push_back(writeDescriptorSet);
			writeDescriptorSet.descriptorCount = imageInfos.size();
			writeDescriptorSet.dstArrayElement = 0;
			writeDescriptorSet.dstBinding = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			writeDescriptorSet.pImageInfo = imageInfos.data();
			writeDescriptorSets.push_back(writeDescriptorSet);

			vkUpdateDescriptorSets(m_Device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
			writeDescriptorSets.clear();
		}
		{
			m_QueuedSubmits.resize(3);
		}
	}

	return 0;
}

void Renderer::Shutdown()
{
	m_ImageBuffer = ImageBuffer();
	m_UniformStagingBuffer = StagingBuffer();
	m_UniformBuffer = UniformBuffer();
	m_Skybox = CubeMapBuffer();
	m_SkyboxUniformBuffer = UniformBuffer();
	m_SkyboxVertexBuffer = VertexBuffer();
	vkDestroySampler(m_Device, m_SkyboxSampler, NULL);
	vkDestroyPipeline(m_Device, m_SkyboxPipeline, NULL);
	vkDestroyPipelineLayout(m_Device, m_SkyboxPipelineLayout, NULL);
	vkDestroyDescriptorPool(m_Device, m_SkyboxDesriptorPool, NULL);
	vkDestroyDescriptorSetLayout(m_Device, m_SkyboxDescriptorSetLayout, NULL);
	vkFreeCommandBuffers(m_Device, m_GraphicsCmdPool, m_SkyboxDrawCommandBuffers.size(), m_SkyboxDrawCommandBuffers.data());
	m_Running.store(false, std::memory_order_relaxed);
	for (auto& fence : m_TransferQueueFences) {
		vkDestroyFence(m_Device, fence, NULL);
	}
	for (auto& fence : m_GraphicsQueueFences) {
		vkDestroyFence(m_Device, fence, NULL);
	}
	for (auto& fence : m_InFlightFences) {
		vkDestroyFence(m_Device, fence, NULL);
	}
	for (auto& semaphore : m_RenderFinishedSemaphores) {
		vkDestroySemaphore(m_Device, semaphore, NULL);
	}
	for (auto& semaphore : m_ImageAvailableSemaphores) {
		vkDestroySemaphore(m_Device, semaphore, NULL);
	}
	vkFreeCommandBuffers(m_Device, m_GraphicsCmdPool, m_DrawCmdBuffers.size(), m_DrawCmdBuffers.data());
	vkDestroyCommandPool(m_Device, m_TransferCmdPool, NULL);
	vkDestroyCommandPool(m_Device, m_GraphicsCmdPool, NULL);
	for (auto& framebuffer : m_Framebuffers) {
		vkDestroyFramebuffer(m_Device, framebuffer, NULL);
	}
	for (auto& pipeline : m_PipelineMap) {
		vkDestroyPipeline(m_Device, pipeline.second, NULL);
	}
	vkDestroyPipelineCache(m_Device, m_PipelineCache, NULL);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, NULL);
	vkDestroySampler(m_Device, m_Sampler, NULL);
	vkDestroyDescriptorPool(m_Device, m_DescriptorPool, NULL);
	vkDestroyDescriptorSetLayout(m_Device, m_ImageDescriptorSetLayout, NULL);
	vkDestroyDescriptorSetLayout(m_Device, m_UniformDescriptorSetLayout, NULL);
	vkDestroyRenderPass(m_Device, m_RenderPass, NULL);
	for (auto& imageView : m_SwapchainImageViews) {
		vkDestroyImageView(m_Device, imageView, NULL);
	}
	m_DepthImage = ImageBuffer();
	vkDestroySwapchainKHR(m_Device, m_Swapchain, NULL);
	vkDestroySurfaceKHR(m_Instance, m_Surface, NULL);
	vkDestroyDevice(m_Device, NULL);
#ifndef NDEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXTfn =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXTfn)
		vkDestroyDebugUtilsMessengerEXTfn(m_Instance, m_DebugMessenger, NULL);
#endif
	vkDestroyInstance(m_Instance, NULL);
}

void Renderer::Clear(const glm::vec4& clearColor) {
	m_ClearColor = clearColor;
}

int Renderer::StartFrame() {
	ZoneScoped;
	VkResult result;

	if (m_RecreateSwapchain) {
		InvalidateSwapchain();
	}

	try {
		m_ActivePipeline = m_PipelineMap.at(RENDERER_PIPELINE_FLAG_3D);
	} catch (std::out_of_range& e) {
		m_ActivePipeline = m_PipelineMap[0];
	}

	VkFence waitFences[] = {
		m_InFlightFences[m_FrameIndex]
	};
	vkWaitForFences(m_Device, 1, waitFences, VK_TRUE, UINT64_MAX);
retryAqurireNextImage:
	result = vkAcquireNextImageKHR(m_Device,
								   m_Swapchain,
								   UINT64_MAX,
								   m_ImageAvailableSemaphores[m_FrameIndex],
								   VK_NULL_HANDLE,
								   &m_ImageIndex);
	if (result == VK_SUBOPTIMAL_KHR) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO,
										 "Suboptimal KHR... Will recreate swapchain before next frame.");
		m_RecreateSwapchain = true;
	} else if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		InvalidateSwapchain();
		goto retryAqurireNextImage;
	} else if (result != VK_SUCCESS) {
		m_RecreateSwapchain = true;
		return -1;
	}

	vkResetFences(m_Device, 1, waitFences);

	{
		vkResetCommandBuffer(m_SkyboxDrawCommandBuffers[m_FrameIndex], 0);

		VkCommandBufferInheritanceInfo inheritanceInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
				.pNext = NULL,
				.renderPass = m_RenderPass,
				.subpass = 0,
				.framebuffer = m_Framebuffers[m_ImageIndex],
				.occlusionQueryEnable = VK_FALSE,
				.queryFlags = 0,
				.pipelineStatistics = 0
			};
			VkCommandBufferBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = NULL,
				.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
						 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				.pInheritanceInfo = &inheritanceInfo
			};

			result = vkBeginCommandBuffer(m_SkyboxDrawCommandBuffers[m_FrameIndex], &beginInfo);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to begin command buffer for drawing skybox.");

			vkCmdBindPipeline(m_SkyboxDrawCommandBuffers[m_FrameIndex],
							  VK_PIPELINE_BIND_POINT_GRAPHICS,
							  m_SkyboxPipeline);

			VkViewport viewport = {
				.x = 0,
				.y = 0,
				.width = static_cast<float>(m_SwapchainExtent.width),
				.height = static_cast<float>(m_SwapchainExtent.height),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			};
			VkRect2D scissor = {
				.offset = { 0, 0 },
				.extent = m_SwapchainExtent
			};
			vkCmdSetViewport(m_SkyboxDrawCommandBuffers[m_FrameIndex], 0, 1, &viewport);
			vkCmdSetScissor(m_SkyboxDrawCommandBuffers[m_FrameIndex], 0, 1, &scissor);

			VkDescriptorSet descriptorSetsToBind[] = {
				m_SkyboxDescriptorSets[m_FrameIndex]
			};
			vkCmdBindDescriptorSets(m_SkyboxDrawCommandBuffers[m_FrameIndex],
									VK_PIPELINE_BIND_POINT_GRAPHICS,
									m_SkyboxPipelineLayout,
									0,
									1, descriptorSetsToBind,
									0, NULL);

			VkDeviceSize offsets[] = {
				0
			};
			vkCmdBindVertexBuffers(m_SkyboxDrawCommandBuffers[m_FrameIndex],
								   0,
								   1, &m_SkyboxVertexBuffer.m_Buffer,
								   offsets);

			vkCmdDraw(m_SkyboxDrawCommandBuffers[m_FrameIndex],
					  6, 1, 0, 0);

			result = vkEndCommandBuffer(m_SkyboxDrawCommandBuffers[m_FrameIndex]);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to record command buffer for skybox");
	}

	{
		vkResetCommandBuffer(m_GeomertyDrawCmdBuffers[m_FrameIndex], 0);

		VkCommandBufferInheritanceInfo inheritanceInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
				.pNext = NULL,
				.renderPass = m_RenderPass,
				.subpass = 0,
				.framebuffer = m_Framebuffers[m_ImageIndex],
				.occlusionQueryEnable = VK_FALSE,
				.queryFlags = 0,
				.pipelineStatistics = 0
			};
			VkCommandBufferBeginInfo beginInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = NULL,
				.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
						 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				.pInheritanceInfo = &inheritanceInfo
			};

			result = vkBeginCommandBuffer(m_GeomertyDrawCmdBuffers[m_FrameIndex], &beginInfo);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to begin command buffer for drawing skybox.");

		std::array<VkDescriptorSet, 2> descriptorSets = {
			m_UniformDescriptorSets[m_FrameIndex],
			m_ImageDescriptorSets[m_FrameIndex]
		};
		vkCmdBindDescriptorSets(m_GeomertyDrawCmdBuffers[m_FrameIndex],
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								m_PipelineLayout,
								0,
								descriptorSets.size(),
								descriptorSets.data(),
								0, NULL);
		vkCmdBindPipeline(m_GeomertyDrawCmdBuffers[m_FrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_ActivePipeline);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = m_SwapchainExtent.width;
		viewport.height = m_SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor = {};
		scissor.extent = m_SwapchainExtent;
		scissor.offset = { 0, 0 };

		vkCmdSetScissor(m_GeomertyDrawCmdBuffers[m_FrameIndex], 0, 1, &scissor);
		vkCmdSetViewport(m_GeomertyDrawCmdBuffers[m_FrameIndex], 0, 1, &viewport);
	}

	vkResetCommandBuffer(m_DrawCmdBuffers[m_FrameIndex], 0);

	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = NULL;
	cmdBeginInfo.flags = 0;
	cmdBeginInfo.pInheritanceInfo = NULL;

	m_InFrame = true;
	result = vkBeginCommandBuffer(m_DrawCmdBuffers[m_FrameIndex], &cmdBeginInfo);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Failed to beigin command buffer.");
		return -1;
	}

	std::array<VkClearValue, 2> clearValues;
	memcpy(clearValues[0].color.float32, glm::value_ptr(m_ClearColor), sizeof(glm::vec4));
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	renderPassBeginInfo.framebuffer = m_Framebuffers[m_ImageIndex];
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = m_SwapchainExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(m_DrawCmdBuffers[m_FrameIndex],
						 &renderPassBeginInfo,
						 VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	std::array<VkCommandBuffer, 1> SecondaryCommandBuffers = {
		m_SkyboxDrawCommandBuffers[m_FrameIndex]
	};
	vkCmdExecuteCommands(m_DrawCmdBuffers[m_FrameIndex],
						SecondaryCommandBuffers.size(),
						SecondaryCommandBuffers.data());


	return 0;
}

int Renderer::EndFrame() {
	ZoneScoped;
	VkResult result = VK_SUCCESS;

	FlushQueuedSubmits();

	{
		vkEndCommandBuffer(m_GeomertyDrawCmdBuffers[m_FrameIndex]);

		std::array<VkCommandBuffer, 1> SecondaryCommandBuffers = {
			m_GeomertyDrawCmdBuffers[m_FrameIndex]
		};
		vkCmdExecuteCommands(m_DrawCmdBuffers[m_FrameIndex],
							SecondaryCommandBuffers.size(),
							SecondaryCommandBuffers.data());
	}
	{
		ZoneScoped;
		ZoneNamed(EndFrameResources, true);
		vkCmdEndRenderPass(m_DrawCmdBuffers[m_FrameIndex]);

		vkEndCommandBuffer(m_DrawCmdBuffers[m_FrameIndex]);
	}
	VkCommandBuffer commandBuffersForSubmission[] = {
		m_DrawCmdBuffers[m_FrameIndex]
	};

	VkSemaphore commandBufferSignalSemaphores[] = {
		m_RenderFinishedSemaphores[m_FrameIndex]
	};
	VkSemaphore commandBufferWaitSemaphores[] = {
		m_ImageAvailableSemaphores[m_FrameIndex]
	};
	VkPipelineStageFlags commandBufferWaitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	{
		ZoneScoped;
		ZoneNamed(GraphicsSubmit, true);
		VkSubmitInfo commandBufferSubmitInfo = {};
		commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		commandBufferSubmitInfo.pNext = NULL;
		commandBufferSubmitInfo.commandBufferCount = 1;
		commandBufferSubmitInfo.pCommandBuffers = commandBuffersForSubmission;
		commandBufferSubmitInfo.signalSemaphoreCount = sizeof(commandBufferSignalSemaphores)/sizeof(commandBufferSignalSemaphores[0]);
		commandBufferSubmitInfo.pSignalSemaphores = commandBufferSignalSemaphores;
		commandBufferSubmitInfo.waitSemaphoreCount = sizeof(commandBufferWaitSemaphores)/sizeof(commandBufferWaitSemaphores[0]);
		commandBufferSubmitInfo.pWaitSemaphores = commandBufferWaitSemaphores;
		commandBufferSubmitInfo.pWaitDstStageMask = commandBufferWaitStages;

		result = vkQueueSubmit(m_GraphicsQueue, 1, &commandBufferSubmitInfo, m_InFlightFences[m_FrameIndex]);
		if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											"Failed to submit geometry command buffer to graphics queue.");
		}
	}
	{
		ZoneScoped;
		ZoneNamed(PresentSubmit, true);
		VkSemaphore presentWaitSemaphores[] = {
			m_RenderFinishedSemaphores[m_FrameIndex]
		};

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = NULL;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_ImageIndex;
		presentInfo.pResults = NULL;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = presentWaitSemaphores;

		result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (result == VK_SUBOPTIMAL_KHR) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO,
											"Suboptimal KHR... Will recreate swapchain before next frame.");
			m_RecreateSwapchain = true;
		} else if (result != VK_SUCCESS) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											"Failed to queue present.");
		}
	}

	if (++m_FrameIndex >= m_Capabilites.maxFramesInFlight)
		m_FrameIndex = 0;

	FrameMark;

	return 0;
}

int Renderer::Draw(const IndexBuffer& indexBuffer, const VertexBuffer& vertexBuffer, uint32_t indexCount) {
	ZoneScoped;

	vkCmdBindIndexBuffer(m_GeomertyDrawCmdBuffers[m_FrameIndex], indexBuffer.m_Buffer, 0, VK_INDEX_TYPE_UINT32);
	size_t offset = 0;
	vkCmdBindVertexBuffers(m_GeomertyDrawCmdBuffers[m_FrameIndex], 0, 1, &vertexBuffer.m_Buffer, &offset);

	vkCmdDrawIndexed(m_GeomertyDrawCmdBuffers[m_FrameIndex], indexCount, 1, 0, 0, 0);

	return 0;
}

int Renderer::UpdateCamera(Camera& camera) {
	ZoneScoped;

	VkFence WaitFences[] = { m_InFlightFences[m_FrameIndex] };
	vkWaitForFences(m_Device, 1, WaitFences, VK_TRUE, UINT64_MAX);

	m_UniformStagingBuffer.SetData(sizeof(glm::mat4), 0, glm::value_ptr(camera.GetTransform()));
	m_UniformStagingBuffer.SetData(sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(camera.GetProjection()));
	m_UniformStagingBuffer.TransferData(m_UniformBuffer, 0, 0, sizeof(glm::mat4) * 2);
	m_UniformStagingBuffer.TransferData(m_SkyboxUniformBuffer, 0, 0, sizeof(glm::mat4) * 2);

	VkDescriptorBufferInfo projectionBufferInfo = {};
	projectionBufferInfo.buffer= m_UniformBuffer.m_Buffer;
	projectionBufferInfo.range = sizeof(glm::mat4) * 2;
	projectionBufferInfo.offset = 0;

	VkWriteDescriptorSet projectionDescriptorWrite = {};
	projectionDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	projectionDescriptorWrite.pNext = NULL;
	projectionDescriptorWrite.dstSet = m_UniformDescriptorSets[m_FrameIndex];
	projectionDescriptorWrite.dstBinding = 0;
	projectionDescriptorWrite.dstArrayElement = 0;
	projectionDescriptorWrite.descriptorCount = 1;
	projectionDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	projectionDescriptorWrite.pImageInfo = NULL;
	projectionDescriptorWrite.pBufferInfo = &projectionBufferInfo;
	projectionDescriptorWrite.pTexelBufferView = NULL;

	VkDescriptorBufferInfo skyboxProjectionBufferInfo = {};
	skyboxProjectionBufferInfo.buffer= m_SkyboxUniformBuffer.m_Buffer;
	skyboxProjectionBufferInfo.range = sizeof(glm::mat4) * 2;
	skyboxProjectionBufferInfo.offset = 0;

	VkWriteDescriptorSet skyboxProjectionDescriptorWrite = {};
	skyboxProjectionDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	skyboxProjectionDescriptorWrite.pNext = NULL;
	skyboxProjectionDescriptorWrite.dstSet = m_SkyboxDescriptorSets[m_FrameIndex];
	skyboxProjectionDescriptorWrite.dstBinding = 0;
	skyboxProjectionDescriptorWrite.dstArrayElement = 0;
	skyboxProjectionDescriptorWrite.descriptorCount = 1;
	skyboxProjectionDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	skyboxProjectionDescriptorWrite.pImageInfo = NULL;
	skyboxProjectionDescriptorWrite.pBufferInfo = &skyboxProjectionBufferInfo;
	skyboxProjectionDescriptorWrite.pTexelBufferView = NULL;

	VkWriteDescriptorSet writeDescriptorSets[] = {
		projectionDescriptorWrite,
		skyboxProjectionDescriptorWrite
	};

	vkUpdateDescriptorSets(m_Device, 2, writeDescriptorSets, 0, NULL);

	return 0;
}

void Renderer::UpdateSkybox(CubeMapBuffer& newSkybox) {
	VkDescriptorImageInfo imageInfo = {
		.sampler = m_SkyboxSampler,
		.imageView = newSkybox.m_ImageView,
		.imageLayout = newSkybox.m_Layout
	};

	const uint32_t loopEntry = m_FrameIndex >= m_Capabilites.maxFramesInFlight ? 0 : m_FrameIndex + 1;
	uint32_t i = loopEntry;

	do {
		VkFence waitFence = m_InFlightFences[i];
		vkWaitForFences(m_Device, 1, &waitFence, VK_TRUE, UINT64_MAX);
		VkWriteDescriptorSet writeDescriptorSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = NULL,
			.dstSet = m_SkyboxDescriptorSets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType  =VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo,
			.pBufferInfo = NULL,
			.pTexelBufferView = NULL
		};

		vkUpdateDescriptorSets(m_Device, 1, &writeDescriptorSet, 0, NULL);

		if (++i >= m_Capabilites.maxFramesInFlight)
			i = 0;
	} while(i != loopEntry);

	m_Skybox = std::move(newSkybox);
}

uint32_t Renderer::GetQueueFamilyIndex(CommandQueueType queueType) const {
	switch (queueType) {
	case QUEUE_GRAPHICS:
		return m_QueueFamilyIndices.graphicsIndex.value_or(VK_QUEUE_FAMILY_IGNORED);
	case QUEUE_TRANSFER:
		return m_QueueFamilyIndices.transferIndex.value_or(VK_QUEUE_FAMILY_IGNORED);
	case QUEUE_COMPUTE:
		return m_QueueFamilyIndices.computeIndex.value_or(VK_QUEUE_FAMILY_IGNORED);
	default:
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Attempting to get queue family index of unknown type.");
		return VK_QUEUE_FAMILY_IGNORED;
	}
}

void Renderer::InvalidateSwapchain() {
	ZoneScoped;
	VkSwapchainKHR oldSwapchain = m_Swapchain;
	VkResult result = vkWaitForFences(m_Device,
									  m_InFlightFences.size(),
									  m_InFlightFences.data(),
									  VK_TRUE, UINT64_MAX);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING, "Failed to wait for fences before recreating swapchain.");
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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Swapchain not adequate.");
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
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create swapchain.");
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
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create image views for the swapchain.");
				return;
			}
			m_SwapchainImageViews.push_back(imageView);
		}

		m_DepthImage = this->CreateImageBuffer(m_SwapchainExtent.width,
											   m_SwapchainExtent.height,
											   IMAGE_FORMAT_DEPTH);

		m_Framebuffers.clear();
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++) {
			VkImageView attachments[] = {
				m_SwapchainImageViews[i],
				m_DepthImage.m_ImageView
			};
			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = NULL;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = m_RenderPass;
			framebufferCreateInfo.layers = 1;
			framebufferCreateInfo.attachmentCount = 2;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = m_SwapchainExtent.width;
			framebufferCreateInfo.height = m_SwapchainExtent.height;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			result = vkCreateFramebuffer(m_Device, &framebufferCreateInfo, NULL, &framebuffer);
			if (result != VK_SUCCESS) {
				DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create framebuffer %u.", i);
				return;
			}
			m_Framebuffers.push_back(framebuffer);
		}

		vkDestroySwapchainKHR(m_Device, oldSwapchain, NULL);
		m_RecreateSwapchain = false;
}

void Renderer::InvalidatePipeline() {

}

VkResult Renderer::ImmediateSubmit(std::function<void(RawCommandBuffer&)> fn, CommandQueueType queueType) {
	ZoneScoped;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	if (queueType == QUEUE_GRAPHICS) {
		commandBufferAllocateInfo.commandPool = m_GraphicsCmdPool;
	} else if (queueType == QUEUE_TRANSFER) {
		commandBufferAllocateInfo.commandPool = m_TransferCmdPool;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Queue type not supported.");
		return VK_ERROR_UNKNOWN;
	}
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	VkCommandBuffer commandBuffer;

	VkResult result = vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, &commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to allocate command buffer for immedate submission.");
		return result;
	}

	result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to begin command buffer for immedate submission.");
		return result;
	}

	RawCommandBuffer cb;
	cb.commandBuffer = commandBuffer;
	cb.queueType = queueType;
	
	fn(cb);
	
	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to end command buffer for immedate submission.");
		return result;
	}

	VkSubmitInfo commandBufferSubmitInfo = {};
	commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	commandBufferSubmitInfo.pNext = NULL;
	commandBufferSubmitInfo.commandBufferCount = 1;
	commandBufferSubmitInfo.pCommandBuffers = &commandBuffer;
	commandBufferSubmitInfo.signalSemaphoreCount = 0;
	commandBufferSubmitInfo.pSignalSemaphores = NULL;
	commandBufferSubmitInfo.waitSemaphoreCount = 0;
	commandBufferSubmitInfo.pWaitSemaphores = NULL;
	commandBufferSubmitInfo.pWaitDstStageMask = NULL;

	VkQueue queue;
	if (queueType == QUEUE_GRAPHICS) {
		queue = m_GraphicsQueue;
	} else if (queueType == QUEUE_TRANSFER) {
		queue = m_TransferQueue;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Queue type not supported.");
		return VK_ERROR_UNKNOWN;
	}
	result = vkQueueSubmit(queue, 1, &commandBufferSubmitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to submit immedate command buffer.");
		return result;
	}

	vkQueueWaitIdle(queue);

	return VK_SUCCESS;
}

VkResult Renderer::QueueSubmit(std::function<void(RawCommandBuffer&)> fn, CommandQueueType queueType) {
	ZoneScoped;
	VkResult result = VK_SUCCESS;

	BakedCommandBuffer bakedCommandBuffer;
	bakedCommandBuffer.queueType = queueType;

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	if (queueType == QUEUE_GRAPHICS) {
		commandBufferAllocateInfo.commandPool = m_GraphicsCmdPool;
	} else if (queueType == QUEUE_TRANSFER) {
		commandBufferAllocateInfo.commandPool = m_TransferCmdPool;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Queue type not supported.");
		return VK_ERROR_UNKNOWN;
	}
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	result = vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, &bakedCommandBuffer.commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to allocate command buffer for immedate submission.");
		return result;
	}

	result = vkBeginCommandBuffer(bakedCommandBuffer.commandBuffer, &commandBufferBeginInfo);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to begin command buffer for immedate submission.");
		return result;
	}

	RawCommandBuffer cb;
	cb.commandBuffer = bakedCommandBuffer.commandBuffer;
	cb.queueType = bakedCommandBuffer.queueType;

	fn(cb);

	result = vkEndCommandBuffer(bakedCommandBuffer.commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to end command buffer for immedate submission.");
		return result;
	}

	m_QueuedSubmits[m_FrameIndex].push_back(bakedCommandBuffer);

	return result;
}

VkResult Renderer::FlushQueuedSubmits() {
	ZoneScoped;
	VkResult result = VK_SUCCESS;

	std::vector<VkCommandBuffer> transferCommandBuffers;
	std::vector<VkCommandBuffer> graphicsCommandBuffers;

	for (const auto& bakedCommandBuffer : m_QueuedSubmits[m_FrameIndex]) {
		if (bakedCommandBuffer.queueType == QUEUE_TRANSFER) {
			transferCommandBuffers.push_back(bakedCommandBuffer.commandBuffer);
		} else if (bakedCommandBuffer.queueType == QUEUE_GRAPHICS) {
			graphicsCommandBuffers.push_back(bakedCommandBuffer.commandBuffer);
		} else {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Submitting queue of invalid type.");
			continue;
		}
	}

	VkFence waitFences[] = {
		m_GraphicsQueueFences[m_QueueSubmissionIndex],
		m_TransferQueueFences[m_QueueSubmissionIndex]
	};
	vkWaitForFences(m_Device, 2, waitFences, VK_TRUE, std::numeric_limits<uint64_t>::max());
	result = vkResetFences(m_Device, 2, waitFences);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to reset fences for submission queue.");
	}

	VkSubmitInfo transferCommandBufferSubmitInfo = {};
	transferCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	transferCommandBufferSubmitInfo.pNext = NULL;
	transferCommandBufferSubmitInfo.commandBufferCount = transferCommandBuffers.size();
	transferCommandBufferSubmitInfo.pCommandBuffers = transferCommandBuffers.data();
	transferCommandBufferSubmitInfo.signalSemaphoreCount = 0;
	transferCommandBufferSubmitInfo.pSignalSemaphores = NULL;
	transferCommandBufferSubmitInfo.waitSemaphoreCount = 0;
	transferCommandBufferSubmitInfo.pWaitSemaphores = NULL;
	transferCommandBufferSubmitInfo.pWaitDstStageMask = NULL;

	result = vkQueueSubmit(m_TransferQueue, 1, &transferCommandBufferSubmitInfo, m_TransferQueueFences[m_QueueSubmissionIndex]);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to submit immedate command buffer.");
		return result;
	}

	VkSubmitInfo graphicsCommandBufferSubmitInfo = {};
	graphicsCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	graphicsCommandBufferSubmitInfo.pNext = NULL;
	graphicsCommandBufferSubmitInfo.commandBufferCount = graphicsCommandBuffers.size();
	graphicsCommandBufferSubmitInfo.pCommandBuffers = graphicsCommandBuffers.data();
	graphicsCommandBufferSubmitInfo.signalSemaphoreCount = 0;
	graphicsCommandBufferSubmitInfo.pSignalSemaphores = NULL;
	graphicsCommandBufferSubmitInfo.waitSemaphoreCount = 0;
	graphicsCommandBufferSubmitInfo.pWaitSemaphores = NULL;
	graphicsCommandBufferSubmitInfo.pWaitDstStageMask = NULL;

	result = vkQueueSubmit(m_GraphicsQueue, 1, &graphicsCommandBufferSubmitInfo, m_GraphicsQueueFences[m_QueueSubmissionIndex]);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to submit immedate command buffer.");
		return result;
	}
	m_QueuedSubmits[m_FrameIndex].clear();

	std::vector<VkCommandBuffer> finishedTransferCommandBuffers;
	std::vector<VkCommandBuffer> finishedGraphicsCommandBuffers;
	auto it = m_CommandBufferDeletionQueue.begin();
	while (it != m_CommandBufferDeletionQueue.end()) {
		if (it->age > (m_Capabilites.maxFramesInFlight)) {
			if ((*it).queueType == QUEUE_TRANSFER) {
				finishedTransferCommandBuffers.push_back(it->commandBuffer);
			} else if ((*it).queueType == QUEUE_GRAPHICS) {
				finishedGraphicsCommandBuffers.push_back(it->commandBuffer);
			}
			it = m_CommandBufferDeletionQueue.erase(it);
		} else {
			it->age++;
			it++;
		}
	}
	if (!finishedTransferCommandBuffers.empty()) {
		vkFreeCommandBuffers(m_Device, m_TransferCmdPool, finishedTransferCommandBuffers.size(), finishedTransferCommandBuffers.data());
	}
	if (!finishedGraphicsCommandBuffers.empty()) {
		vkFreeCommandBuffers(m_Device, m_GraphicsCmdPool, finishedGraphicsCommandBuffers.size(), finishedGraphicsCommandBuffers.data());
	}

	for (auto& commandBuffer : transferCommandBuffers) {
		m_CommandBufferDeletionQueue.push_back({ commandBuffer, QUEUE_TRANSFER, 0 });
	}
	for (auto& commandBuffer : graphicsCommandBuffers) {
		m_CommandBufferDeletionQueue.push_back({ commandBuffer, QUEUE_GRAPHICS, 0 });
	}

	m_QueueSubmissionIndex++;
	m_QueueSubmissionIndex %= m_Capabilites.maxFramesInFlight;

	return result;
}

VkCommandBuffer Renderer::StartCommandBuffer(CommandQueueType queueType) {
	ZoneScoped;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	if (queueType == QUEUE_GRAPHICS) {
		commandBufferAllocateInfo.commandPool = m_GraphicsCmdPool;
	} else if (queueType == QUEUE_TRANSFER) {
		commandBufferAllocateInfo.commandPool = m_TransferCmdPool;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Queue type not supported.");
		return VK_NULL_HANDLE;
	}
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = NULL;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = NULL;

	VkCommandBuffer commandBuffer;

	VkResult result = vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, &commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to allocate command buffer for immedate submission.");
		return VK_NULL_HANDLE;
	}

	result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to begin command buffer for immedate submission.");
		return VK_NULL_HANDLE;
	}

	return commandBuffer;
}

VkResult Renderer::SubmitCommandBufferNow(VkCommandBuffer commandBuffer, CommandQueueType queueType) {
	ZoneScoped;
	VkResult result = VK_SUCCESS;

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Failed to end command buffer for immedate submit.");
		return result;
	}

	VkSubmitInfo commandBufferSubmitInfo = {};
	commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	commandBufferSubmitInfo.pNext = NULL;
	commandBufferSubmitInfo.commandBufferCount = 1;
	commandBufferSubmitInfo.pCommandBuffers = &commandBuffer;
	commandBufferSubmitInfo.signalSemaphoreCount = 0;
	commandBufferSubmitInfo.pSignalSemaphores = NULL;
	commandBufferSubmitInfo.waitSemaphoreCount = 0;
	commandBufferSubmitInfo.pWaitSemaphores = NULL;
	commandBufferSubmitInfo.pWaitDstStageMask = NULL;

	VkQueue queue;
	if (queueType == QUEUE_GRAPHICS) {
		queue = m_GraphicsQueue;
	} else if (queueType == QUEUE_TRANSFER) {
		queue = m_TransferQueue;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Queue type not supported.");
		return VK_ERROR_UNKNOWN;
	}
	result = vkQueueSubmit(queue, 1, &commandBufferSubmitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to submit immedate command buffer.");
		return result;
	}

	vkQueueWaitIdle(queue);

	VkCommandPool pool;
	if (queueType == QUEUE_GRAPHICS) {
		pool = m_GraphicsCmdPool;
	} else if (queueType == QUEUE_TRANSFER) {
		pool = m_TransferCmdPool;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Queue type not supported.");
		return VK_ERROR_UNKNOWN;
	}
	vkFreeCommandBuffers(m_Device, pool, 1, &commandBuffer);
	return result;
}

void Renderer::QueueCommandBuffer(VkCommandBuffer commandBuffer, CommandQueueType queueType) {
	ZoneScoped;
	VkResult result;

	result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Failed to end command buffer for queued submit.");
		return;
	}

	BakedCommandBuffer bakedCommandBuffer;
	bakedCommandBuffer.commandBuffer = commandBuffer;
	bakedCommandBuffer.queueType = queueType;

	m_QueuedSubmits[m_FrameIndex].push_back(bakedCommandBuffer);
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
	DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Unable to find suitable memory type.");
	return UINT32_MAX;
}

VkShaderModule Renderer::CreateShaderModule(VkDevice device, std::shared_ptr<ShaderBinary> code) {
	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = NULL;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = code->spvCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(code->spvCode.data());

	VkShaderModule shader;

	VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &shader);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create shader module.");
		return VK_NULL_HANDLE;
	}
	return shader;
}

VkShaderModule Renderer::CreateShaderModule(VkDevice device, std::shared_ptr<ShaderCode> code) {
	DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "NOT YET IMPLEMENTED!");
	raise(SIGINT);
	
	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = NULL;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = code->glslCode.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(code->glslCode.data());

	VkShaderModule shader;

	VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &shader);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create shader module.");
		return VK_NULL_HANDLE;
	}
	return shader;
}

VkFormat Renderer::ChooseDepthFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tilingMode, VkFormatFeatureFlags features) {
	for (auto& format : candidates) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

		if (tilingMode == VK_IMAGE_TILING_LINEAR &&
			(formatProperties.linearTilingFeatures & features) == features) {
			return format;
		} else if (tilingMode == VK_IMAGE_TILING_OPTIMAL &&
			(formatProperties.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to find supported depth format.");
	return VK_FORMAT_MAX_ENUM;
}

VkResult Renderer::CreateImageObjects(VkImage* image, VkDeviceMemory* deviceMemory, VkImageView* imageView,
									  VkFormat format, VkImageUsageFlags usage,
									  uint32_t* width, uint32_t* height, size_t* size,
									  uint32_t mipLevels, uint32_t layers)
{
	VkResult result = VK_SUCCESS;

	VkImageCreateInfo imageCreateInfo = {};
	VkMemoryRequirements imageMemoryRequirements = {};
	VkMemoryAllocateInfo imageAlloacteInfo = {};
	VkImageViewCreateInfo imageViewCreateInfo = {};

	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.flags = layers == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent = { *width, *height, 1 };
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = layers;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	result = vkCreateImage(m_Device, &imageCreateInfo, NULL, image);
	CEE_VERIFY(result == VK_SUCCESS, "Failed to create components for image. create image.");

	vkGetImageMemoryRequirements(m_Device, *image, &imageMemoryRequirements);

	imageAlloacteInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAlloacteInfo.pNext = NULL;
	imageAlloacteInfo.allocationSize = imageMemoryRequirements.size;
	imageAlloacteInfo.memoryTypeIndex = ChooseMemoryType(imageMemoryRequirements.memoryTypeBits,
														 m_PhysicalDeviceMemoryProperties,
														 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	result = vkAllocateMemory(m_Device, &imageAlloacteInfo, NULL, deviceMemory);
	CEE_VERIFY(result == VK_SUCCESS, "Failed to create components for image. alloc.");

	result = vkBindImageMemory(m_Device, *image, *deviceMemory, 0);
	CEE_VERIFY(result == VK_SUCCESS, "Failed to create components for image. bind.");

	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = *image;
	imageViewCreateInfo.viewType = layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components = {
		.r = VK_COMPONENT_SWIZZLE_R,
		.g = VK_COMPONENT_SWIZZLE_G,
		.b = VK_COMPONENT_SWIZZLE_B,
		.a = VK_COMPONENT_SWIZZLE_A
	};
	imageViewCreateInfo.subresourceRange = {
		.aspectMask = format == m_DepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = layers
	};

	result = vkCreateImageView(m_Device, &imageViewCreateInfo, NULL, imageView);
	CEE_VERIFY(result == VK_SUCCESS, "Failed to create components for image. create image view.");

	*size = imageAlloacteInfo.allocationSize;

	return result;
}

int Renderer::TransitionImageLayout(VkImage image,
									VkImageLayout oldLayout,
									VkImageLayout newLayout)
{
	VkResult result = VK_SUCCESS;

	VkImageSubresourceRange imageSubresourceRange = {};
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.levelCount = 1;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.layerCount = 1;
	imageSubresourceRange.baseArrayLayer = 0;

	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.pNext = NULL;
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = image;
	imageBarrier.subresourceRange = imageSubresourceRange;

	VkPipelineStageFlags srcStage, dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Invalid image layout transition");
		return -1;
	}

	result = ImmediateSubmit([srcStage, dstStage, imageBarrier](RawCommandBuffer& cmdBuffer){
		vkCmdPipelineBarrier(cmdBuffer.commandBuffer,
						 srcStage,
						 dstStage,
						 0,
						 0,
						 NULL,
						 0,
						 NULL,
						 1,
						 &imageBarrier);
	}, QUEUE_TRANSFER);
	if (result != VK_SUCCESS) {
		return -1;
	}

	return 0;
}

int Renderer::CreateCommonBuffer(VkBuffer& buffer,
								  VkDeviceMemory& memory,
								  VkBufferUsageFlags usage,
								  size_t size)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;

	VkResult result = vkCreateBuffer(m_Device, &bufferCreateInfo, NULL, &buffer);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to create staging buffer.");
		return -1;
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(m_Device, buffer, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);

	VkMemoryPropertyFlags memoryPropertyFlags;
	if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
		memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	} else {
		memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = ChooseMemoryType(
		memoryRequirements.memoryTypeBits,
		memoryProperties,
		memoryPropertyFlags);

	result = vkAllocateMemory(m_Device, &memoryAllocateInfo, NULL, &memory);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to allocate memory for buffer.");
		vkDestroyBuffer(m_Device, buffer, NULL);
		return -1;
	}

	result = vkBindBufferMemory(m_Device, buffer, memory, 0);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to bind buffer memory.");
		vkDestroyBuffer(m_Device, buffer, NULL);
		vkFreeMemory(m_Device, memory, NULL);
		return -1;
	}

	return 0;
}

VertexBuffer Renderer::CreateVertexBuffer(size_t size) {
	VertexBuffer buffer;
	buffer.m_Device = m_Device;

	if (CreateCommonBuffer(buffer.m_Buffer,
		buffer.m_DeviceMemory,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		size))
	{
		return VertexBuffer();
	}

	buffer.m_Size = size;
	buffer.m_Initialized = true;

	return buffer;
}

IndexBuffer Renderer::CreateIndexBuffer(size_t size) {
	IndexBuffer buffer;
	buffer.m_Device = m_Device;

	if (CreateCommonBuffer(buffer.m_Buffer,
		buffer.m_DeviceMemory,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		size))
	{
		return IndexBuffer();
	}

	buffer.m_Size = size;
	buffer.m_Initialized = true;

	return buffer;
}

UniformBuffer Renderer::CreateUniformBuffer(size_t size) {
	UniformBuffer buffer;
	buffer.m_Device = m_Device;

	if (CreateCommonBuffer(buffer.m_Buffer,
		buffer.m_DeviceMemory,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		size))
	{
		return UniformBuffer();
	}

	buffer.m_Size = size;
	buffer.m_Initialized = true;

	return buffer;
}

ImageBuffer Renderer::CreateImageBuffer(size_t width, size_t height, ImageFormat format) {
	ImageBuffer buffer;
	buffer.m_Device = m_Device;
	buffer.m_CommandPool = m_TransferCmdPool;
	buffer.m_TransferQueue = m_TransferQueue;

	VkResult result = VK_SUCCESS;

	if (format == IMAGE_FORMAT_DEPTH) {
		result = Renderer::Get()->CreateImageObjects(&buffer.m_Image,
													 &buffer.m_DeviceMemory,
													 &buffer.m_ImageView,
													 m_DepthFormat,
													 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
													 VK_IMAGE_USAGE_SAMPLED_BIT,
													 (uint32_t*)&width,
													 (uint32_t*)&height,
													 &buffer.m_Size,
													 1, 1);
	} else {
		result = Renderer::Get()->CreateImageObjects(&buffer.m_Image,
													 &buffer.m_DeviceMemory,
													 &buffer.m_ImageView,
													 CeeFormatToVkFormat(format),
													 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
													 VK_IMAGE_USAGE_SAMPLED_BIT,
													 (uint32_t*)&width,
													 (uint32_t*)&height,
													 &buffer.m_Size,
													 1, 1);
	}
	CEE_VERIFY(result == VK_SUCCESS, "Failed to create image buffer.");

	buffer.m_Initialized = true;

	return buffer;
}

StagingBuffer Renderer::CreateStagingBuffer(size_t size) {
	StagingBuffer buffer;
	buffer.m_Device = m_Device;
	buffer.m_CommandPool = m_TransferCmdPool;
	buffer.m_TransferQueue = m_TransferQueue;

	if (CreateCommonBuffer(buffer.m_Buffer,
		buffer.m_DeviceMemory,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		size))
	{
		return StagingBuffer();
	}

	VkResult result = vkMapMemory(m_Device,
								  buffer.m_DeviceMemory,
								  0,
								  size,
								  0,
								  &buffer.m_MappedMemoryAddress);
	if (result != VK_SUCCESS) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to map memory for staging buffer.");
		vkDestroyBuffer(m_Device, buffer.m_Buffer, NULL);
		vkFreeMemory(m_Device, buffer.m_DeviceMemory, NULL);
		return StagingBuffer();
	}

	buffer.m_Size = size;
	buffer.m_Initialized = true;

	return buffer;
}
}

