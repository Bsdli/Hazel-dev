#include "hzpch.h"
#include "VulkanImage.h"

#include "VulkanContext.h"

namespace Hazel {

	VulkanImage2D::VulkanImage2D(ImageFormat format, uint32_t width, uint32_t height)
		: m_Format(format), m_Width(width), m_Height(height)
	{
	}

	VulkanImage2D::~VulkanImage2D()
	{
	}

	void VulkanImage2D::Invalidate()
	{

	}

	void VulkanImage2D::Release()
	{
		auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDestroyImageView(vulkanDevice, m_Info.ImageView, nullptr);
		vkDestroySampler(vulkanDevice, m_Info.Sampler, nullptr);

		VulkanAllocator allocator("VulkanImage2D");
		allocator.DestroyImage(m_Info.Image, m_Info.MemoryAlloc);

		HZ_CORE_WARN("VulkanImage2D::Release ImageView = {0}", (const void*)m_Info.ImageView);

	}

	void VulkanImage2D::UpdateDescriptor()
	{
		if (m_Format == ImageFormat::DEPTH24STENCIL8 || m_Format == ImageFormat::DEPTH32F)
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;

		HZ_CORE_WARN("VulkanImage2D::UpdateDescriptor to ImageView = {0}", (const void*)m_Info.ImageView);
	}

}