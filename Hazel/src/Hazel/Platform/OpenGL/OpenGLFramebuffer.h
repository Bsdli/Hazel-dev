#pragma once

#include "Hazel/Renderer/Framebuffer.h"

namespace Hazel {

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual RendererID GetRendererID() const { return m_RendererID; }
		virtual RendererID GetColorAttachmentRendererID(int index = 0) const { return m_ColorAttachments[index]; }
		virtual RendererID GetDepthAttachmentRendererID() const { return m_DepthAttachment; }

		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
	private:
		FramebufferSpecification m_Specification;
		RendererID m_RendererID = 0;

		std::vector<RendererID> m_ColorAttachments;
		RendererID m_DepthAttachment;

		std::vector<FramebufferTextureFormat> m_ColorAttachmentFormats;
		FramebufferTextureFormat m_DepthAttachmentFormat = FramebufferTextureFormat::None;

		uint32_t m_Width = 0, m_Height = 0;
	};

}