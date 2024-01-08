#include "hzpch.h"
#include "OpenGLTexture.h"

#include "Hazel/Renderer/Renderer.h"

#include <glad/glad.h>
#include "stb_image.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLImage.h"

namespace Hazel {

	//////////////////////////////////////////////////////////////////////////////////
	// Texture2D
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTexture2D::OpenGLTexture2D(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
		: m_Width(width), m_Height(height), m_Properties(properties)
	{
		m_Image = Image2D::Create(format, width, height, data);
		Renderer::Submit([=]()
		{
			m_Image->Invalidate();
		});
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, TextureProperties properties)
		: m_FilePath(path), m_Properties(properties)
	{
		int width, height, channels;
		if (stbi_is_hdr(path.c_str()))
		{
			HZ_CORE_INFO("Loading HDR texture {0}, srgb={1}", path, properties.SRGB);

			float* imageData = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
			HZ_CORE_ASSERT(imageData);
			Buffer buffer(imageData, Utils::GetImageMemorySize(ImageFormat::RGBA32F, width, height));
			m_Image = Image2D::Create(ImageFormat::RGBA32F, width, height, buffer);
		}
		else
		{
			HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, properties.SRGB);

			stbi_uc* imageData = stbi_load(path.c_str(), &width, &height, &channels, properties.SRGB ? STBI_rgb : STBI_rgb_alpha);
			HZ_CORE_ASSERT(imageData);
			//ImageFormat format = channels == 4 ? ImageFormat::RGBA : ImageFormat::RGB;
			ImageFormat format = properties.SRGB ? ImageFormat::RGB : ImageFormat::RGBA;
			Buffer buffer(imageData, Utils::GetImageMemorySize(format, width, height));
			m_Image = Image2D::Create(format, width, height, buffer);
		}

		m_Image.As<OpenGLImage2D>()->CreateSampler(m_Properties);

		m_Width = width;
		m_Height = height;
		m_Loaded = true;

		Ref<Image2D>& image = m_Image;
		Renderer::Submit([image]() mutable
		{
			image->Invalidate();

			Buffer& buffer = image->GetBuffer();
			stbi_image_free(buffer.Data);
			buffer = Buffer();
		});

	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		Ref<Image2D> image = m_Image;
		Renderer::Submit([image]() mutable {
			image->Release();
		});
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		Ref<OpenGLImage2D> image = m_Image.As<OpenGLImage2D>();
		Renderer::Submit([slot, image]() {
			glBindTextureUnit(slot, image->GetRendererID());
		});
	}

	void OpenGLTexture2D::Lock()
	{
		m_Locked = true;
	}

	void OpenGLTexture2D::Unlock()
	{
		m_Locked = false;
		Ref<OpenGLTexture2D> instance = this;
		Ref<OpenGLImage2D> image = m_Image.As<OpenGLImage2D>();
		Renderer::Submit([instance, image]() mutable {
			glTextureSubImage2D(image->GetRendererID(), 0, 0, 0, instance->m_Width, instance->m_Height, Utils::OpenGLImageFormat(image->GetFormat()), GL_UNSIGNED_BYTE, instance->m_Image->GetBuffer().Data);
		});
	}

	Buffer OpenGLTexture2D::GetWriteableBuffer()
	{
		HZ_CORE_ASSERT(m_Locked, "Texture must be locked!");
		return m_Image->GetBuffer();
	}

