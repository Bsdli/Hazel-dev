#if 0
#include "Hazel.h"
#include "Hazel/EntryPoint.h"

#include "Hazel/Platform/Vulkan/VulkanTestLayer.h"

using namespace Hazel;

class SandboxLayer : public Layer
{
public:
	SandboxLayer()
	{
	}

	virtual ~SandboxLayer()
	{
	}

	virtual void OnAttach() override
	{
		FramebufferSpecification framebufferSpec;
		framebufferSpec.ClearColor = { 0.0f, 0.8f, 0.0f, 1.0f };
		framebufferSpec.SwapChainTarget = true;

		RenderPassSpecification renderPassSpec;
		//renderPassSpec.TargetFramebuffer = Framebuffer::Create(framebufferSpec);

		//m_RenderPass = RenderPass::Create(renderPassSpec);
	}

	virtual void OnDetach() override
	{
	}

	virtual void OnUpdate(Timestep ts) override
	{
		//Renderer::BeginRenderPass(m_RenderPass);

		//Renderer::EndRenderPass();
	}

	virtual void OnImGuiRender() override
	{
	
	}

	virtual void OnEvent(Hazel::Event& event) override
	{
	}
private:
	Ref<RenderPass> m_RenderPass;
};

class Sandbox : public Hazel::Application
{
public:
	Sandbox()
	{
	}

	virtual void OnInit() override
	{
		//PushLayer(new SandboxLayer());
		PushLayer(new VulkanTestLayer());
	}
};

Hazel::Application* Hazel::CreateApplication()
{
	RendererAPI::SetAPI(RendererAPIType::Vulkan);
	return new Sandbox();
}
#endif