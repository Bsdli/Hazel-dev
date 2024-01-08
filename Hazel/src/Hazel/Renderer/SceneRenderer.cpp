#include "hzpch.h"
#include "SceneRenderer.h"

#include "Renderer.h"
#include "SceneEnvironment.h"

#include <glad/glad.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.h"

#include "Hazel/Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"

#include "Hazel/Platform/OpenGL/OpenGLFramebuffer.h"
#include "Hazel/Platform/OpenGL/OpenGLShader.h"
#include "Hazel/ImGui/ImGui.h"

namespace Hazel {

	struct SceneRendererData
	{
		const Scene* ActiveScene = nullptr;
		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			// Resources
			Ref<Environment> SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			Light ActiveLight;
		} SceneData;

		Ref<Texture2D> BRDFLUT;
		Ref<Shader> CompositeShader;
		Ref<Shader> BloomBlurShader;
		Ref<Shader> BloomBlendShader;

		Ref<RenderPass> GeoPass;
		Ref<RenderPass> CompositePass;
		Ref<RenderPass> BloomBlurPass[2];
		Ref<RenderPass> BloomBlendPass;

		Ref<Shader> ShadowMapShader, ShadowMapAnimShader;
		Ref<RenderPass> ShadowMapRenderPass[4];
		float ShadowMapSize = 20.0f;
		float LightDistance = 0.1f;
		glm::mat4 LightViewMatrix;
		float CascadeSplitLambda = 0.91f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 15.0f, CascadeNearPlaneOffset = -15.0f;
		bool ShowCascades = false;
		bool SoftShadows = true;
		float LightSize = 0.25f;
		float MaxShadowDistance = 200.0f;
		float ShadowFade = 25.0f;
		float CascadeTransitionFade = 1.0f;
		bool CascadeFading = true;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		RendererID ShadowMapSampler;
		Ref<Material> CompositeMaterial;

		Ref<Pipeline> GeometryPipeline;
		Ref<Pipeline> CompositePipeline;
		Ref<Pipeline> ShadowPassPipeline;
		Ref<Pipeline> SkyboxPipeline;
		Ref<Material> SkyboxMaterial;

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<Material> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ColliderDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		// Grid
		Ref<Pipeline> GridPipeline;
		Ref<Shader> GridShader;
		Ref<Material> GridMaterial;
		Ref<Material> OutlineMaterial, OutlineAnimMaterial;
		Ref<Material> ColliderMaterial;

		SceneRendererOptions Options;

		uint32_t ViewportWidth = 0, ViewportHeight = 0;
		bool NeedsResize = false;

