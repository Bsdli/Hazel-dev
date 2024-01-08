#include "hzpch.h"
#include "OpenGLFramebuffer.h"

#include "Hazel/Renderer/Renderer.h"
#include <glad/glad.h>

#include "OpenGLImage.h"

namespace Hazel {

	namespace Utils
	{
		static GLenum TextureTarget(bool multisampled)
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures(bool multisampled, RendererID* outID, uint32_t count)
		{
			glCreateTextures(TextureTarget(multisampled), 1, outID);
		}

		static void BindTexture(bool multisampled, RendererID id)
		{
			glBindTexture(TextureTarget(multisampled), id);
		}

		static GLenum DataType(GLenum format)
		{
			switch (format)
			{
				case GL_RGBA8:             return GL_UNSIGNED_BYTE;
				case GL_RG16F:
				case GL_RG32F:
				case GL_RGBA16F:
				case GL_RGBA32F:           return GL_FLOAT;
				case GL_DEPTH24_STENCIL8:  return GL_UNSIGNED_INT_24_8;
			}

			HZ_CORE_ASSERT(false, "Unknown format");
			return 0;
		}

		static GLenum DepthAttachmentType(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::DEPTH32F:        return GL_DEPTH_ATTACHMENT;
				case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH_STENCIL_ATTACHMENT;
			}
			HZ_CORE_ASSERT(false, "Unknown format");
			return 0;
		}

		static Ref<Image2D> CreateAndAttachColorAttachment(int samples, ImageFormat format, uint32_t width, uint32_t height, int index)
		{
			bool multisampled = samples > 1;
			Ref<Image2D> image;
			if (multisampled)
			{
				//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				image = Image2D::Create(format, width, height);
				image->Invalidate();
			}

			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			glImage->CreateSampler(TextureProperties());
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), glImage->GetRendererID(), 0);
			return image;
		}

		static Ref<Image2D> AttachDepthTexture(int samples, ImageFormat format, uint32_t width, uint32_t height)
		{
#if 0
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(TextureTarget(multisampled), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
#endif
			bool multisampled = samples > 1;
			Ref<Image2D> image;
			if (multisampled)
			{
				//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				image = Image2D::Create(format, width, height);
				image->Invalidate();
			}

			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			glImage->CreateSampler(TextureProperties());
			glFramebufferTexture2D(GL_FRAMEBUFFER, Utils::DepthAttachmentType(format), TextureTarget(multisampled), glImage->GetRendererID(), 0);
			return image;
			
		}

		static bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::DEPTH24STENCIL8:
				case ImageFormat::DEPTH32F:
					return true;
			}
			return false;
		}

	}

	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
		: m_Specification(spec), m_Width(spec.Width), m_Height(spec.Height)
	{
		HZ_CORE_ASSERT(spec.Attachments.Attachments.size());
		for (auto format : m_Specification.Attachments.Attachments)
		{
			if (!Utils::IsDepthFormat(format.Format))
				m_ColorAttachmentFormats.emplace_back(format.Format);
			else
				m_DepthAttachmentFormat = format.Format;
		}

		uint32_t width = spec.Width;
		uint32_t height = spec.Height;
		if (width == 0)
		{
			width = Application::Get().GetWindow().GetWidth();
			height = Application::Get().GetWindow().GetHeight();
		}
		Resize(width, height, true);
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		Ref<OpenGLFramebuffer> instance = this;
		Renderer::Submit([instance]() {
			glDeleteFramebuffers(1, &instance->m_RendererID);
		});
	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && (m_Width == width && m_Height == height))
			return;

		m_Width = width;
		m_Height = height;

		Ref<OpenGLFramebuffer> instance = this;
		Renderer::Submit([instance]() mutable
		{
			if (instance->m_RendererID)
			{
				glDeleteFramebuffers(1, &instance->m_RendererID);

				instance->m_ColorAttachments.clear();
			}

			glGenFramebuffers(1, &instance->m_RendererID);
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_RendererID);

			if (instance->m_ColorAttachmentFormats.size())
			{
				instance->m_ColorAttachments.resize(instance->m_ColorAttachmentFormats.size());

				// Create color attachments
				for (size_t i = 0; i < instance->m_ColorAttachments.size(); i++)
					instance->m_ColorAttachments[i] = Utils::CreateAndAttachColorAttachment(instance->m_Specification.Samples, instance->m_ColorAttachmentFormats[i], instance->m_Width, instance->m_Height, i);
			}

			if (instance->m_DepthAttachmentFormat != ImageFormat::None)
			{
				instance->m_DepthAttachment = Utils::AttachDepthTexture(instance->m_Specification.Samples, instance->m_DepthAttachmentFormat, instance->m_Width, instance->m_Height);
			}

			if (instance->m_ColorAttachments.size() > 1)
			{
				HZ_CORE_ASSERT(instance->m_ColorAttachments.size() <= 4);
				GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
				glDrawBuffers(instance->m_ColorAttachments.size(), buffers);
			}
			else if (instance->m_ColorAttachments.empty())
			{
				// Only depth-pass
				glDrawBuffer(GL_NONE);
			}

			HZ_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		});
	}

	void OpenGLFramebuffer::Bind() const
	{
		Ref<const OpenGLFramebuffer> instance = this;
		Renderer::Submit([instance]() {
			glBindFramebuffer(GL_FRAMEBUFFER, instance->m_RendererID);
			glViewport(0, 0, instance->m_Width, instance->m_Height);
		});
	}

	void OpenGLFramebuffer::Unbind() const
	{
		Renderer::Submit([]() {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		});
	}

	void OpenGLFramebuffer::BindTexture(uint32_t attachmentIndex, uint32_t slot) const
	{
		Ref<const OpenGLFramebuffer> instance = this;
		Renderer::Submit([instance, attachmentIndex, slot]() {
			glBindTextureUnit(slot, instance->m_ColorAttachments[attachmentIndex]);
		});
	}
}
