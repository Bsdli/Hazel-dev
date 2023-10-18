#include "hzpch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"

namespace Hazel {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPIType::OpenGL:  return std::make_shared<OpenGLRenderPass>(spec);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}