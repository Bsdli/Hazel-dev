#include "hzpch.h"
#include "SceneRenderer.h"

#include "Renderer.h"
#include "SceneEnvironment.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.h"

#include "Hazel/ImGui/ImGui.h"

#include "Hazel/Core/Timer.h"

#include <limits>

namespace Hazel {

	struct SceneRendererData
	{
		const Scene* ActiveScene = nullptr;
		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			// Resources
			Ref<MaterialInstance> SkyboxMaterial;
			Environment SceneEnvironment;
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
		glm::mat4 LightMatrices[4];
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

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<MaterialInstance> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ColliderDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		// Grid
		Ref<MaterialInstance> GridMaterial;
		Ref<MaterialInstance> OutlineMaterial, OutlineAnimMaterial;
		Ref<MaterialInstance> ColliderMaterial;

		SceneRendererOptions Options;
	};

	struct SceneRendererStats
	{
		float ShadowPass = 0.0f;
		float GeometryPass = 0.0f;
		float CompositePass = 0.0f;

		Timer ShadowPassTimer;
		Timer GeometryPassTimer;
		Timer CompositePassTimer;
	};

	static SceneRendererData s_Data;
	static SceneRendererStats s_Stats;

	void SceneRenderer::Init()
	{
		FramebufferSpecification geoFramebufferSpec;
		geoFramebufferSpec.Attachments = { FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::RGBA16F, FramebufferTextureFormat::Depth };
		geoFramebufferSpec.Samples = 8;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification geoRenderPassSpec;
		geoRenderPassSpec.TargetFramebuffer = Framebuffer::Create(geoFramebufferSpec);
		s_Data.GeoPass = RenderPass::Create(geoRenderPassSpec);

		FramebufferSpecification compFramebufferSpec;
		compFramebufferSpec.Attachments = { FramebufferTextureFormat::RGBA8 };
		compFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFramebuffer = Framebuffer::Create(compFramebufferSpec);
		s_Data.CompositePass = RenderPass::Create(compRenderPassSpec);

		FramebufferSpecification bloomBlurFramebufferSpec;
		bloomBlurFramebufferSpec.Attachments = { FramebufferTextureFormat::RGBA16F };
		bloomBlurFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification bloomBlurRenderPassSpec;
		bloomBlurRenderPassSpec.TargetFramebuffer = Framebuffer::Create(bloomBlurFramebufferSpec);
		s_Data.BloomBlurPass[0] = RenderPass::Create(bloomBlurRenderPassSpec);
		bloomBlurRenderPassSpec.TargetFramebuffer = Framebuffer::Create(bloomBlurFramebufferSpec);
		s_Data.BloomBlurPass[1] = RenderPass::Create(bloomBlurRenderPassSpec);

		FramebufferSpecification bloomBlendFramebufferSpec;
		bloomBlendFramebufferSpec.Attachments = { FramebufferTextureFormat::RGBA8 };
		bloomBlendFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification bloomBlendRenderPassSpec;
		bloomBlendRenderPassSpec.TargetFramebuffer = Framebuffer::Create(bloomBlendFramebufferSpec);
		s_Data.BloomBlendPass = RenderPass::Create(bloomBlendRenderPassSpec);

		s_Data.CompositeShader = Shader::Create("assets/shaders/SceneComposite.glsl");
		s_Data.BloomBlurShader = Shader::Create("assets/shaders/BloomBlur.glsl");
		s_Data.BloomBlendShader = Shader::Create("assets/shaders/BloomBlend.glsl");
		s_Data.BRDFLUT = Texture2D::Create("assets/textures/BRDF_LUT.tga");

		// Grid
		auto gridShader = Shader::Create("assets/shaders/Grid.glsl");
		s_Data.GridMaterial = MaterialInstance::Create(Material::Create(gridShader));
		s_Data.GridMaterial->SetFlag(MaterialFlag::TwoSided, true);
		float gridScale = 16.025f, gridSize = 0.025f;
		s_Data.GridMaterial->Set("u_Scale", gridScale);
		s_Data.GridMaterial->Set("u_Res", gridSize);

		// Outline
		auto outlineShader = Shader::Create("assets/shaders/Outline.glsl");
		s_Data.OutlineMaterial = MaterialInstance::Create(Material::Create(outlineShader));
		s_Data.OutlineMaterial->SetFlag(MaterialFlag::DepthTest, false);

		auto outlineAnimShader = Shader::Create("assets/shaders/Outline_Anim.glsl");
		s_Data.OutlineAnimMaterial = MaterialInstance::Create(Material::Create(outlineAnimShader));
		s_Data.OutlineAnimMaterial->SetFlag(MaterialFlag::DepthTest, false);

		// Collider
		auto colliderShader = Shader::Create("assets/shaders/Collider.glsl");
		s_Data.ColliderMaterial = MaterialInstance::Create(Material::Create(colliderShader));
		s_Data.ColliderMaterial->SetFlag(MaterialFlag::DepthTest, false);

		// Shadow Map
		s_Data.ShadowMapShader = Shader::Create("assets/shaders/ShadowMap.glsl");
		s_Data.ShadowMapAnimShader = Shader::Create("assets/shaders/ShadowMap_Anim.glsl");

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
			s_Data.ShadowMapRenderPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
		}

