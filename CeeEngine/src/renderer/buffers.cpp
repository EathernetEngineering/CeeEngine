/* Implementation of buffer objects.
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

#include <CeeEngine/renderer/buffers.h>

#include <CeeEngine/messageBus.h>
#include <CeeEngine/renderer.h>

#include <cstring>

#include <stb/stb_image.h>

namespace cee {
static void FlushBufferMemory(size_t size, size_t offset, VkDeviceMemory deviceMemory) {
		VkMappedMemoryRange range = {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.pNext = NULL,
				.memory = deviceMemory,
				.offset = offset,
				.size = size
			};
			VkResult result = vkFlushMappedMemoryRanges(oldRenderer::Get()->GetDevice(), 1, &range);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to flush mapped memory.");
	}

	static bool CheckMemoryCopyBounds(size_t srcSize, size_t srcOffset, size_t dstSize, size_t dstOffset, size_t range) {
		if (((range + srcOffset) > srcSize) || ((range + dstOffset) > dstSize) || (range == 0))
			return true;

		return false;
	}

	VertexBuffer::VertexBuffer()
	: m_Layout(), m_Initialized(false), m_Size(0),
	  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_HostVisable(false),
	  m_HostMappedAddress(std::nullopt), m_PersistantlyMapped(false)
	{
	}

	VertexBuffer::VertexBuffer(const BufferLayout& layout, size_t size, bool persistantlyMapped)
	: m_Layout(layout), m_Initialized(false), m_Size(size),
	  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_HostVisable(false),
	  m_HostMappedAddress(std::nullopt), m_PersistantlyMapped(persistantlyMapped)
	{
		oldRenderer::Get()->CreateBufferObjects(&m_Buffer, &m_DeviceMemory, &m_Size,
											 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_HostVisable);
		m_Initialized = true;
	}

	VertexBuffer::VertexBuffer(VertexBuffer&& other)
	: VertexBuffer()
	{
		*this = std::move(other);
	}

	VertexBuffer::~VertexBuffer() {
		FreeResources();
	}

	VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) {
		if (this == &other) {
			return *this;
		}

		if (this->m_Initialized) {
			this->FreeResources();
		}

		this->m_Layout = std::move(other.m_Layout);
		this->m_Size = other.m_Size;
		this->m_Buffer = other.m_Buffer;
		this->m_DeviceMemory = other.m_DeviceMemory;
		this->m_HostVisable = other.m_HostVisable;
		this->m_HostMappedAddress = other.m_HostMappedAddress;
		this->m_PersistantlyMapped = other.m_PersistantlyMapped;

		this->m_Initialized = other.m_Initialized;
		other.m_Initialized = false;

		//other.m_Layout = BufferLayout();
		other.m_Size = 0;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_DeviceMemory = VK_NULL_HANDLE;
		other.m_HostVisable = false;
		other.m_HostMappedAddress.reset();
		other.m_PersistantlyMapped = false;
;
		return *this;
	}

	void VertexBuffer::SetData(size_t size, size_t offset, const void* data) {
		if (m_HostVisable) {
			if (!m_HostMappedAddress.has_value()) {
				MapMemory();
			}
			CheckMemoryCopyBounds(m_Size, 0, m_Size + offset,offset, size);
			memcpy(static_cast<uint8_t*>(m_HostMappedAddress.value_or(nullptr)) + offset, data, size);
			if (!m_PersistantlyMapped) {
				UnmapMemory();
			}
		} else {
			// TODO: Create a staging buffer, copy data, then destroy staging buffer.
			// OR: Create a staging buffer, then only destroy on destruction of this.
		}
	}

	void VertexBuffer::FlushMemory() {
		if (m_HostMappedAddress.has_value()) {
			FlushBufferMemory(m_Size, 0, m_DeviceMemory);
		} else  {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Calling flush on unmapped memory.");
		}
	}

	void VertexBuffer::MapMemory() {
		if (!m_HostMappedAddress.has_value()) {
			void* address;
			VkResult result = vkMapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, 0, m_Size, 0, &address);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to map memory.");
			m_HostMappedAddress = address;
		}
	}

	void VertexBuffer::UnmapMemory() {
		if (m_HostMappedAddress.has_value()) {
				FlushMemory();
				vkUnmapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory);
				m_HostMappedAddress.reset();
			}
	}

	void VertexBuffer::FreeResources() {
		if (m_Initialized) {
			vkDeviceWaitIdle(oldRenderer::Get()->GetDevice());
			if (m_HostMappedAddress.has_value()) {
				UnmapMemory();
			}
			vkDestroyBuffer(oldRenderer::Get()->GetDevice(), m_Buffer, NULL);
			vkFreeMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, NULL);
			m_Initialized = false;
		}
	}

	IndexBuffer::IndexBuffer()
	: m_Initialized(false), m_Size(0),
	  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_HostVisable(false),
	  m_HostMappedAddress(std::nullopt), m_PersistantlyMapped(false)
	{
	}

	IndexBuffer::IndexBuffer(size_t size, bool persistantlyMapped)
	: m_Initialized(false), m_Size(size),
	  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_HostVisable(false),
	  m_HostMappedAddress(std::nullopt), m_PersistantlyMapped(persistantlyMapped)
	{
		oldRenderer::Get()->CreateBufferObjects(&m_Buffer, &m_DeviceMemory, &m_Size,
											 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_HostVisable);
		m_Initialized = true;
	}

	IndexBuffer::IndexBuffer(IndexBuffer&& other)
	: IndexBuffer()
	{
		*this = std::move(other);
	}

	IndexBuffer::~IndexBuffer() {
		FreeResources();
	}

	IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) {
		if (this->m_Initialized) {
			this->FreeResources();
		}

		this->m_Size = other.m_Size;
		this->m_Buffer = other.m_Buffer;
		this->m_DeviceMemory = other.m_DeviceMemory;
		this->m_HostMappedAddress = other.m_HostMappedAddress;
		this->m_HostVisable = other.m_HostVisable;
		this->m_PersistantlyMapped = other.m_PersistantlyMapped;

		this->m_Initialized = other.m_Initialized;
		other.m_Initialized = false;

		other.m_Buffer = VK_NULL_HANDLE;
		other.m_DeviceMemory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_HostMappedAddress.reset();
		other.m_HostVisable = false;
		other.m_PersistantlyMapped = false;

		return *this;
	}

	void IndexBuffer::SetData(size_t size, size_t offset, const void* data) {
		if (m_HostVisable) {
			if (!m_HostMappedAddress.has_value()) {
				MapMemory();
			}
			memcpy(static_cast<uint8_t*>(m_HostMappedAddress.value_or(nullptr)) + offset, data, size);
			if (!m_PersistantlyMapped) {
				UnmapMemory();
			}
		} else {
			// TODO: Create a staging buffer, copy data, then destroy staging buffer.
			// OR: Create a staging buffer, then only destroy on destruction of this.
		}
	}

	void IndexBuffer::FlushMemory() {
		if (m_HostMappedAddress.has_value()) {
			FlushBufferMemory(m_Size, 0, m_DeviceMemory);
		} else  {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Calling flush on unmapped memory.");
		}
	}

	void IndexBuffer::MapMemory() {
		if (!m_HostMappedAddress.has_value()) {
			void* address;
			VkResult result = vkMapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, 0, m_Size, 0, &address);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to map memory.");
			m_HostMappedAddress = address;
		}
	}

	void IndexBuffer::UnmapMemory() {
		if (m_HostMappedAddress.has_value()) {
				FlushMemory();
				vkUnmapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory);
				m_HostMappedAddress.reset();
			}
	}

	void IndexBuffer::FreeResources() {
		if (m_Initialized) {
			vkDeviceWaitIdle(oldRenderer::Get()->GetDevice());
			if (m_HostMappedAddress.has_value()) {
				UnmapMemory();
			}
			vkDestroyBuffer(oldRenderer::Get()->GetDevice(), m_Buffer, NULL);
			vkFreeMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, NULL);
			m_Initialized = false;
		}
	}

	UniformBuffer::UniformBuffer()
	: m_Initialized(false), m_Size(0),
	  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_HostVisable(false),
	  m_HostMappedAddress(std::nullopt), m_PersistantlyMapped(false)
	{
	}

	UniformBuffer::UniformBuffer(const BufferLayout& layout, size_t size, bool persistantlyMapped)
	: m_Initialized(false), m_Size(size),
	  m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE), m_HostVisable(false),
	  m_HostMappedAddress(std::nullopt), m_PersistantlyMapped(persistantlyMapped),
	  m_Layout(layout)
	{
		oldRenderer::Get()->CreateBufferObjects(&m_Buffer, &m_DeviceMemory, &m_Size,
											 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_HostVisable);
		m_Initialized = true;
	}

	UniformBuffer::UniformBuffer(UniformBuffer&& other)
	: UniformBuffer()
	{
		*this = std::move(other);
	}

	UniformBuffer::~UniformBuffer() {
		FreeResources();
	}

	UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) {
		if (this->m_Initialized) {
			this->FreeResources();
		}

		this->m_Size = other.m_Size;
		this->m_Buffer = other.m_Buffer;
		this->m_DeviceMemory = other.m_DeviceMemory;
		this->m_HostVisable = other.m_HostVisable;
		this->m_HostMappedAddress = other.m_HostMappedAddress;
		this->m_PersistantlyMapped = other.m_PersistantlyMapped;
		this->m_Layout = other.m_Layout;

		this->m_Initialized = other.m_Initialized;
		other.m_Initialized = false;

		other.m_Buffer = VK_NULL_HANDLE;
		other.m_DeviceMemory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_HostVisable = false;
		other.m_HostMappedAddress.reset();
		other.m_PersistantlyMapped = false;
		other.m_Layout = BufferLayout();

		return *this;
	}

	void UniformBuffer::SetData(size_t size, size_t offset, const void* data) {
		if (m_HostVisable) {
			if (!m_HostMappedAddress.has_value()) {
				MapMemory();
			}
			memcpy(static_cast<uint8_t*>(m_HostMappedAddress.value_or(nullptr)) + offset, data, size);
			if (!m_PersistantlyMapped) {
				UnmapMemory();
			}
		} else {
			// TODO: Create a staging buffer, copy data, then destroy staging buffer.
			// OR: Create a staging buffer, then only destroy on destruction of this.
		}
	}

	void UniformBuffer::FlushMemory() {
		if (m_HostMappedAddress.has_value()) {
			FlushBufferMemory(m_Size, 0, m_DeviceMemory);
		} else  {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Calling flush on unmapped memory.");
		}
	}

	void UniformBuffer::MapMemory() {
		if (!m_HostMappedAddress.has_value()) {
			void* address;
			VkResult result = vkMapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, 0, m_Size, 0, &address);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to map memory.");
			m_HostMappedAddress = address;
		}
	}

	void UniformBuffer::UnmapMemory() {
		if (m_HostMappedAddress.has_value()) {
				FlushMemory();
				vkUnmapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory);
				m_HostMappedAddress.reset();
			}
	}

	void UniformBuffer::FreeResources() {
		if (m_Initialized) {
			vkDeviceWaitIdle(oldRenderer::Get()->GetDevice());
			if (m_HostMappedAddress.has_value()) {
				UnmapMemory();
			}
			vkDestroyBuffer(oldRenderer::Get()->GetDevice(), m_Buffer, NULL);
			vkFreeMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, NULL);
			m_Layout.clear();
			m_Initialized = false;
		}
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
		FreeResources();
	}

	ImageBuffer& ImageBuffer::operator=(ImageBuffer&& other) {
		if (this->m_Initialized) {
			this->FreeResources();
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

	void ImageBuffer::FreeResources() {
		if (m_Initialized) {
			//vkDeviceWaitIdle(m_Device);
			vkDestroyImageView(m_Device, m_ImageView, NULL);
			vkDestroyImage(m_Device, m_Image, NULL);
			vkFreeMemory(m_Device, m_DeviceMemory, NULL);
			m_Initialized = false;
		}
	}

	void ImageBuffer::Clear(glm::vec4 clearColor) {
		oldRenderer::Get()->ImmediateSubmit([this, clearColor](VkCommandBuffer cmdBuffer){
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
			vkCmdClearColorImage(cmdBuffer, m_Image, m_Layout, &clearValue, 1, &range);
		}, oldRenderer::QUEUE_TRANSFER);
	}

	void ImageBuffer::TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
		if (newLayout == m_Layout) {
			return;
		}

		VkImageMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.pNext = NULL;
		memoryBarrier.oldLayout = m_Layout;
		memoryBarrier.newLayout = newLayout;
		// TODO check queue families.
		memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image = m_Image;
		memoryBarrier.subresourceRange = {
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
			memoryBarrier.srcAccessMask = VK_ACCESS_NONE;
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (m_Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if (m_Layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else
		{
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Unsupported image layout transition");
			return;
		}


		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0,
							 0, NULL,
							 0, NULL,
							 1, &memoryBarrier);

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
		oldRenderer::Get()->CreateImageObjects(&m_Image, &m_DeviceMemory, &m_ImageView,
											VK_FORMAT_R8G8B8A8_SRGB,
											VK_IMAGE_USAGE_TRANSFER_DST_BIT |
											VK_IMAGE_USAGE_SAMPLED_BIT,
											&width, &height, &m_Size,
											1, 6);

		m_Initialized = true;
	}

	CubeMapBuffer::CubeMapBuffer(std::vector<std::string> filenames)
	: m_Initialized(false), m_Size(0u), m_Extent({ 0u, 0u, 1u }), m_Image(VK_NULL_HANDLE),
	  m_DeviceMemory(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE), m_Layout(VK_IMAGE_LAYOUT_UNDEFINED)
	{
		void* imageData = stbi_load(filenames[0].c_str(),
									reinterpret_cast<int*>(&m_Extent.width),
									reinterpret_cast<int*>(&m_Extent.height),
									NULL, 4);
		CEE_VERIFY(imageData != NULL, "Failed to read image from file");

		oldRenderer::Get()->CreateImageObjects(&m_Image, &m_DeviceMemory, &m_ImageView,
											VK_FORMAT_R8G8B8A8_SRGB,
											VK_IMAGE_USAGE_TRANSFER_DST_BIT |
											VK_IMAGE_USAGE_SAMPLED_BIT,
											&m_Extent.width, &m_Extent.height, &m_Size,
											1, 6);
		m_Initialized = true;

		size_t singleImageSize = m_Extent.width * m_Extent.height * 4;
		StagingBuffer sb(m_Size);
		sb.SetData(singleImageSize, 0, imageData);
		free(imageData);
		for (uint32_t i = 1; i < 6; i++) {
			int thisImageWidth, thisImageHeight;
			imageData = stbi_load(filenames[i].c_str(),
								  &thisImageWidth,
								  &thisImageHeight,
								  NULL, 4);
			CEE_VERIFY(imageData != NULL, "Failed to read image from file");
			CEE_ASSERT(thisImageWidth == static_cast<int>(m_Extent.width) &&
					   thisImageHeight == static_cast<int>(m_Extent.height),
					   "Image sizes do not match");
			sb.SetData(singleImageSize, i * singleImageSize, imageData);
			free(imageData);
		}
		sb.TransferData(*this, 0);
	}

	CubeMapBuffer::CubeMapBuffer(CubeMapBuffer&& other) {
		*this = std::move(other);
	}

	CubeMapBuffer::~CubeMapBuffer() {
		FreeResources();
	}

	CubeMapBuffer& CubeMapBuffer::operator=(CubeMapBuffer&& other) {
		if (this->m_Initialized) {
			this->FreeResources();
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

	void CubeMapBuffer::FreeResources() {
		if (m_Initialized) {
			vkDestroyImageView(oldRenderer::Get()->GetDevice(), m_ImageView, NULL);
			vkFreeMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, NULL);
			vkDestroyImage(oldRenderer::Get()->GetDevice(), m_Image, NULL);
			m_Initialized = false;
		}
	}

	void CubeMapBuffer::Clear(glm::vec4 clearColor) {
		oldRenderer::Get()->ImmediateSubmit([this, &clearColor](VkCommandBuffer cmdBuffer){
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
			vkCmdClearColorImage(cmdBuffer, m_Image, m_Layout, &vkClearColor, 1, &range);
			this->TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}, oldRenderer::QUEUE_TRANSFER);
	}

	void CubeMapBuffer::TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout) {
		if (newLayout == m_Layout) {
			return;
		}

		VkImageMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.pNext = NULL;
		memoryBarrier.oldLayout = m_Layout;
		memoryBarrier.newLayout = newLayout;
		// TODO check queue families.
		memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image = m_Image;
		memoryBarrier.subresourceRange = {
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
			memoryBarrier.srcAccessMask = VK_ACCESS_NONE;
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (m_Layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if (m_Layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else
		{
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Unsupported image layout transition");
			return;
		}


		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0,
							 0, NULL,
							 0, NULL,
							 1, &memoryBarrier);

		m_Layout = newLayout;
	}

	StagingBuffer::StagingBuffer()
	: m_Initialized(false), m_Size(0), m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE),
	  m_MappedMemoryAddress(std::nullopt), m_PersistantlyMapped(false)
	{
	}

	StagingBuffer::StagingBuffer(size_t size, bool persistantlyMapped)
	: m_Initialized(false), m_Size(size), m_Buffer(VK_NULL_HANDLE), m_DeviceMemory(VK_NULL_HANDLE),
	  m_MappedMemoryAddress(std::nullopt), m_PersistantlyMapped(persistantlyMapped)
	{
		oldRenderer::Get()->CreateBufferObjects(&m_Buffer, &m_DeviceMemory, &m_Size,
											 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
											 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr);

		m_Initialized = true;
	}

	StagingBuffer::StagingBuffer(StagingBuffer&& other)
	: StagingBuffer()
	{
		*this = std::move(other);
	}

	StagingBuffer::~StagingBuffer() {
		FreeResources();
	}

	StagingBuffer& StagingBuffer::operator=(StagingBuffer&& other) {
		if (this->m_Initialized) {
			FreeResources();
		}

		this->m_Size = other.m_Size;
		this->m_Buffer = other.m_Buffer;
		this->m_DeviceMemory = other.m_DeviceMemory;
		this->m_MappedMemoryAddress = other.m_MappedMemoryAddress;
		this->m_PersistantlyMapped = other.m_PersistantlyMapped;

		this->m_Initialized = other.m_Initialized;
		other.m_Initialized = false;

		other.m_Buffer = VK_NULL_HANDLE;
		other.m_DeviceMemory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_MappedMemoryAddress.reset();
		other.m_PersistantlyMapped = false;

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

		if (!m_MappedMemoryAddress.has_value()) {
			MapMemory();
		}

		// Copy data.
		memcpy(static_cast<uint8_t*>(m_MappedMemoryAddress.value_or(nullptr)) + offset, data, size);

		if (!m_PersistantlyMapped) {
			UnmapMemory();
		}
		return 0;
	}

	void StagingBuffer::FreeResources() {
		if (m_Initialized) {
			vkDeviceWaitIdle(oldRenderer::Get()->GetDevice());
			if (m_MappedMemoryAddress.has_value()) {
				UnmapMemory();
			}
			vkDestroyBuffer(oldRenderer::Get()->GetDevice(), m_Buffer, NULL);
			vkFreeMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, NULL);
		}
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
		oldRenderer::Get()->QueueSubmit([src, dst, copyRegion](VkCommandBuffer commandBuffer){
			vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
		}, oldRenderer::QUEUE_TRANSFER);

		return 0;
	}

	int StagingBuffer::TransferDataInternalImmediate(VkBuffer src,
													 VkBuffer dst,
													 VkBufferCopy copyRegion)
	{
		oldRenderer::Get()->ImmediateSubmit([src, dst, copyRegion](VkCommandBuffer cb) {
			vkCmdCopyBuffer(cb, src, dst, 1, &copyRegion);
		}, oldRenderer::QUEUE_TRANSFER);

		return 0;
	}

	void StagingBuffer::FlushMemory() {
		if (m_MappedMemoryAddress.has_value()) {
			FlushBufferMemory(m_Size, 0, m_DeviceMemory);
		} else  {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Calling flush on unmapped memory.");
		}
	}

	void StagingBuffer::MapMemory() {
		if (!m_MappedMemoryAddress.has_value()) {
			void* address;
			VkResult result = vkMapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory, 0, m_Size, 0, &address);
			CEE_VERIFY(result == VK_SUCCESS, "Failed to map memory for staging buffer.");
			m_MappedMemoryAddress = address;
		} else {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
											 "Calling MapMemory() on an already mapped buffer.");
		}
	}

	void StagingBuffer::UnmapMemory() {
		if (m_MappedMemoryAddress.has_value()) {
			FlushMemory();
			vkUnmapMemory(oldRenderer::Get()->GetDevice(), m_DeviceMemory);
			m_MappedMemoryAddress.reset();
		}
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
		result = oldRenderer::Get()->QueueSubmit([src, dst, width, height, &imageBuffer](VkCommandBuffer commandBuffer) {
			imageBuffer.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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

			vkCmdCopyBufferToImage(commandBuffer,
								src,
								dst,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								1,
								&imageCopy);

			imageBuffer.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}, oldRenderer::QUEUE_TRANSFER);


		return result == VK_SUCCESS ? 0 : -1;
	}
	int StagingBuffer::TransferData(CubeMapBuffer& imageBuffer, size_t srcOffset) {
		if (!this->m_Initialized || !imageBuffer.m_Initialized) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Trying to copy data using an uninitialized buffer.");
			return -1;
		}
		VkResult result = oldRenderer::Get()->ImmediateSubmit([this, &imageBuffer, srcOffset](VkCommandBuffer cmdBuffer){
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
			vkCmdCopyBufferToImage(cmdBuffer, m_Buffer,
								   imageBuffer.m_Image, imageBuffer.m_Layout,
								   6, imageCopyRanges.data());

			imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}, oldRenderer::QUEUE_TRANSFER);


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
		result = oldRenderer::Get()->ImmediateSubmit([src, dst, width, height, &imageBuffer](VkCommandBuffer commandBuffer) {
			imageBuffer.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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

			vkCmdCopyBufferToImage(commandBuffer,
								src,
								dst,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								1,
								&imageCopy);
			imageBuffer.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}, oldRenderer::QUEUE_TRANSFER);


		return result == VK_SUCCESS ? 0 : -1;
	}

	int StagingBuffer::TransferDataImmediate(CubeMapBuffer& imageBuffer, size_t srcOffset) {
		if (!this->m_Initialized || !imageBuffer.m_Initialized) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Trying to copy data using an uninitialized buffer.");
			return -1;
		}
		VkResult result = oldRenderer::Get()->ImmediateSubmit([this, &imageBuffer, srcOffset](VkCommandBuffer cmdBuffer){
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
			vkCmdCopyBufferToImage(cmdBuffer, m_Buffer,
								   imageBuffer.m_Image, imageBuffer.m_Layout,
								   6, imageCopyRanges.data());

			imageBuffer.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}, oldRenderer::QUEUE_TRANSFER);
		return result == VK_SUCCESS ? 0 : -1;
	}
}
