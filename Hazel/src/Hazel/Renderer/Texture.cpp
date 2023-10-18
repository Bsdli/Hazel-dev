#include "hzpch.h"
#include "Texture.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Platform/OpenGL/OpenGLTexture.h"

namespace Hazel {

	Ref<Texture2D> Texture2D::Create(TextureFormat format, unsigned int width, unsigned int height, TextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return CreateRef<OpenGLTexture2D>(format, width, height, wrap);
		}
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::string& path, bool srgb)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return CreateRef<OpenGLTexture2D>(path, srgb);
		}
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(TextureFormat format, uint32_t width, uint32_t height)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return CreateRef<OpenGLTextureCube>(format, width, height);
		}
		return nullptr;
	}

	Ref<TextureCube> TextureCube::Create(const std::string& path)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return CreateRef<OpenGLTextureCube>(path);
		}
		return nullptr;
	}

	uint32_t Texture::GetBPP(TextureFormat format)
	{
		switch (format)
		{
			case TextureFormat::RGB:    return 3;
			case TextureFormat::RGBA:   return 4;
		}
		return 0;
	}

	uint32_t Texture::CalculateMipMapCount(uint32_t width, uint32_t height)
	{
		uint32_t levels = 1;
		while ((width | height) >> levels)
			levels++;

		return levels;
	}

}