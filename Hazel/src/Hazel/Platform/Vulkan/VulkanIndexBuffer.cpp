#include "hzpch.h"
#include "VulkanIndexBuffer.h"

#include "VulkanContext.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t size)
		: m_Size(size)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(void* data, uint32_t size)
		: m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		Ref<VulkanIndexBuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			// Index buffer
			VkBufferCreateInfo indexbufferCreateInfo = {};
			indexbufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			indexbufferCreateInfo.size = instance->m_Size;
			indexbufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;


			VulkanAllocator allocator("IndexBuffer");
			auto bufferAlloc = allocator.AllocateBuffer(indexbufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, instance->m_VulkanBuffer);

			void* dstBuffer = allocator.MapMemory<void>(bufferAlloc);
			memcpy(dstBuffer, instance->m_LocalData.Data, instance->m_Size);
			allocator.UnmapMemory(bufferAlloc);
		});
	}

	void VulkanIndexBuffer::SetData(void* buffer, uint32_t size, uint32_t offset)
	{
	}

	void VulkanIndexBuffer::Bind() const
	{
	}

	RendererID VulkanIndexBuffer::GetRendererID() const
	{
		return 0;
	}

}