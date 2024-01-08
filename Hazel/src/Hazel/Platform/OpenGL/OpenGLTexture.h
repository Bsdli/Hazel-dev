#pragma once

#include "Hazel/Renderer/RendererTypes.h"
#include "Hazel/Renderer/Texture.h"

namespace Hazel {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		OpenGLTexture2D(const std::string& path, TextureProperties properties);
		virtual ~OpenGLTexture2D();

		virtual void Bind(uint32_t slot = 0) const;

		virtual Ref<Image2D> GetImage() const override { return m_Image; }
		
		virtual ImageFormat GetFormat() const override { return m_Image->GetFormat(); }
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;

		virtual void Lock() override;
		virtual void Unlock() override;

		virtual Buffer GetWriteableBuffer() override;

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual bool Loaded() const override { return m_Loaded; }

		virtual uint64_t GetHash() const { return m_Image->GetHash(); }
	private:
		Ref<Image2D> m_Image;
		TextureProperties m_Properties;
		uint32_t m_Width, m_Height;

		bool m_IsHDR = false;

		bool m_Locked = false;
		bool m_Loaded = false;

		std::string m_FilePath;
	};

	class OpenGLTextureCube : public TextureCube
	{
	public:
		OpenGLTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties);
		OpenGLTextureCube(const std::string& path, TextureProperties properties);
		virtual ~OpenGLTextureCube();

		virtual void Bind(uint32_t slot = 0) const;

		virtual ImageFormat GetFormat() const { return m_Format; }
		virtual uint32_t GetWidth() const { return m_Width; }
		virtual uint32_t GetHeight() const { return m_Height; }
		// This function currently returns the expected number of mips based on image size,
		// not present mips in data
		virtual uint32_t GetMipLevelCount() const override;

		virtual const std::string& GetPath() const override { return m_FilePath; }

		RendererID GetRendererID() const { return m_RendererID; }

		virtual uint64_t GetHash() const { return (uint64_t)m_RendererID; }
	private:
		RendererID m_RendererID;
		ImageFormat m_Format;
		uint32_t m_Width, m_Height;

		TextureProperties m_Properties;

		Buffer m_LocalStorage;

		std::string m_FilePath;
	};
}