		Renderer::Submit([]()
		{
			glGenSamplers(1, &s_Data.ShadowMapSampler);

			// Setup the shadowmap depth sampler
			glSamplerParameteri(s_Data.ShadowMapSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(s_Data.ShadowMapSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glSamplerParameteri(s_Data.ShadowMapSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(s_Data.ShadowMapSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		});
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
		s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
	}

	void SceneRenderer::BeginScene(const Scene* scene, const SceneRendererCamera& camera)
	{
		HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

		s_Data.ActiveScene = scene;

		s_Data.SceneData.SceneCamera = camera;
		s_Data.SceneData.SkyboxMaterial = scene->m_SkyboxMaterial;
		s_Data.SceneData.SceneEnvironment = scene->m_Environment;
		s_Data.SceneData.SceneEnvironmentIntensity = scene->m_EnvironmentIntensity;
		s_Data.SceneData.ActiveLight = scene->m_Light;
		s_Data.SceneData.SceneLightEnvironment = scene->m_LightEnvironment;
	}

	void SceneRenderer::EndScene()
	{
		HZ_CORE_ASSERT(s_Data.ActiveScene, "");

		s_Data.ActiveScene = nullptr;

		FlushDrawList();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<MaterialInstance> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data.DrawList.push_back({ mesh, overrideMaterial, transform });
		s_Data.ShadowPassDrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_Data.SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
		s_Data.ShadowPassDrawList.push_back({ mesh, nullptr, transform });
	}

	void SceneRenderer::SubmitColliderMesh(const BoxColliderComponent& component, const glm::mat4& parentTransform)
	{
		s_Data.ColliderDrawList.push_back({ component.DebugMesh, nullptr, glm::translate(parentTransform, component.Offset) });
	}

	void SceneRenderer::SubmitColliderMesh(const SphereColliderComponent& component, const glm::mat4& parentTransform)
	{
		s_Data.ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const CapsuleColliderComponent& component, const glm::mat4& parentTransform)
	{
		s_Data.ColliderDrawList.push_back({ component.DebugMesh, nullptr, parentTransform });
	}

	void SceneRenderer::SubmitColliderMesh(const MeshColliderComponent& component, const glm::mat4& parentTransform)
	{
		for (auto debugMesh : component.ProcessedMeshes)
			s_Data.ColliderDrawList.push_back({ debugMesh, nullptr, parentTransform });
	}

	static Ref<Shader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

	std::pair<Ref<TextureCube>, Ref<TextureCube>> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		const uint32_t cubemapSize = 1024;
		const uint32_t irradianceMapSize = 32;

		Ref<TextureCube> envUnfiltered = TextureCube::Create(TextureFormat::Float16, cubemapSize, cubemapSize);
		if (!equirectangularConversionShader)
			equirectangularConversionShader = Shader::Create("assets/shaders/EquirectangularToCubeMap.glsl");
		Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
		HZ_CORE_ASSERT(envEquirect->GetFormat() == TextureFormat::Float16, "Texture is not HDR!");

		equirectangularConversionShader->Bind();
		envEquirect->Bind();
		Renderer::Submit([envUnfiltered, cubemapSize, envEquirect]()
			{
				glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
				glGenerateTextureMipmap(envUnfiltered->GetRendererID());
			});

		if (!envFilteringShader)
			envFilteringShader = Shader::Create("assets/shaders/EnvironmentMipFilter.glsl");

		Ref<TextureCube> envFiltered = TextureCube::Create(TextureFormat::Float16, cubemapSize, cubemapSize);

		Renderer::Submit([envUnfiltered, envFiltered]()
			{
				glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
					envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
					envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
			});

		envFilteringShader->Bind();
		envUnfiltered->Bind();

		Renderer::Submit([envUnfiltered, envFiltered, cubemapSize]() {
			const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
			for (int level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
			{
				const GLuint numGroups = glm::max(1, size / 32);
				glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glProgramUniform1f(envFilteringShader->GetRendererID(), 0, level * deltaRoughness);
				glDispatchCompute(numGroups, numGroups, 6);
			}
			});

		if (!envIrradianceShader)
			envIrradianceShader = Shader::Create("assets/shaders/EnvironmentIrradiance.glsl");

		Ref<TextureCube> irradianceMap = TextureCube::Create(TextureFormat::Float16, irradianceMapSize, irradianceMapSize);
		envIrradianceShader->Bind();
		envFiltered->Bind();
		Renderer::Submit([irradianceMap]()
			{
				glBindImageTexture(0, irradianceMap->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
				glGenerateTextureMipmap(irradianceMap->GetRendererID());
			});

		return { envFiltered, irradianceMap };
	}

	void SceneRenderer::GeometryPass()
	{
		bool outline = s_Data.SelectedMeshDrawList.size() > 0;
		bool collider = s_Data.ColliderDrawList.size() > 0;

		if (outline || collider)
		{
			Renderer::Submit([]()
			{
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			});
		}

		Renderer::BeginRenderPass(s_Data.GeoPass);

		if (outline || collider)
		{
			Renderer::Submit([]()
			{
				glStencilMask(0);
			});
		}

		auto& sceneCamera = s_Data.SceneData.SceneCamera;

		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.ViewMatrix)[3]; // TODO: Negate instead

		// Skybox
		auto skyboxShader = s_Data.SceneData.SkyboxMaterial->GetShader();
		s_Data.SceneData.SkyboxMaterial->Set("u_InverseVP", glm::inverse(viewProjection));
		s_Data.SceneData.SkyboxMaterial->Set("u_SkyIntensity", s_Data.SceneData.SceneEnvironmentIntensity);
		Renderer::SubmitFullscreenQuad(s_Data.SceneData.SkyboxMaterial);

		float aspectRatio = (float)s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetWidth() / (float)s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetHeight();
		float frustumSize = 2.0f * sceneCamera.Near * glm::tan(sceneCamera.FOV * 0.5f) * aspectRatio;

		// Render entities
		for (auto& dc : s_Data.DrawList)
		{
			auto baseMaterial = dc.Mesh->GetMaterial();
			baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
			baseMaterial->Set("u_ViewMatrix", sceneCamera.ViewMatrix);
			baseMaterial->Set("u_CameraPosition", cameraPosition);
			baseMaterial->Set("u_LightMatrixCascade0", s_Data.LightMatrices[0]);
			baseMaterial->Set("u_LightMatrixCascade1", s_Data.LightMatrices[1]);
			baseMaterial->Set("u_LightMatrixCascade2", s_Data.LightMatrices[2]);
			baseMaterial->Set("u_LightMatrixCascade3", s_Data.LightMatrices[3]);
			baseMaterial->Set("u_ShowCascades", s_Data.ShowCascades);
			baseMaterial->Set("u_LightView", s_Data.LightViewMatrix);
			baseMaterial->Set("u_CascadeSplits", s_Data.CascadeSplits);
			baseMaterial->Set("u_SoftShadows", s_Data.SoftShadows);
			baseMaterial->Set("u_LightSize", s_Data.LightSize);
			baseMaterial->Set("u_MaxShadowDistance", s_Data.MaxShadowDistance);
			baseMaterial->Set("u_ShadowFade", s_Data.ShadowFade);
			baseMaterial->Set("u_CascadeFading", s_Data.CascadeFading);
			baseMaterial->Set("u_CascadeTransitionFade", s_Data.CascadeTransitionFade);
			baseMaterial->Set("u_IBLContribution", s_Data.SceneData.SceneEnvironmentIntensity);

			// Environment (TODO: don't do this per mesh)
			baseMaterial->Set("u_EnvRadianceTex", s_Data.SceneData.SceneEnvironment.RadianceMap);
			baseMaterial->Set("u_EnvIrradianceTex", s_Data.SceneData.SceneEnvironment.IrradianceMap);
			baseMaterial->Set("u_BRDFLUTTexture", s_Data.BRDFLUT);

			// Set lights (TODO: move to light environment and don't do per mesh)
			auto directionalLight = s_Data.SceneData.SceneLightEnvironment.DirectionalLights[0];
			baseMaterial->Set("u_DirectionalLights", directionalLight);

			auto rd = baseMaterial->FindResourceDeclaration("u_ShadowMapTexture");
			if (rd)
			{
				auto reg = rd->GetRegister();

				auto tex = s_Data.ShadowMapRenderPass[0]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();
				auto tex1 = s_Data.ShadowMapRenderPass[1]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();
				auto tex2 = s_Data.ShadowMapRenderPass[2]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();
				auto tex3 = s_Data.ShadowMapRenderPass[3]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();

				Renderer::Submit([reg, tex, tex1, tex2, tex3]() mutable
				{
					// 4 cascades
					glBindTextureUnit(reg, tex);
					glBindSampler(reg++, s_Data.ShadowMapSampler);

					glBindTextureUnit(reg, tex1);
					glBindSampler(reg++, s_Data.ShadowMapSampler);

					glBindTextureUnit(reg, tex2);
					glBindSampler(reg++, s_Data.ShadowMapSampler);

					glBindTextureUnit(reg, tex3);
					glBindSampler(reg++, s_Data.ShadowMapSampler);
				});
			}


			auto overrideMaterial = nullptr; // dc.Material;
			Renderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
		}

		if (outline || collider)
		{
			Renderer::Submit([]()
			{
				glStencilFunc(GL_ALWAYS, 1, 0xff);
				glStencilMask(0xff);
			});
		}

		for (auto& dc : s_Data.SelectedMeshDrawList)
		{
			auto baseMaterial = dc.Mesh->GetMaterial();
			baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
			baseMaterial->Set("u_ViewMatrix", sceneCamera.ViewMatrix);
			baseMaterial->Set("u_CameraPosition", cameraPosition);
			baseMaterial->Set("u_CascadeSplits", s_Data.CascadeSplits);
			baseMaterial->Set("u_ShowCascades", s_Data.ShowCascades);
			baseMaterial->Set("u_SoftShadows", s_Data.SoftShadows);
			baseMaterial->Set("u_LightSize", s_Data.LightSize);
			baseMaterial->Set("u_MaxShadowDistance", s_Data.MaxShadowDistance);
			baseMaterial->Set("u_ShadowFade", s_Data.ShadowFade);
			baseMaterial->Set("u_CascadeFading", s_Data.CascadeFading);
			baseMaterial->Set("u_CascadeTransitionFade", s_Data.CascadeTransitionFade);
			baseMaterial->Set("u_IBLContribution", s_Data.SceneData.SceneEnvironmentIntensity);

			// Environment (TODO: don't do this per mesh)
			baseMaterial->Set("u_EnvRadianceTex", s_Data.SceneData.SceneEnvironment.RadianceMap);
			baseMaterial->Set("u_EnvIrradianceTex", s_Data.SceneData.SceneEnvironment.IrradianceMap);
			baseMaterial->Set("u_BRDFLUTTexture", s_Data.BRDFLUT);

			baseMaterial->Set("u_LightMatrixCascade0", s_Data.LightMatrices[0]);
			baseMaterial->Set("u_LightMatrixCascade1", s_Data.LightMatrices[1]);
			baseMaterial->Set("u_LightMatrixCascade2", s_Data.LightMatrices[2]);
			baseMaterial->Set("u_LightMatrixCascade3", s_Data.LightMatrices[3]);

			// Set lights (TODO: move to light environment and don't do per mesh)
			baseMaterial->Set("u_DirectionalLights", s_Data.SceneData.SceneLightEnvironment.DirectionalLights[0]);

			auto rd = baseMaterial->FindResourceDeclaration("u_ShadowMapTexture");
			if (rd)
			{
				auto reg = rd->GetRegister();

				auto tex = s_Data.ShadowMapRenderPass[0]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();
				auto tex1 = s_Data.ShadowMapRenderPass[1]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();
				auto tex2 = s_Data.ShadowMapRenderPass[2]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();
				auto tex3 = s_Data.ShadowMapRenderPass[3]->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID();

				Renderer::Submit([reg, tex, tex1, tex2, tex3]() mutable
				{
					// 4 cascades
					glBindTextureUnit(reg, tex);
					glBindSampler(reg++, s_Data.ShadowMapSampler);

					glBindTextureUnit(reg, tex1);
					glBindSampler(reg++, s_Data.ShadowMapSampler);

					glBindTextureUnit(reg, tex2);
					glBindSampler(reg++, s_Data.ShadowMapSampler);

					glBindTextureUnit(reg, tex3);
					glBindSampler(reg++, s_Data.ShadowMapSampler);
				});
			}

			auto overrideMaterial = nullptr; // dc.Material;
			Renderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
		}

		if (outline)
		{
			Renderer::Submit([]()
			{
				glStencilFunc(GL_NOTEQUAL, 1, 0xff);
				glStencilMask(0);

				glLineWidth(10);
				glEnable(GL_LINE_SMOOTH);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_DEPTH_TEST);
			});

			// Draw outline here
			s_Data.OutlineMaterial->Set("u_ViewProjection", viewProjection);
			s_Data.OutlineAnimMaterial->Set("u_ViewProjection", viewProjection);
			for (auto& dc : s_Data.SelectedMeshDrawList)
			{
				Renderer::SubmitMesh(dc.Mesh, dc.Transform, dc.Mesh->IsAnimated() ? s_Data.OutlineAnimMaterial : s_Data.OutlineMaterial);
			}

			Renderer::Submit([]()
			{
				glPointSize(10);
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			});
			for (auto& dc : s_Data.SelectedMeshDrawList)
			{
				Renderer::SubmitMesh(dc.Mesh, dc.Transform, dc.Mesh->IsAnimated() ? s_Data.OutlineAnimMaterial : s_Data.OutlineMaterial);
			}

			Renderer::Submit([]()
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glStencilMask(0xff);
				glStencilFunc(GL_ALWAYS, 1, 0xff);
				glEnable(GL_DEPTH_TEST);
			});
		}

		if (collider)
		{
			Renderer::Submit([]()
			{
				glStencilFunc(GL_NOTEQUAL, 1, 0xff);
				glStencilMask(0);

				glLineWidth(1);
				glEnable(GL_LINE_SMOOTH);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_DEPTH_TEST);
			});

			s_Data.ColliderMaterial->Set("u_ViewProjection", viewProjection);
			for (auto& dc : s_Data.ColliderDrawList)
			{
				if (dc.Mesh)
					Renderer::SubmitMesh(dc.Mesh, dc.Transform, s_Data.ColliderMaterial);
			}

			Renderer::Submit([]()
			{
				glPointSize(1);
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			});

			for (auto& dc : s_Data.ColliderDrawList)
			{
				if (dc.Mesh)
					Renderer::SubmitMesh(dc.Mesh, dc.Transform, s_Data.ColliderMaterial);
			}

			Renderer::Submit([]()
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glStencilMask(0xff);
				glStencilFunc(GL_ALWAYS, 1, 0xff);
				glEnable(GL_DEPTH_TEST);
			});
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			s_Data.GridMaterial->Set("u_ViewProjection", viewProjection);
			Renderer::SubmitQuad(s_Data.GridMaterial, glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f)));
		}

		if (GetOptions().ShowBoundingBoxes)
		{
			Renderer2D::BeginScene(viewProjection);
			for (auto& dc : s_Data.DrawList)
				Renderer::DrawAABB(dc.Mesh, dc.Transform);
			Renderer2D::EndScene();
		}

		Renderer::EndRenderPass();
	}

	void SceneRenderer::CompositePass()
	{
		auto& compositeBuffer = s_Data.CompositePass->GetSpecification().TargetFramebuffer;

		Renderer::BeginRenderPass(s_Data.CompositePass);
		s_Data.CompositeShader->Bind();
		s_Data.CompositeShader->SetFloat("u_Exposure", s_Data.SceneData.SceneCamera.Camera.GetExposure());
		s_Data.CompositeShader->SetInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
		s_Data.CompositeShader->SetFloat2("u_ViewportSize", glm::vec2(compositeBuffer->GetWidth(), compositeBuffer->GetHeight()));
		s_Data.CompositeShader->SetFloat2("u_FocusPoint", s_Data.FocusPoint);
		s_Data.CompositeShader->SetInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
		s_Data.CompositeShader->SetFloat("u_BloomThreshold", s_Data.BloomThreshold);
		s_Data.GeoPass->GetSpecification().TargetFramebuffer->BindTexture();
		Renderer::Submit([]()
			{
				glBindTextureUnit(1, s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetDepthAttachmentRendererID());
			});
		Renderer::SubmitFullscreenQuad(nullptr);
		Renderer::EndRenderPass();
	}

	void SceneRenderer::BloomBlurPass()
	{
		int amount = 10;
		int index = 0;

		int horizontalCounter = 0, verticalCounter = 0;
		for (int i = 0; i < amount; i++)
		{
			index = i % 2;
			Renderer::BeginRenderPass(s_Data.BloomBlurPass[index]);
			s_Data.BloomBlurShader->Bind();
			s_Data.BloomBlurShader->SetBool("u_Horizontal", index);
			if (index)
				horizontalCounter++;
			else
				verticalCounter++;
			if (i > 0)
			{
				auto fb = s_Data.BloomBlurPass[1 - index]->GetSpecification().TargetFramebuffer;
				fb->BindTexture();
			}
			else
			{
				auto fb = s_Data.CompositePass->GetSpecification().TargetFramebuffer;
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
			Renderer::BeginRenderPass(s_Data.BloomBlendPass);
			s_Data.BloomBlendShader->Bind();
			s_Data.BloomBlendShader->SetFloat("u_Exposure", s_Data.SceneData.SceneCamera.Camera.GetExposure());
			s_Data.BloomBlendShader->SetBool("u_EnableBloom", s_Data.EnableBloom);

			s_Data.CompositePass->GetSpecification().TargetFramebuffer->BindTexture(0);
			s_Data.BloomBlurPass[index]->GetSpecification().TargetFramebuffer->BindTexture(1);

			Renderer::SubmitFullscreenQuad(nullptr);
			Renderer::EndRenderPass();
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

	static void CalculateCascades(CascadeData* cascades, const glm::vec3& lightDirection)
	{
		FrustumBounds frustumBounds[3];

		auto& sceneCamera = s_Data.SceneData.SceneCamera;
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
			float d = s_Data.CascadeSplitLambda * (log - uniform) + uniform;
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
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + s_Data.CascadeNearPlaneOffset, maxExtents.z - minExtents.z + s_Data.CascadeFarPlaneOffset);

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

	void SceneRenderer::ShadowMapPass()
	{
		auto& directionalLights = s_Data.SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
		{
			for (int i = 0; i < 4; i++)
			{
				// Clear shadow maps
				Renderer::BeginRenderPass(s_Data.ShadowMapRenderPass[i]);
				Renderer::EndRenderPass();
			}
			return;
		}

		CascadeData cascades[4];
		CalculateCascades(cascades, directionalLights[0].Direction);
		s_Data.LightViewMatrix = cascades[0].View;

		Renderer::Submit([]()
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
			});

		for (int i = 0; i < 4; i++)
		{
			s_Data.CascadeSplits[i] = cascades[i].SplitDepth;

			Renderer::BeginRenderPass(s_Data.ShadowMapRenderPass[i]);

			glm::mat4 shadowMapVP = cascades[i].ViewProj;

			static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
			s_Data.LightMatrices[i] = scaleBiasMatrix * cascades[i].ViewProj;


			// Render entities
			for (auto& dc : s_Data.ShadowPassDrawList)
			{
				Ref<Shader> shader = dc.Mesh->IsAnimated() ? s_Data.ShadowMapAnimShader : s_Data.ShadowMapShader;
				shader->SetMat4("u_ViewProjection", shadowMapVP);
				Renderer::SubmitMeshWithShader(dc.Mesh, dc.Transform, shader);
			}

			Renderer::EndRenderPass();
		}
	}

	void SceneRenderer::FlushDrawList()
	{
		HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

		memset(&s_Stats, 0, sizeof(SceneRendererStats));

		{
			Renderer::Submit([]()
			{
				s_Stats.ShadowPassTimer.Reset();
			});
			ShadowMapPass();
			Renderer::Submit([]
			{
				s_Stats.ShadowPass = s_Stats.ShadowPassTimer.ElapsedMillis();
			});
		}
		{
			Renderer::Submit([]()
			{
				s_Stats.GeometryPassTimer.Reset();
			});
			GeometryPass();
			Renderer::Submit([]
			{
				s_Stats.GeometryPass = s_Stats.GeometryPassTimer.ElapsedMillis();
			});
		}
		{
			Renderer::Submit([]()
			{
				s_Stats.CompositePassTimer.Reset();
			});

			CompositePass();
			Renderer::Submit([]
			{
				s_Stats.CompositePass = s_Stats.CompositePassTimer.ElapsedMillis();
			});

			//	BloomBlurPass();
		}

		s_Data.DrawList.clear();
		s_Data.SelectedMeshDrawList.clear();
		s_Data.ShadowPassDrawList.clear();
		s_Data.ColliderDrawList.clear();
		s_Data.SceneData = {};
	}

	Ref<Texture2D> SceneRenderer::GetFinalColorBuffer()
	{
		// return s_Data.CompositePass->GetSpecification().TargetFramebuffer;
		HZ_CORE_ASSERT(false, "Not implemented");
		return nullptr;
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return s_Data.CompositePass;
	}

	uint32_t SceneRenderer::GetFinalColorBufferRendererID()
	{
		return s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetColorAttachmentRendererID();
	}

	void SceneRenderer::SetFocusPoint(const glm::vec2& point)
	{
		s_Data.FocusPoint = point;
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return s_Data.Options;
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", s_Data.SoftShadows);
			UI::Property("Light Size", s_Data.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", s_Data.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", s_Data.ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Show Cascades", s_Data.ShowCascades);
				UI::Property("Cascade Fading", s_Data.CascadeFading);
				UI::Property("Cascade Transition Fade", s_Data.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", s_Data.CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", s_Data.CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", s_Data.CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = s_Data.ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFramebuffer;
				auto id = fb->GetDepthAttachmentRendererID();

				float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				ImGui::Image((ImTextureID)id, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Bloom"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Bloom", s_Data.EnableBloom);
			UI::Property("Bloom threshold", s_Data.BloomThreshold, 0.05f);
			UI::EndPropertyGrid();

			auto fb = s_Data.BloomBlurPass[0]->GetSpecification().TargetFramebuffer;
			auto id = fb->GetColorAttachmentRendererID();

			float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
			float w = size;
			float h = w / ((float)fb->GetWidth() / (float)fb->GetHeight());
			ImGui::Image((ImTextureID)id, { w, h }, { 0, 1 }, { 1, 0 });
			UI::EndTreeNode();
		}


		ImGui::End();
	}

}