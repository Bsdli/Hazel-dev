#include "hzpch.h"
#include "VulkanVertexBuffer.h"

#include "VulkanContext.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
		: m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		Ref<VulkanVertexBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			// TODO: Use staging
			auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			// Vertex buffer
			VkBufferCreateInfo vertexBufferCreateInfo = {};
			vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferCreateInfo.size = instance->m_Size;
			vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			
			VulkanAllocator allocator("VertexBuffer");
			auto bufferAlloc = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, instance->m_VulkanBuffer);

			void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
			memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
			allocator.UnmapMemory(bufferAlloc);
		});
	}


}