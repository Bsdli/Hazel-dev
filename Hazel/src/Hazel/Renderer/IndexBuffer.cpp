#include "hzpch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Hazel/Platform/OpenGL/OpenGLIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return Ref<OpenGLIndexBuffer>::Create(size);
			case RendererAPIType::Vulkan:  return Ref<VulkanIndexBuffer>::Create(size);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(void* data, uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return nullptr;
			case RendererAPIType::OpenGL:  return Ref<OpenGLIndexBuffer>::Create(data, size);
			case RendererAPIType::Vulkan:  return Ref<VulkanIndexBuffer>::Create(data, size);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}