		VkDescriptorImageInfo ColorBufferInfo;
	};

	static SceneRendererData* s_Data = nullptr;

	void SceneRenderer::Init()
	{
		s_Data = new SceneRendererData();
		
#if 0
		FramebufferSpecification geoFramebufferSpec;
		//geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA32F, ImageFormat::Depth };
		geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
		geoFramebufferSpec.Samples = 1;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification geoRenderPassSpec;
		geoRenderPassSpec.TargetFramebuffer = Framebuffer::Create(geoFramebufferSpec);
		s_Data->GeoPass = RenderPass::Create(geoRenderPassSpec);

		FramebufferSpecification compFramebufferSpec;
		compFramebufferSpec.Attachments = { ImageFormat::RGBA };
		compFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFramebuffer = Framebuffer::Create(compFramebufferSpec);
		s_Data->CompositePass = RenderPass::Create(compRenderPassSpec);

		FramebufferSpecification bloomBlurFramebufferSpec;
		bloomBlurFramebufferSpec.Attachments = { ImageFormat::RGBA32F };
		bloomBlurFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification bloomBlurRenderPassSpec;
		bloomBlurRenderPassSpec.TargetFramebuffer = Framebuffer::Create(bloomBlurFramebufferSpec);
		s_Data->BloomBlurPass[0] = RenderPass::Create(bloomBlurRenderPassSpec);
		bloomBlurRenderPassSpec.TargetFramebuffer = Framebuffer::Create(bloomBlurFramebufferSpec);
		s_Data->BloomBlurPass[1] = RenderPass::Create(bloomBlurRenderPassSpec);

		FramebufferSpecification bloomBlendFramebufferSpec;
		bloomBlendFramebufferSpec.Attachments = { ImageFormat::RGBA };
		bloomBlendFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification bloomBlendRenderPassSpec;
		bloomBlendRenderPassSpec.TargetFramebuffer = Framebuffer::Create(bloomBlendFramebufferSpec);
		s_Data->BloomBlendPass = RenderPass::Create(bloomBlendRenderPassSpec);
#endif


#if 0
		s_Data->CompositeShader = Shader::Create("assets/shaders/SceneComposite.glsl");
		s_Data->BloomBlurShader = Shader::Create("assets/shaders/BloomBlur.glsl");
		s_Data->BloomBlendShader = Shader::Create("assets/shaders/BloomBlend.glsl");
#endif

		s_Data->CompositeShader = Renderer::GetShaderLibrary()->Get("SceneComposite");
		s_Data->CompositeMaterial = Material::Create(s_Data->CompositeShader);
		s_Data->BRDFLUT = Texture2D::Create("assets/textures/BRDF_LUT.tga");

		// Shadow pass
		{
			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.Width = 4096;
			shadowMapFramebufferSpec.Height = 4096;
			shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
			shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFramebufferSpec.NoResize = true;

			// 4 cascades
			for (int i = 0; i < 4; i++)
			{
				RenderPassSpecification shadowMapRenderPassSpec;
				shadowMapRenderPassSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
				shadowMapRenderPassSpec.DebugName = "ShadowMap";
				s_Data->ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			}

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("ShadowMap");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "ShadowPass";
			pipelineSpec.Shader = shadowPassShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = s_Data->ShadowMapRenderPass[0];
			s_Data->ShadowPassPipeline = Pipeline::Create(pipelineSpec);
		}
		
		// Geometry
		{
			FramebufferSpecification geoFramebufferSpec;
			geoFramebufferSpec.Width = 1280;
			geoFramebufferSpec.Height = 720;
			geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::RGBA32F, ImageFormat::Depth };
			geoFramebufferSpec.Samples = 1;
			geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

			Ref<Framebuffer> framebuffer = Framebuffer::Create(geoFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "Geometry";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			s_Data->GeometryPipeline = Pipeline::Create(pipelineSpecification);
		}

		// Composite
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.Width = 1280;
			compFramebufferSpec.Height = 720;
			compFramebufferSpec.Attachments = { ImageFormat::RGBA };
			compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

			Ref<Framebuffer> framebuffer = Framebuffer::Create(compFramebufferSpec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			renderPassSpec.DebugName = "Composite";
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			s_Data->CompositePipeline = Pipeline::Create(pipelineSpecification);
		}

		// Grid
		{
			s_Data->GridShader = Renderer::GetShaderLibrary()->Get("Grid");
			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			s_Data->GridMaterial = Material::Create(s_Data->GridShader);
			s_Data->GridMaterial->Set("u_Settings.Scale", gridScale);
			s_Data->GridMaterial->Set("u_Settings.Size", gridSize);

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = s_Data->GridShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = s_Data->GeometryPipeline->GetSpecification().RenderPass;
			s_Data->GridPipeline = Pipeline::Create(pipelineSpec);
		}

		// Collider
		//auto colliderShader = Shader::Create("assets/shaders/Collider.glsl");
		//s_Data->ColliderMaterial = Material::Create(Material::Create(colliderShader));
		//s_Data->ColliderMaterial->SetFlag(MaterialFlag::DepthTest, false);

#if 0
		// Shadow Map
		s_Data->ShadowMapShader = Shader::Create("assets/shaders/ShadowMap.glsl");
		s_Data->ShadowMapAnimShader = Shader::Create("assets/shaders/ShadowMap_Anim.glsl");

		FramebufferSpecification shadowMapFramebufferSpec;
		shadowMapFramebufferSpec.Width = 4096;
		shadowMapFramebufferSpec.Height = 4096;
		shadowMapFramebufferSpec.Attachments = { FramebufferTextureFormat::DEPTH32F };
		shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		shadowMapFramebufferSpec.NoResize = true;

		// 4 cascades
		for (int i = 0; i < 4; i++)
		{
			RenderPassSpecification shadowMapRenderPassSpec;
			shadowMapRenderPassSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
			s_Data->ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
		}

		Renderer::Submit([]()
		{
			glGenSamplers(1, &s_Data->ShadowMapSampler);

			// Setup the shadowmap depth sampler
			glSamplerParameteri(s_Data->ShadowMapSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(s_Data->ShadowMapSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glSamplerParameteri(s_Data->ShadowMapSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(s_Data->ShadowMapSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		});
#endif
		// Skybox
		{
			auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");

			PipelineSpecification pipelineSpec;
			pipelineSpec.DebugName = "Skybox";
			pipelineSpec.Shader = skyboxShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.RenderPass = s_Data->GeometryPipeline->GetSpecification().RenderPass;
			s_Data->SkyboxPipeline = Pipeline::Create(pipelineSpec);

			s_Data->SkyboxMaterial = Material::Create(skyboxShader);
			s_Data->SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, false);
		}
	}

	void SceneRenderer::Shutdown()
	{
		delete s_Data;
	}
	
	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (s_Data->ViewportWidth != width || s_Data->ViewportHeight != height)
		{
			s_Data->ViewportWidth = width;
			s_Data->ViewportHeight = height;
			s_Data->NeedsResize = true;
		}
	}

	struct FrustumBounds
	{
		float r, l, b, t, f, n;
	};

	struct CascadeData
	{
		glm::mat4 ViewProj;
		glm::mat4 View;
		float SplitDepth;
	};

	static void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const glm::vec3& lightDirection)
	{
		FrustumBounds frustumBounds[3];

		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		// TODO: less hard-coding!
		float nearClip = 0.1f;
		float farClip = 1000.0f;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = s_Data->CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Manually set cascades here
		// cascadeSplits[0] = 0.05f;
		// cascadeSplits[1] = 0.15f;
		// cascadeSplits[2] = 0.3f;
		// cascadeSplits[3] = 1.0f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + s_Data->CascadeNearPlaneOffset, maxExtents.z - minExtents.z + s_Data->CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			const float ShadowMapResolution = 4096.0f;
			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::BeginScene(const Scene* scene, const SceneRendererCamera& camera)
	{
		HZ_CORE_ASSERT(!s_Data->ActiveScene, "");

		s_Data->ActiveScene = scene;

		s_Data->SceneData.SceneCamera = camera;
		s_Data->SceneData.SceneEnvironment = scene->m_Environment;
		s_Data->SceneData.SceneEnvironmentIntensity = scene->m_EnvironmentIntensity;
		s_Data->SceneData.ActiveLight = scene->m_Light;
		s_Data->SceneData.SceneLightEnvironment = scene->m_LightEnvironment;
		s_Data->SceneData.SkyboxLod = scene->m_SkyboxLod;
		s_Data->SceneData.ActiveLight = scene->m_Light;

		if (s_Data->NeedsResize)
		{
			s_Data->GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(s_Data->ViewportWidth, s_Data->ViewportHeight);
			s_Data->CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(s_Data->ViewportWidth, s_Data->ViewportHeight);
			s_Data->NeedsResize = false;
		}

		Renderer::SetSceneEnvironment(s_Data->SceneData.SceneEnvironment, s_Data->ShadowPassPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetDepthImage());

		auto& sceneCamera = s_Data->SceneData.SceneCamera;
		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * s_Data->SceneData.SceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(sceneCamera.ViewMatrix)[3];

		// TODO: handle uniform buffers better
		const DirectionalLight& directionalLight = s_Data->SceneData.SceneLightEnvironment.DirectionalLights[0];
		Renderer::Submit([viewProjection, sceneCamera, cameraPosition, directionalLight]()
		{
			{
				auto inverseVP = glm::inverse(viewProjection);
				struct ViewProj
				{
					glm::mat4 ViewProjection;
					glm::mat4 InverseViewProjection;
				};
				ViewProj viewProj;
				viewProj.ViewProjection = viewProjection;
				viewProj.InverseViewProjection = inverseVP;
				//viewProj.LightMatrixCascade = s_Data->LightMatrices[0];

				struct Light
				{
					glm::vec3 Direction;
					float Padding = 0.0f;
					glm::vec3 Radiance;
					float Multiplier;
				};

				struct SceneData
				{
					Light lights;
					glm::vec3 u_CameraPosition;
				};


				SceneData ub;
				ub.lights.Direction = directionalLight.Direction;
				ub.lights.Radiance = directionalLight.Radiance;
				ub.lights.Multiplier = directionalLight.Multiplier;

				ub.u_CameraPosition = cameraPosition;

				if (RendererAPI::Current() == RendererAPIType::Vulkan)
				{
					void* ubPtr = VulkanShader::MapUniformBuffer(0);
					memcpy(ubPtr, &viewProj, sizeof(ViewProj));
					VulkanShader::UnmapUniformBuffer(0);

					ubPtr = VulkanShader::MapUniformBuffer(2);
					memcpy(ubPtr, &ub, sizeof(SceneData));
					VulkanShader::UnmapUniformBuffer(2);
				}
				else
				{
					auto shader = s_Data->GridMaterial->GetShader().As<OpenGLShader>();
					shader->SetUniformBuffer("Camera", &viewProj, sizeof(ViewProj));
					shader->SetUniformBuffer("SceneData", &ub, sizeof(SceneData));
				}
			}

			{
				CascadeData cascades[4];
				CalculateCascades(cascades, sceneCamera, directionalLight.Direction);
				s_Data->LightViewMatrix = cascades[0].View;

				// TODO: change to four cascades (or set number)
				for (int i = 0; i < 1; i++)
				{
					s_Data->CascadeSplits[i] = cascades[i].SplitDepth;
				}

				struct ShadowData
				{
					glm::mat4 ViewProjection;
				};
				ShadowData shadowData;
				shadowData.ViewProjection = cascades[0].ViewProj;

				if (RendererAPI::Current() == RendererAPIType::Vulkan)
				{
					void* ubPtr = VulkanShader::MapUniformBuffer(1);
					memcpy(ubPtr, &shadowData, sizeof(ShadowData));
					VulkanShader::UnmapUniformBuffer(1);
				}
				else
				{
					auto shadowPassShader = s_Data->ShadowPassPipeline->GetSpecification().Shader;
					auto shader = shadowPassShader.As<OpenGLShader>();
					shader->SetUniformBuffer("ShadowData", &shadowData, sizeof(ShadowData));
				}
			}

			{
#if 0
				SceneData ub;
				ub.lights.Direction = directionalLight.Direction;
				ub.lights.Radiance = directionalLight.Radiance;
				ub.lights.Multiplier = directionalLight.Multiplier;

				ub.u_CameraPosition = cameraPosition;

				if (RendererAPI::Current() == RendererAPIType::Vulkan)
				{
					void* ubPtr = VulkanShader::MapUniformBuffer(0);
					memcpy(ubPtr, &viewProj, sizeof(ViewProj));
					VulkanShader::UnmapUniformBuffer(0);

					ubPtr = VulkanShader::MapUniformBuffer(2);
					memcpy(ubPtr, &ub, sizeof(SceneData));
					VulkanShader::UnmapUniformBuffer(2);
				}
				else
				{
					auto shader = s_Data->GridMaterial->GetShader().As<OpenGLShader>();
					shader->SetUniformBuffer("Camera", &viewProj, sizeof(ViewProj));
					shader->SetUniformBuffer("SceneData", &ub, sizeof(SceneData));
				}
#endif
			}
		});
	}

	void SceneRenderer::EndScene()
	{
		HZ_CORE_ASSERT(s_Data->ActiveScene, "");

		s_Data->ActiveScene = nullptr;

		FlushDrawList();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data->DrawList.push_back({ mesh, overrideMaterial, transform });
		s_Data->ShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_Data->SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
		s_Data->ShadowPassDrawList.push_back({ mesh, nullptr, transform });
	}

	void SceneRenderer::SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform)
	{
		s_Data->ColliderDrawList.push_back({ component.DebugMesh, nullptr, glm::translate(parentTransform, component.Offset) });
	}

	void SceneRenderer::SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform)
	{
		s_Data->ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform)
	{
		s_Data->ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform)
	{
		for (auto debugMesh : component.ProcessedMeshes)
			s_Data->ColliderDrawList.push_back({ debugMesh, nullptr, parentTransform });
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return Renderer::CreateEnvironmentMap(filepath);
	}

	void SceneRenderer::ShadowMapPass()
	{
		auto& directionalLights = s_Data->SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
		{
			for (int i = 0; i < 4; i++)
			{
				// Clear shadow maps
				Renderer::BeginRenderPass(s_Data->ShadowMapRenderPass[i]);
				Renderer::EndRenderPass();
			}
			return;
		}

		// TODO: change to four cascades (or set number)
		for (int i = 0; i < 1; i++)
		{
			Renderer::BeginRenderPass(s_Data->ShadowMapRenderPass[i]);

			// static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
			
			// Render entities
			for (auto& dc : s_Data->ShadowPassDrawList)
			{
				Renderer::RenderMeshWithoutMaterial(s_Data->ShadowPassPipeline, dc.Mesh, dc.Transform);
			}

			Renderer::EndRenderPass();
		}
	}
	
	void SceneRenderer::GeometryPass()
	{
		Renderer::BeginRenderPass(s_Data->GeometryPipeline->GetSpecification().RenderPass);
		// Skybox
		s_Data->SkyboxMaterial->Set("u_Uniforms.TextureLod", s_Data->SceneData.SkyboxLod);

		Ref<TextureCube> radianceMap = s_Data->SceneData.SceneEnvironment ? s_Data->SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		s_Data->SkyboxMaterial->Set("u_Texture", radianceMap);
		Renderer::SubmitFullscreenQuad(s_Data->SkyboxPipeline, s_Data->SkyboxMaterial);

		// Render entities
		for (auto& dc : s_Data->DrawList)
			Renderer::RenderMesh(s_Data->GeometryPipeline, dc.Mesh, dc.Transform);

		for (auto& dc : s_Data->SelectedMeshDrawList)
			Renderer::RenderMesh(s_Data->GeometryPipeline, dc.Mesh, dc.Transform);

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
			Renderer::RenderQuad(s_Data->GridPipeline, s_Data->GridMaterial, transform);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
#if 0
			Renderer2D::BeginScene(viewProjection);
			for (auto& dc : s_Data->DrawList)
				Renderer::DrawAABB(dc.Mesh, dc.Transform);
			Renderer2D::EndScene();
#endif
		}

		Renderer::EndRenderPass();
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginRenderPass(s_Data->CompositePipeline->GetSpecification().RenderPass);

		auto framebuffer = s_Data->GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
		// float exposure = s_Data->SceneData.SceneCamera.Camera.GetExposure();
		float exposure = s_Data->SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = framebuffer->GetSpecification().Samples;

		s_Data->CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		//s_Data->CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		s_Data->CompositeMaterial->Set("u_Texture", framebuffer->GetImage());

		Renderer::SubmitFullscreenQuad(s_Data->CompositePipeline, s_Data->CompositeMaterial);
		Renderer::EndRenderPass();
	}

	void SceneRenderer::BloomBlurPass()
	{
#if 0
		int amount = 10;
		int index = 0;

		int horizontalCounter = 0, verticalCounter = 0;
		for (int i = 0; i < amount; i++)
		{
			index = i % 2;
			Renderer::BeginRenderPass(s_Data->BloomBlurPass[index]);
			s_Data->BloomBlurShader->Bind();
			s_Data->BloomBlurShader->SetBool("u_Horizontal", index);
			if (index)
				horizontalCounter++;
			else
				verticalCounter++;
			if (i > 0)
			{
				auto fb = s_Data->BloomBlurPass[1 - index]->GetSpecification().TargetFramebuffer;
				fb->BindTexture();
			}
			else
			{
				auto fb = s_Data->CompositePass->GetSpecification().TargetFramebuffer;
				auto id = fb->GetColorAttachmentRendererID(1);
				Renderer::Submit([id]()
					{
						glBindTextureUnit(0, id);
					});
			}
			Renderer::SubmitFullscreenQuad(nullptr);
			Renderer::EndRenderPass();
		}

		// Composite bloom
		{
			Renderer::BeginRenderPass(s_Data->BloomBlendPass);
			s_Data->BloomBlendShader->Bind();
			s_Data->BloomBlendShader->SetFloat("u_Exposure", s_Data->SceneData.SceneCamera.Camera.GetExposure());
			s_Data->BloomBlendShader->SetBool("u_EnableBloom", s_Data->EnableBloom);

			s_Data->CompositePass->GetSpecification().TargetFramebuffer->BindTexture(0);
			s_Data->BloomBlurPass[index]->GetSpecification().TargetFramebuffer->BindTexture(1);

			Renderer::SubmitFullscreenQuad(nullptr);
			Renderer::EndRenderPass();
		}
#endif
	}

	void SceneRenderer::FlushDrawList()
	{
		HZ_CORE_ASSERT(!s_Data->ActiveScene, "");

		ShadowMapPass();
		GeometryPass();
		CompositePass();
		//	BloomBlurPass();

		s_Data->DrawList.clear();
		s_Data->SelectedMeshDrawList.clear();
		s_Data->ShadowPassDrawList.clear();
		s_Data->ColliderDrawList.clear();
		s_Data->SceneData = {};
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return s_Data->CompositePipeline->GetSpecification().RenderPass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage()
	{
		return s_Data->CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return s_Data->Options;
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::s_AllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload(true);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", s_Data->SoftShadows);
			UI::Property("Light Size", s_Data->LightSize, 0.01f);
			UI::Property("Max Shadow Distance", s_Data->MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", s_Data->ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Show Cascades", s_Data->ShowCascades);
				UI::Property("Cascade Fading", s_Data->CascadeFading);
				UI::Property("Cascade Transition Fade", s_Data->CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", s_Data->CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", s_Data->CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", s_Data->CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = s_Data->ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFramebuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				UI::Image(image, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

#if 0
		if (UI::BeginTreeNode("Bloom"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Bloom", s_Data->EnableBloom);
			UI::Property("Bloom threshold", s_Data->BloomThreshold, 0.05f);
			UI::EndPropertyGrid();

			auto fb = s_Data->BloomBlurPass[0]->GetSpecification().TargetFramebuffer;
			auto id = fb->GetColorAttachmentRendererID();

			float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
			float w = size;
			float h = w / ((float)fb->GetWidth() / (float)fb->GetHeight());
			ImGui::Image((ImTextureID)id, { w, h }, { 0, 1 }, { 1, 0 });
			UI::EndTreeNode();
		}
#endif

		ImGui::End();
	}

}
