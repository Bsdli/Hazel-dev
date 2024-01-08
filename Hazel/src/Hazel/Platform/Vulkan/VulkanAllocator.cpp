#include "hzpch.h"
#include "VulkanAllocator.h"

#include "VulkanContext.h"

namespace Hazel {

	namespace Utils {

		std::string BytesToString(uint64_t bytes)
		{
			static const float gb = 1024 * 1024 * 1024;
			static const float mb = 1024 * 1024;
			static const float kb = 1024;

			char buffer[16];

			if (bytes > gb)
				sprintf_s(buffer, "%.2f GB", bytes / gb);
			else if (bytes > mb)
				sprintf_s(buffer, "%.2f MB", bytes / mb);
			else if (bytes > kb)
				sprintf_s(buffer, "%.2f KB", bytes / kb);
			else
				sprintf_s(buffer, "%.2f bytes", bytes);

			return std::string(buffer);
		}

	}

	struct VulkanAllocatorData
	{
		VmaAllocator Allocator;
		uint64_t TotalAllocatedBytes = 0;
	};

	static VulkanAllocatorData* s_Data = nullptr;

	VulkanAllocator::VulkanAllocator(const std::string& tag)
		: m_Tag(tag)
	{
	}

	VulkanAllocator::~VulkanAllocator()
	{
	}

#if 0
	void VulkanAllocator::Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
	{
		HZ_CORE_ASSERT(m_Device);

		// TODO: Tracking
		HZ_CORE_TRACE("VulkanAllocator ({0}): allocating {1}", m_Tag, Utils::BytesToString(requirements.size));

		{
			static uint64_t totalAllocatedBytes = 0;
			totalAllocatedBytes += requirements.size;
			HZ_CORE_TRACE("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(totalAllocatedBytes));
		}

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}
#endif

	VmaAllocation VulkanAllocator::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_Data->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);

		// TODO: Tracking
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		HZ_CORE_TRACE("VulkanAllocator ({0}): allocating buffer; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			HZ_CORE_TRACE("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

		return allocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_Data->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);

		// TODO: Tracking
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		HZ_CORE_TRACE("VulkanAllocator ({0}): allocating image; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			HZ_CORE_TRACE("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}
		return allocation;
	}

	void VulkanAllocator::Free(VmaAllocation allocation)
	{
		vmaFreeMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		HZ_CORE_ASSERT(image);
		HZ_CORE_ASSERT(allocation);
		vmaDestroyImage(s_Data->Allocator, image, allocation);
	}

	void VulkanAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		HZ_CORE_ASSERT(buffer);
		HZ_CORE_ASSERT(allocation);
		vmaDestroyBuffer(s_Data->Allocator, buffer, allocation);
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocator::DumpStats()
	{
		VmaStats stats;
		vmaCalculateStats(s_Data->Allocator, &stats);

		uint64_t usedMemory = stats.memoryHeap[0].usedBytes;
		uint64_t freeMemory = stats.memoryHeap[0].unusedBytes;
	}

	GPUMemoryStats VulkanAllocator::GetStats()
	{
		VmaStats stats;
		vmaCalculateStats(s_Data->Allocator, &stats);

		uint64_t usedMemory = stats.total.usedBytes;
		uint64_t freeMemory = stats.total.unusedBytes;

		return { usedMemory, freeMemory };
	}

	void VulkanAllocator::Init(Ref<VulkanDevice> device)
	{
		s_Data = new VulkanAllocatorData();

		// Initialize VulkanMemoryAllocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		allocatorInfo.device = device->GetVulkanDevice();
		allocatorInfo.instance = VulkanContext::GetInstance();

		vmaCreateAllocator(&allocatorInfo, &s_Data->Allocator);
	}

	void VulkanAllocator::Shutdown()
	{
		vmaDestroyAllocator(s_Data->Allocator);

		delete s_Data;
		s_Data = nullptr;
	}

	VmaAllocator& VulkanAllocator::GetVMAAllocator()
	{
		return s_Data->Allocator;
	}

} 