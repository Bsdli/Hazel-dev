#include "hzpch.h"
#include "Renderer.h"

#include "Shader.h"

#include <glad/glad.h>

#include "RendererAPI.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

namespace Hazel {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}
	
	void Renderer::RegisterShaderDependency(Ref<Shader> shader, Ref<Material> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void Renderer::OnShaderReloaded(size_t hash)
	{
		if (s_ShaderDependencies.find(hash) != s_ShaderDependencies.end())
		{
			auto& dependencies = s_ShaderDependencies.at(hash);
			for (auto& pipeline : dependencies.Pipelines)
			{
				pipeline->Invalidate();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{	
		// TODO: make sure this is called at a valid time
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		RendererConfig Config;

		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;
	};

	static RendererData* s_Data = nullptr;
	static RenderCommandQueue* s_CommandQueue = nullptr;

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL: return new OpenGLRenderer();
			case RendererAPIType::Vulkan: return new VulkanRenderer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	
	void Renderer::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();
		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// Compute shaders
		Renderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentMipFilter.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/EquirectangularToCubeMap.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentIrradiance.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/PreethamSky.glsl");

		Renderer::GetShaderLibrary()->Load("assets/shaders/Grid.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/SceneComposite.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
		//Renderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Anim.glsl");
		//Renderer::GetShaderLibrary()->Load("assets/shaders/Outline.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/ShadowMap.glsl");

		// Compile shaders
		Renderer::WaitAndRender();

		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture = Texture2D::Create(ImageFormat::RGBA, 1, 1, &whiteTextureData);
		
		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCube::Create(ImageFormat::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		s_RendererAPI->Init();
		SceneRenderer::Init();
	}

	void Renderer::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRenderer::Shutdown();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::WaitAndRender()
	{
		s_CommandQueue->Execute();
	}

	void Renderer::BeginRenderPass(Ref<RenderPass> renderPass, bool clear)
	{
		HZ_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		s_RendererAPI->BeginRenderPass(renderPass);
	}

	void Renderer::EndRenderPass()
	{
		s_RendererAPI->EndRenderPass();
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow)
	{
		s_RendererAPI->SetSceneEnvironment(environment, shadow);
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> Renderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	Ref<TextureCube> Renderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		return s_RendererAPI->CreatePreethamSky(turbidity, azimuth, inclination);
	}

	void Renderer::RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void Renderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

	void Renderer::RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	void Renderer::SubmitQuad(Ref<Material> material, const glm::mat4& transform)
	{
		/*bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		s_Data->m_FullscreenQuadPipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);*/
	}

	void Renderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(pipeline, material);
	}

#if 0
	void Renderer::SubmitFullscreenQuad(Ref<Material> material)
	{
		// Retrieve pipeline from cache
		auto& shader = material->GetShader();
		auto hash = shader->GetHash();
		if (s_PipelineCache.find(hash) == s_PipelineCache.end())
		{
			// Create pipeline
			PipelineSpecification spec = s_Data->m_FullscreenQuadPipelineSpec;
			spec.Shader = shader;
			spec.DebugName = "Renderer-FullscreenQuad-" + shader->GetName();
			s_PipelineCache[hash] = Pipeline::Create(spec);
		}

		auto& pipeline = s_PipelineCache[hash];

		bool depthTest = true;
		bool cullFace = true;
		if (material)
		{
			// material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);
		}

		s_Data->FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}
#endif

	void Renderer::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Submesh& submesh : mesh->m_Submeshes)
		{
			auto& aabb = submesh.BoundingBox;
			auto aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void Renderer::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[i + 4], color);
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}


	Ref<Environment> Renderer::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}

	RendererConfig& Renderer::GetConfig()
	{
		return s_Data->Config;
	}

}
