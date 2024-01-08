#pragma once

#include <string>
#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

namespace Hazel {

	namespace Utils {

		std::string BytesToString(uint64_t bytes);

	}

	struct GPUMemoryStats
	{
		uint64_t Used = 0;
		uint64_t Free = 0;
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator() = default;
		VulkanAllocator(const std::string& tag);
		~VulkanAllocator();

		//void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer);
		VmaAllocation AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage);
		void Free(VmaAllocation allocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
			vmaMapMemory(VulkanAllocator::GetVMAAllocator(), allocation, (void**)&mappedMemory);
			return mappedMemory;
		}

		void UnmapMemory(VmaAllocation allocation);

		static void DumpStats();
		static GPUMemoryStats GetStats();

		static void Init(Ref<VulkanDevice> device);
		static void Shutdown();

		static VmaAllocator& GetVMAAllocator();
	private:
		std::string m_Tag;
	};


}