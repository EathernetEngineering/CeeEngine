/* Decleration of buffer objects for Vulkan.
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

#ifndef CEE_RENDERER_BUFFERS_H
#define CEE_RENDERER_BUFFERS_H

#include <cstdint>
#include <cstddef>
#include <optional>
#include <vector>
#include <string>

#include <CeeEngine/assert.h>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace cee {
	class oldRenderer;
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

	struct BufferAttribute {
		ShaderDataType type;
		uint32_t size;
		uint32_t offset;

		bool normalized;

		BufferAttribute(ShaderDataType type, bool normalized = false)
		: type(type), size(GetShaderDataTypeSize(type)), normalized(normalized)
		{ }
	};

	struct BufferLayout {
		BufferLayout() {}
		BufferLayout(std::initializer_list<BufferAttribute> attibutes)
		: m_Attributes(attibutes)
		{
			CalculateOffsetAndStride();
		}

		void clear() { m_Attributes.clear(); }
		void push_back(const BufferAttribute& attribute) { m_Attributes.push_back(attribute); }

		uint32_t GetStride() const { return m_Stride; }
		const std::vector<BufferAttribute>& GetElements() const { return m_Attributes; }

		std::vector<BufferAttribute>::iterator begin() { return m_Attributes.begin(); }
		std::vector<BufferAttribute>::iterator end() { return m_Attributes.end(); }
		std::vector<BufferAttribute>::const_iterator cbegin() const { return m_Attributes.cbegin(); }
		std::vector<BufferAttribute>::const_iterator cend() const { return m_Attributes.cend(); }

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
		std::vector<BufferAttribute> m_Attributes;
		uint32_t m_Stride;
	};

	class VertexBuffer {
	public:
		VertexBuffer();
		VertexBuffer(const BufferLayout& layout, size_t size, bool persistantlyMapped = false);
		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&& other);
		~VertexBuffer();

		VertexBuffer& operator=(const VertexBuffer&) = delete;
		VertexBuffer& operator=(VertexBuffer&& other);

		void SetData(size_t size, size_t offset, const void* data);

		const BufferLayout& GetLayout() const { return m_Layout; }

	private:
		void FlushMemory();
		void MapMemory();
		void UnmapMemory();

		// To use this buffer for buffer copy/usage, this function should be
		// called instead of accessing member directly as it flushes data to the GPU.
		VkBuffer& GetBuffer() { if (m_PersistantlyMapped) FlushMemory(); return m_Buffer; }

		void FreeResources();

	private:
		BufferLayout m_Layout;
		bool m_Initialized;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		bool m_HostVisable;
		std::optional<void*> m_HostMappedAddress;

		bool m_PersistantlyMapped;

		friend oldRenderer;
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

		VkBuffer GetBuffer() { if (m_PersistantlyMapped) FlushMemory(); return m_Buffer; }

		void FreeResources();

	private:
		bool m_Initialized;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		bool m_HostVisable;
		std::optional<void*> m_HostMappedAddress;

		bool m_PersistantlyMapped;

		friend oldRenderer;
		friend StagingBuffer;
	};

	class UniformBuffer {
	public:
		UniformBuffer();
		UniformBuffer(const BufferLayout& layout, size_t size, bool persistantlyMapped = false);
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

		VkBuffer GetBuffer() { if (m_PersistantlyMapped) FlushMemory(); return m_Buffer; }
		const BufferLayout& GetLayout() const { return m_Layout; }

		void FreeResources();

	private:
		bool m_Initialized;

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		bool m_HostVisable;
		std::optional<void*> m_HostMappedAddress;

		bool m_PersistantlyMapped;

		BufferLayout m_Layout;

		friend oldRenderer;
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
		void FreeResources();
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

		friend oldRenderer;
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
		void FreeResources();
		void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

	private:
		bool m_Initialized;

		size_t m_Size;
		VkExtent3D m_Extent;
		VkImage m_Image;
		VkDeviceMemory m_DeviceMemory;
		VkImageView m_ImageView;

		VkImageLayout m_Layout;

		friend oldRenderer;
		friend StagingBuffer;
	};

	class StagingBuffer {
	public:
		StagingBuffer();
		StagingBuffer(size_t size, bool persistantlyMapped  =false);
		StagingBuffer(const StagingBuffer&) = delete;
		StagingBuffer(StagingBuffer&& other);
		~StagingBuffer();

		StagingBuffer& operator=(const StagingBuffer&) = delete;
		StagingBuffer& operator=(StagingBuffer&& other);

		int SetData(size_t size, size_t offset, const void* data);

	private:
		VkBuffer GetBuffer() { if (m_PersistantlyMapped) FlushMemory(); return m_Buffer; }

		void FreeResources();

		int BoundsCheck(size_t size, size_t srcSize, size_t dstSize, size_t srcOffset, size_t dstOddset);

		int TransferDataInternal(VkBuffer src,
								 VkBuffer dst,
								 VkBufferCopy copyRegion);
		int TransferDataInternalImmediate(VkBuffer src,
										  VkBuffer dst,
										  VkBufferCopy copyRegion);

		void FlushMemory();
		void MapMemory();
		void UnmapMemory();

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

		size_t m_Size;
		VkBuffer m_Buffer;
		VkDeviceMemory m_DeviceMemory;

		std::optional<void*> m_MappedMemoryAddress;
		bool m_PersistantlyMapped;

		friend oldRenderer;
	};
}

#endif
