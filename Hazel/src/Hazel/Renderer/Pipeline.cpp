#include "hzpch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "Hazel/Platform/OpenGL/OpenGLPipeline.h"

namespace Hazel {

	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return Ref<OpenGLPipeline>::Create(spec);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}