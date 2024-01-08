#pragma once

#include "Hazel/Renderer/Texture.h"

#include "Vulkan.h"

#include "VulkanImage.h"

namespace Hazel {

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const std::string& path, TextureProperties properties);
		VulkanTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		virtual ~VulkanTexture2D();

		void Invalidate();

		virtual ImageFormat GetFormat() const override { return m_Format; }
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override;

		virtual Ref<Image2D> GetImage() const override { return m_Image; }
		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_Image.As<VulkanImage2D>()->GetDescriptor(); }

		void Lock() override;
		void Unlock() override;

		Buffer GetWriteableBuffer() override;
		bool Loaded() const override;
		const std::string& GetPath() const override;
		uint32_t GetMipLevelCount() const override;

		void GenerateMips();

		virtual uint64_t GetHash() const { return (uint64_t)m_Image; }
	private:
		std::string m_Path;
		uint32_t m_Width;
		uint32_t m_Height;
		TextureProperties m_Properties;

		Buffer m_ImageData;

		Ref<Image2D> m_Image;

		ImageFormat m_Format = ImageFormat::None;
	};

	class VulkanTextureCube : public TextureCube
	{
	public:
		VulkanTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		VulkanTextureCube(const std::string& path, TextureProperties properties);
		virtual ~VulkanTextureCube();

		virtual const std::string& GetPath() const override { return ""; }

		virtual void Bind(uint32_t slot = 0) const override {}

		virtual ImageFormat GetFormat() const { return m_Format; }

		virtual uint32_t GetWidth() const override{ return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetMipLevelCount() const override;

		virtual uint64_t GetHash() const { return (uint64_t)m_Image; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }
		VkImageView CreateImageViewSingleMip(uint32_t mip);

		void GenerateMips(bool readonly = false);
	private:
		void Invalidate();
	private:
		ImageFormat m_Format = ImageFormat::None;
		uint32_t m_Width = 0, m_Height = 0;
		TextureProperties m_Properties;

		bool m_MipsGenerated = false;

		Buffer m_LocalStorage;
		VmaAllocation m_MemoryAlloc;
		VkImage m_Image;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
	};

}