	uint32_t OpenGLTexture2D::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLTextureCube::OpenGLTextureCube(ImageFormat format, uint32_t width, uint32_t height, const void* data, TextureProperties properties)
		: m_Width(width), m_Height(height), m_Format(format), m_Properties(properties)
	{
		if (data)
		{
			uint32_t size = width * height * 4 * 6; // six layers
			m_LocalStorage = Buffer::Copy(data, size);
		}

		uint32_t levels = Utils::CalculateMipCount(width, height);
		Ref<OpenGLTextureCube> instance = this;
		Renderer::Submit([instance, levels]() mutable
		{
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &instance->m_RendererID);
			glTextureStorage2D(instance->m_RendererID, levels, Utils::OpenGLImageInternalFormat(instance->m_Format), instance->m_Width, instance->m_Height);
			if (instance->m_LocalStorage.Data)
				glTextureSubImage3D(instance->m_RendererID, 0, 0, 0, 0, instance->m_Width, instance->m_Height, 6, Utils::OpenGLImageFormat(instance->m_Format), Utils::OpenGLFormatDataType(instance->m_Format), instance->m_LocalStorage.Data);

			glTextureParameteri(instance->m_RendererID, GL_TEXTURE_MIN_FILTER, Utils::OpenGLSamplerFilter(instance->m_Properties.SamplerFilter, instance->m_Properties.GenerateMips));
			glTextureParameteri(instance->m_RendererID, GL_TEXTURE_MAG_FILTER, Utils::OpenGLSamplerFilter(instance->m_Properties.SamplerFilter, false));
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, Utils::OpenGLSamplerWrap(instance->m_Properties.SamplerWrap));
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, Utils::OpenGLSamplerWrap(instance->m_Properties.SamplerWrap));
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, Utils::OpenGLSamplerWrap(instance->m_Properties.SamplerWrap));
		});
	}

	OpenGLTextureCube::OpenGLTextureCube(const std::string& path, TextureProperties properties)
		: m_FilePath(path), m_Properties(properties)
	{
		HZ_CORE_ASSERT(false);
#if 0
		// TODO: Revisit this, as currently env maps are being loaded as equirectangular 2D images
		//       so this is an old path
		int width, height, channels;
		stbi_set_flip_vertically_on_load(false);
		m_LocalStorage.Data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);

		m_Width = width;
		m_Height = height;
		m_Format = ImageFormat::RGB;

		uint32_t faceWidth = m_Width / 4;
		uint32_t faceHeight = m_Height / 3;
		HZ_CORE_ASSERT(faceWidth == faceHeight, "Non-square faces!");

		std::array<uint8_t*, 6> faces;
		for (size_t i = 0; i < faces.size(); i++)
			faces[i] = new uint8_t[faceWidth * faceHeight * 3]; // 3 BPP

		int faceIndex = 0;

		for (size_t i = 0; i < 4; i++)
		{
			for (size_t y = 0; y < faceHeight; y++)
			{
				size_t yOffset = y + faceHeight;
				for (size_t x = 0; x < faceWidth; x++)
				{
					size_t xOffset = x + i * faceWidth;
					faces[faceIndex][(x + y * faceWidth) * 3 + 0] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 0];
					faces[faceIndex][(x + y * faceWidth) * 3 + 1] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 1];
					faces[faceIndex][(x + y * faceWidth) * 3 + 2] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 2];
				}
			}
			faceIndex++;
		}

		for (size_t i = 0; i < 3; i++)
		{
			// Skip the middle one
			if (i == 1)
				continue;

			for (size_t y = 0; y < faceHeight; y++)
			{
				size_t yOffset = y + i * faceHeight;
				for (size_t x = 0; x < faceWidth; x++)
				{
					size_t xOffset = x + faceWidth;
					faces[faceIndex][(x + y * faceWidth) * 3 + 0] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 0];
					faces[faceIndex][(x + y * faceWidth) * 3 + 1] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 1];
					faces[faceIndex][(x + y * faceWidth) * 3 + 2] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 2];
				}
			}
			faceIndex++;
		}

		Ref<OpenGLTextureCube> instance = this;
		Renderer::Submit([instance, faceWidth, faceHeight, faces]() mutable
		{
			glGenTextures(1, &instance->m_RendererID);
			glBindTexture(GL_TEXTURE_CUBE_MAP, instance->m_RendererID);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTextureParameterf(instance->m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, Renderer::GetCapabilities().MaxAnisotropy);

			auto format = Utils::OpenGLImageFormat(instance->m_Format);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[2]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[0]);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[4]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[5]);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[1]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[3]);

			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

			glBindTexture(GL_TEXTURE_2D, 0);

			for (size_t i = 0; i < faces.size(); i++)
				delete[] faces[i];

			stbi_image_free(instance->m_ImageData);
		});
#endif
	}

	OpenGLTextureCube::~OpenGLTextureCube()
	{
		GLuint rendererID = m_RendererID;
		Renderer::Submit([rendererID]() {
			glDeleteTextures(1, &rendererID);
		});
	}

	void OpenGLTextureCube::Bind(uint32_t slot) const
	{
		Ref<const OpenGLTextureCube> instance = this;
		Renderer::Submit([instance, slot]() {
			glBindTextureUnit(slot, instance->m_RendererID);
		});
	}

	uint32_t OpenGLTextureCube::GetMipLevelCount() const
	{
		return Utils::CalculateMipCount(m_Width, m_Height);
	}

}