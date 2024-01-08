#pragma once

#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Scene/Scene.h"

#include "RendererTypes.h"
#include "RendererCapabilities.h"

namespace Hazel {

	enum class RendererAPIType
	{
		None,
		Vulkan,
		OpenGL
	};

	enum class PrimitiveType
	{
		None = 0, Triangles, Lines
	};

	class RendererAPI
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(const Ref<RenderPass>& renderPass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material) = 0;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<Image2D> shadow) = 0;
		virtual std::pair<Ref<TextureCube>, Ref<TextureCube>> CreateEnvironmentMap(const std::string& filepath) = 0;
		virtual Ref<TextureCube> CreatePreethamSky(float turbidity, float azimuth, float inclination) = 0;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<Mesh> mesh, const glm::mat4& transform) = 0;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) = 0;

		virtual RendererCapabilities& GetCapabilities() = 0;

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPIType api);
	private:
		inline static RendererAPIType s_CurrentRendererAPI = RendererAPIType::Vulkan;
	};

}
