#include "hzpch.h"
#include "VulkanRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
#include "VulkanContext.h"

#include "Hazel/Renderer/Renderer.h"

#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"
#include "Hazel/Platform/Vulkan/VulkanMaterial.h"

#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"

#define IMGUI_IMPL_API
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "VulkanComputePipeline.h"

#include <glm/glm.hpp>

namespace Hazel {

	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;

		VkCommandBuffer ActiveCommandBuffer = nullptr;
		Ref<Texture2D> BRDFLut;
		VulkanShader::ShaderMaterialDescriptorSet RendererDescriptorSet;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VulkanShader::ShaderMaterialDescriptorSet QuadDescriptorSet;
	};

	static VulkanRendererData* s_Data = nullptr;

	namespace Utils {

		static const char* VulkanVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
				case 0x10DE: return "NVIDIA";
				case 0x1002: return "AMD";
				case 0x8086: return "INTEL";
				case 0x13B5: return "ARM";
			}
			return "Unknown";
		}

	}

	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();

		auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);

		Utils::DumpGPUInfo();

		// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.1f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.1f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.1f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.1f);
		data[3].TexCoord = glm::vec2(0, 1);

		s_Data->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		{
			TextureProperties props;
			props.SamplerWrap = TextureWrap::Clamp;
			s_Data->BRDFLut = Texture2D::Create("assets/textures/BRDF_LUT.tga", props);
		}

		Renderer::Submit([]() mutable
		{
			auto shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Static");
			Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();
			s_Data->RendererDescriptorSet = pbrShader->CreateDescriptorSets(1);
		});

	}

	void VulkanRenderer::Shutdown()
	{
		VulkanShader::ClearUniformBuffers();
		delete s_Data;
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

	void VulkanRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		Renderer::Submit([pipeline, mesh, transform]() mutable
		{
			auto vulkanMeshVB = mesh->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(mesh->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		});

		auto& submeshes = mesh->GetSubmeshes();
		for (Submesh& submesh : submeshes)
		{
			auto& material = mesh->GetMaterials()[submesh.MaterialIndex].As<VulkanMaterial>();
			material->UpdateForRendering();

			Renderer::Submit([pipeline, submesh, material, transform]() mutable
			{
				Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

				// Bind descriptor sets describing shader binding points
				std::array<VkDescriptorSet, 2> descriptorSets = {
					material->GetDescriptorSet().DescriptorSets[0],
					s_Data->RendererDescriptorSet.DescriptorSets[0]
				};
				vkCmdBindDescriptorSets(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

				glm::mat4 worldTransform = transform * submesh.Transform;

				Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();
				vkCmdPushConstants(s_Data->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
				vkCmdPushConstants(s_Data->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
				vkCmdDrawIndexed(s_Data->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
			});
		}
	}

	void VulkanRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform)
	{
		Renderer::Submit([pipeline, mesh, transform]() mutable
		{
			auto vulkanMeshVB = mesh->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(mesh->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);
		});
		
		auto& submeshes = mesh->GetSubmeshes();
		for (Submesh& submesh : submeshes)
		{
			Renderer::Submit([pipeline, submesh, transform]() mutable
			{
				Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				vkCmdBindPipeline(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				// Bind descriptor sets describing shader binding points
				VkDescriptorSet descriptorSet = {
					vulkanPipeline->GetDescriptorSet()
				};
				vkCmdBindDescriptorSets(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				glm::mat4 worldTransform = transform * submesh.Transform;
				vkCmdPushConstants(s_Data->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
				vkCmdDrawIndexed(s_Data->ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
			});
		}
	}

	void VulkanRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		vulkanMaterial->UpdateForRendering();

		Renderer::Submit([pipeline, vulkanMaterial, transform]() mutable
		{
			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			vkCmdBindDescriptorSets(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &vulkanMaterial->GetDescriptorSet().DescriptorSets[0], 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

			vkCmdPushConstants(s_Data->ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
			vkCmdPushConstants(s_Data->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(s_Data->ActiveCommandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		vulkanMaterial->UpdateForRendering();

		Renderer::Submit([pipeline, vulkanMaterial]() mutable
		{
			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_Data->QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_Data->QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			const auto& descriptorSets = vulkanMaterial->GetDescriptorSet().DescriptorSets;
			if (!descriptorSets.empty())
				vkCmdBindDescriptorSets(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[0], 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
			if (uniformStorageBuffer.Size)
				vkCmdPushConstants(s_Data->ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			vkCmdDrawIndexed(s_Data->ActiveCommandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow)
	{
		if (!environment)
			environment = Renderer::GetEmptyEnvironment();

		Renderer::Submit([environment, shadow]() mutable
		{
			auto shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Static");
			Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();

			std::array<VkWriteDescriptorSet, 4> writeDescriptors;

			Ref<VulkanTextureCube> radianceMap = environment->RadianceMap.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceMap = environment->IrradianceMap.As<VulkanTextureCube>();

			writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_EnvRadianceTex", 1);
			writeDescriptors[0].dstSet = s_Data->RendererDescriptorSet.DescriptorSets[0];
			const auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

			writeDescriptors[1] = *pbrShader->GetDescriptorSet("u_EnvIrradianceTex", 1);
			writeDescriptors[1].dstSet = s_Data->RendererDescriptorSet.DescriptorSets[0];
			const auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

			writeDescriptors[2] = *pbrShader->GetDescriptorSet("u_BRDFLUTTexture", 1);
			writeDescriptors[2].dstSet = s_Data->RendererDescriptorSet.DescriptorSets[0];
			const auto& brdfLutImageInfo = s_Data->BRDFLut.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
			writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

			writeDescriptors[3] = *pbrShader->GetDescriptorSet("u_ShadowMapTexture", 1);
			writeDescriptors[3].dstSet = s_Data->RendererDescriptorSet.DescriptorSets[0];
			const auto& shadowImageInfo = shadow.As<VulkanImage2D>()->GetDescriptor();
			writeDescriptors[3].pImageInfo = &shadowImageInfo;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		});
	}

	void VulkanRenderer::BeginFrame()
	{
		Renderer::Submit([]()
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
			s_Data->ActiveCommandBuffer = drawCommandBuffer;
			HZ_CORE_ASSERT(s_Data->ActiveCommandBuffer);
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));
		});
	}

	void VulkanRenderer::EndFrame()
	{
		Renderer::Submit([]()
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(s_Data->ActiveCommandBuffer));
			s_Data->ActiveCommandBuffer = nullptr;
		});
	}

	void VulkanRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass)
	{
		Renderer::Submit([renderPass]()
		{
			HZ_CORE_ASSERT(s_Data->ActiveCommandBuffer);

			auto fb = renderPass->GetSpecification().TargetFramebuffer;
			Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
			const auto& fbSpec = framebuffer->GetSpecification();

			uint32_t width = framebuffer->GetWidth();
			uint32_t height = framebuffer->GetHeight();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;

			// TODO: Does our framebuffer have a depth attachment?

			const auto& clearValues = framebuffer->GetVulkanClearValues();
			
			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

			vkCmdBeginRenderPass(s_Data->ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(s_Data->ActiveCommandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(s_Data->ActiveCommandBuffer, 0, 1, &scissor);
		});
	}

	void VulkanRenderer::EndRenderPass()
	{
		//HZ_CORE_ASSERT(s_Data->m_ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");

		Renderer::Submit([]()
		{
			vkCmdEndRenderPass(s_Data->ActiveCommandBuffer);
		});
	}

	std::pair<Ref<TextureCube>, Ref<TextureCube>> VulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		if (!Renderer::GetConfig().ComputeEnvironmentMaps)
			return { Renderer::GetBlackCubeTexture(), Renderer::GetBlackCubeTexture() };

		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
		HZ_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");

		Ref<TextureCube> envUnfiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
		Ref<TextureCube> envFiltered = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);
		
		// Convert equirectangular to cubemap
		Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		Ref<VulkanComputePipeline> equirectangularConversionPipeline = Ref<VulkanComputePipeline>::Create(equirectangularConversionShader);

		Renderer::Submit([equirectangularConversionPipeline, envEquirect, envUnfiltered, cubemapSize]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = equirectangularConversionPipeline->GetShader();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			auto descriptorSet = shader->CreateDescriptorSets();
			Ref<VulkanTextureCube> envUnfilteredCubemap = envUnfiltered.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			Ref<VulkanTexture2D> envEquirectVK = envEquirect.As<VulkanTexture2D>();
			writeDescriptors[1] = *shader->GetDescriptorSet("u_EquirectangularTex");
			writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
			equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

			envUnfilteredCubemap->GenerateMips(true);
		});

		Ref<Shader> environmentMipFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
		Ref<VulkanComputePipeline> environmentMipFilterPipeline = Ref<VulkanComputePipeline>::Create(environmentMipFilterShader);

		Renderer::Submit([environmentMipFilterPipeline, envUnfiltered, envFiltered, cubemapSize]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentMipFilterPipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = envFiltered.As<VulkanTextureCube>();
			VkDescriptorImageInfo imageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();

			uint32_t mipCount = Utils::CalculateMipCount(cubemapSize, cubemapSize);

			std::vector<VkWriteDescriptorSet> writeDescriptors(mipCount * 2);
			std::vector<VkDescriptorImageInfo> mipImageInfos(mipCount);
			auto descriptorSet = shader->CreateDescriptorSets(0, 12);
			for (uint32_t i = 0; i < mipCount; i++)
			{
				VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
				mipImageInfo = imageInfo;
				mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);

				writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
				writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
				writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

				Ref<VulkanTextureCube> envUnfilteredCubemap = envUnfiltered.As<VulkanTextureCube>();
				writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
				writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i]; // Should this be set inside the shader?
				writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
			}

			vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

			environmentMipFilterPipeline->Begin(); // begin compute pass
			const float deltaRoughness = 1.0f / glm::max((float)envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
			for (uint32_t i = 0, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				uint32_t numGroups = glm::max(1u, size / 32);
				float roughness = i * deltaRoughness;
				roughness = glm::max(roughness, 0.05f);
				environmentMipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
				environmentMipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
			}
			environmentMipFilterPipeline->End();
		});

		Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
		Ref<VulkanComputePipeline> environmentIrradiancePipeline = Ref<VulkanComputePipeline>::Create(environmentIrradianceShader);
		Ref<TextureCube> irradianceMap = TextureCube::Create(ImageFormat::RGBA32F, irradianceMapSize, irradianceMapSize);

		Renderer::Submit([environmentIrradiancePipeline, irradianceMap, envFiltered]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentIrradiancePipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = envFiltered.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceCubemap = irradianceMap.As<VulkanTextureCube>();
			auto descriptorSet = shader->CreateDescriptorSets();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			writeDescriptors[0] = *shader->GetDescriptorSet("o_IrradianceMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0];
			writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

			writeDescriptors[1] = *shader->GetDescriptorSet("u_RadianceMap");
			writeDescriptors[1].dstSet = descriptorSet.DescriptorSets[0];
			writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);
			environmentIrradiancePipeline->Begin();
			environmentIrradiancePipeline->SetPushConstants(&Renderer::GetConfig().IrradianceMapComputeSamples, sizeof(uint32_t));
			environmentIrradiancePipeline->Dispatch(descriptorSet.DescriptorSets[0], irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
			environmentIrradiancePipeline->End();

			irradianceCubemap->GenerateMips();
		});

		return { envFiltered, irradianceMap };
	}

	Ref<TextureCube> VulkanRenderer::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		Ref<TextureCube> environmentMap = TextureCube::Create(ImageFormat::RGBA32F, cubemapSize, cubemapSize);

		Ref<Shader> preethamSkyShader = Renderer::GetShaderLibrary()->Get("PreethamSky");
		Ref<VulkanComputePipeline> preethamSkyComputePipeline = Ref<VulkanComputePipeline>::Create(preethamSkyShader);

		glm::vec3 params = { turbidity, azimuth, inclination };
		Renderer::Submit([preethamSkyComputePipeline, environmentMap, cubemapSize, params]() mutable
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = preethamSkyComputePipeline->GetShader();

			std::array<VkWriteDescriptorSet, 1> writeDescriptors;
			auto descriptorSet = shader->CreateDescriptorSets();
			Ref<VulkanTextureCube> envUnfilteredCubemap = environmentMap.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = descriptorSet.DescriptorSets[0]; // Should this be set inside the shader?
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, writeDescriptors.size(), writeDescriptors.data(), 0, NULL);

			preethamSkyComputePipeline->Begin();
			preethamSkyComputePipeline->SetPushConstants(&params, sizeof(glm::vec3));
			preethamSkyComputePipeline->Dispatch(descriptorSet.DescriptorSets[0], cubemapSize / 32, cubemapSize / 32, 6);
			preethamSkyComputePipeline->End();

			envUnfilteredCubemap->GenerateMips(true);
		});

		return environmentMap;
	}

}