#include "Hazel.h"

#include "Hazel/ImGui/ImGuiLayer.h"
#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

static void ImGuiShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

class EditorLayer : public Hazel::Layer
{
public:
	EditorLayer()
		: m_Scene(Scene::Model), m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f))
	{
	}

	virtual ~EditorLayer()
	{
	}

	virtual void OnAttach() override
	{
		using namespace glm;

		m_Mesh.reset(new Hazel::Mesh("assets/models/m1911/m1911.fbx"));
		m_MeshMaterial.reset(new Hazel::MaterialInstance(m_Mesh->GetMaterial()));

		m_QuadShader = Hazel::Shader::Create("assets/shaders/quad.glsl");
		m_HDRShader = Hazel::Shader::Create("assets/shaders/hdr.glsl");

		m_SphereMesh.reset(new Hazel::Mesh("assets/models/Sphere1m.fbx"));
		m_PlaneMesh.reset(new Hazel::Mesh("assets/models/Plane1m.obj"));

		m_GridShader = Hazel::Shader::Create("assets/shaders/Grid.glsl");
		m_GridMaterial = Hazel::MaterialInstance::Create(Hazel::Material::Create(m_GridShader));
		m_GridMaterial->Set("u_Scale", m_GridScale);
		m_GridMaterial->Set("u_Res", m_GridSize);

		// Editor
		m_CheckerboardTex.reset(Hazel::Texture2D::Create("assets/editor/Checkerboard.tga"));

		// Environment
		m_EnvironmentCubeMap.reset(Hazel::TextureCube::Create("assets/textures/environments/Arches_E_PineTree_Radiance.tga"));
		//m_EnvironmentCubeMap.reset(Hazel::TextureCube::Create("assets/textures/environments/DebugCubeMap.tga"));
		m_EnvironmentIrradiance.reset(Hazel::TextureCube::Create("assets/textures/environments/Arches_E_PineTree_Irradiance.tga"));
		m_BRDFLUT.reset(Hazel::Texture2D::Create("assets/textures/BRDF_LUT.tga"));

		m_Framebuffer.reset(Hazel::Framebuffer::Create(1280, 720, Hazel::FramebufferFormat::RGBA16F));
		m_FinalPresentBuffer.reset(Hazel::Framebuffer::Create(1280, 720, Hazel::FramebufferFormat::RGBA8));

		float x = -4.0f;
		float roughness = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			Hazel::Ref<Hazel::MaterialInstance> mi(new Hazel::MaterialInstance(m_SphereMesh->GetMaterial()));
			mi->Set("u_Metalness", 1.0f);
			mi->Set("u_Roughness", roughness);
			mi->Set("u_ModelMatrix", translate(mat4(1.0f), vec3(x, 0.0f, 0.0f)));
			x += 1.1f;
			roughness += 0.15f;
			m_MetalSphereMaterialInstances.push_back(mi);
		}

		x = -4.0f;
		roughness = 0.0f;
		for (int i = 0; i < 8; i++)
		{
			Hazel::Ref<Hazel::MaterialInstance> mi(new Hazel::MaterialInstance(m_SphereMesh->GetMaterial()));
			mi->Set("u_Metalness", 0.0f);
			mi->Set("u_Roughness", roughness);
			mi->Set("u_ModelMatrix", translate(mat4(1.0f), vec3(x, 1.2f, 0.0f)));
			x += 1.1f;
			roughness += 0.15f;
			m_DielectricSphereMaterialInstances.push_back(mi);
		}

		// Create Quad
		x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0);
		data[3].TexCoord = glm::vec2(0, 1);

		m_VertexBuffer.reset(Hazel::VertexBuffer::Create());
		m_VertexBuffer->SetData(data, 4 * sizeof(QuadVertex));

		uint32_t* indices = new uint32_t[6] { 0, 1, 2, 2, 3, 0, };
		m_IndexBuffer.reset(Hazel::IndexBuffer::Create());
		m_IndexBuffer->SetData(indices, 6 * sizeof(uint32_t));

		m_Light.Direction = { -0.5f, -0.5f, 1.0f };
		m_Light.Radiance = { 1.0f, 1.0f, 1.0f };
	}

	virtual void OnDetach() override
	{
	}

	virtual void OnUpdate(Hazel::TimeStep ts) override
	{
		// THINGS TO LOOK AT:
		// - BRDF LUT
		// - Cubemap mips and filtering
		// - Tonemapping and proper HDR pipeline
		using namespace Hazel;
		using namespace glm;

		m_Camera.Update(ts);
		auto viewProjection = m_Camera.GetProjectionMatrix() * m_Camera.GetViewMatrix();

		m_Framebuffer->Bind();
		Renderer::Clear();
		// TODO:
		// Renderer::BeginScene(m_Camera);
		// Renderer::EndScene();

		m_QuadShader->Bind();
		m_QuadShader->SetMat4("u_InverseVP", inverse(viewProjection));
		m_EnvironmentIrradiance->Bind(0);
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
		Renderer::DrawIndexed(m_IndexBuffer->GetCount(), false);

		m_MeshMaterial->Set("u_AlbedoColor", m_AlbedoInput.Color);
		m_MeshMaterial->Set("u_Metalness", m_MetalnessInput.Value);
		m_MeshMaterial->Set("u_Roughness", m_RoughnessInput.Value);
		m_MeshMaterial->Set("u_ViewProjectionMatrix", viewProjection);
		m_MeshMaterial->Set("u_ModelMatrix", scale(mat4(1.0f), vec3(m_MeshScale)));
		m_MeshMaterial->Set("lights", m_Light);
		m_MeshMaterial->Set("u_CameraPosition", m_Camera.GetPosition());
		m_MeshMaterial->Set("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
		m_MeshMaterial->Set("u_AlbedoTexToggle", m_AlbedoInput.UseTexture ? 1.0f : 0.0f);
		m_MeshMaterial->Set("u_NormalTexToggle", m_NormalInput.UseTexture ? 1.0f : 0.0f);
		m_MeshMaterial->Set("u_MetalnessTexToggle", m_MetalnessInput.UseTexture ? 1.0f : 0.0f);
		m_MeshMaterial->Set("u_RoughnessTexToggle", m_RoughnessInput.UseTexture ? 1.0f : 0.0f);
		m_MeshMaterial->Set("u_EnvMapRotation", m_EnvMapRotation);

#if 0
		// Bind default texture unit
		UploadUniformInt("u_Texture", 0);

		// PBR shader textures
		UploadUniformInt("u_AlbedoTexture", 1);
		UploadUniformInt("u_NormalTexture", 2);
		UploadUniformInt("u_MetalnessTexture", 3);
		UploadUniformInt("u_RoughnessTexture", 4);

		UploadUniformInt("u_EnvRadianceTex", 10);
		UploadUniformInt("u_EnvIrradianceTex", 11);

		UploadUniformInt("u_BRDFLUTTexture", 15);
#endif
		m_MeshMaterial->Set("u_EnvRadianceTex", m_EnvironmentCubeMap);
		m_MeshMaterial->Set("u_EnvIrradianceTex", m_EnvironmentIrradiance);
		m_MeshMaterial->Set("u_BRDFLUTTexture", m_BRDFLUT);

		m_SphereMesh->GetMaterial()->Set("u_AlbedoColor", m_AlbedoInput.Color);
		m_SphereMesh->GetMaterial()->Set("u_Metalness", m_MetalnessInput.Value);
		m_SphereMesh->GetMaterial()->Set("u_Roughness", m_RoughnessInput.Value);
		m_SphereMesh->GetMaterial()->Set("u_ViewProjectionMatrix", viewProjection);
		m_SphereMesh->GetMaterial()->Set("u_ModelMatrix", scale(mat4(1.0f), vec3(m_MeshScale)));
		m_SphereMesh->GetMaterial()->Set("lights", m_Light);
		m_SphereMesh->GetMaterial()->Set("u_CameraPosition", m_Camera.GetPosition());
		m_SphereMesh->GetMaterial()->Set("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
		m_SphereMesh->GetMaterial()->Set("u_AlbedoTexToggle", m_AlbedoInput.UseTexture ? 1.0f : 0.0f);
		m_SphereMesh->GetMaterial()->Set("u_NormalTexToggle", m_NormalInput.UseTexture ? 1.0f : 0.0f);
		m_SphereMesh->GetMaterial()->Set("u_MetalnessTexToggle", m_MetalnessInput.UseTexture ? 1.0f : 0.0f);
		m_SphereMesh->GetMaterial()->Set("u_RoughnessTexToggle", m_RoughnessInput.UseTexture ? 1.0f : 0.0f);
		m_SphereMesh->GetMaterial()->Set("u_EnvMapRotation", m_EnvMapRotation);
		m_SphereMesh->GetMaterial()->Set("u_EnvRadianceTex", m_EnvironmentCubeMap);
		m_SphereMesh->GetMaterial()->Set("u_EnvIrradianceTex", m_EnvironmentIrradiance);
		m_SphereMesh->GetMaterial()->Set("u_BRDFLUTTexture", m_BRDFLUT);

		if (m_AlbedoInput.TextureMap)
			m_MeshMaterial->Set("u_AlbedoTexture", m_AlbedoInput.TextureMap);
		if (m_NormalInput.TextureMap)
			m_MeshMaterial->Set("u_NormalTexture", m_NormalInput.TextureMap);
		if (m_MetalnessInput.TextureMap)
			m_MeshMaterial->Set("u_MetalnessTexture", m_MetalnessInput.TextureMap);
		if (m_RoughnessInput.TextureMap)
			m_MeshMaterial->Set("u_RoughnessTexture", m_RoughnessInput.TextureMap);

		if (m_Scene == Scene::Spheres)
		{
			// Metals
			for (int i = 0; i < 8; i++)
				m_SphereMesh->Render(ts, glm::mat4(1.0f), m_MetalSphereMaterialInstances[i]);

			// Dielectrics
			for (int i = 0; i < 8; i++)
				m_SphereMesh->Render(ts, glm::mat4(1.0f), m_DielectricSphereMaterialInstances[i]);
		}
		else if (m_Scene == Scene::Model)
		{
			if (m_Mesh) 
				m_Mesh->Render(ts, scale(mat4(1.0f), vec3(m_MeshScale)), m_MeshMaterial);
		}

		m_GridMaterial->Set("u_MVP", viewProjection * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f)));
		m_PlaneMesh->Render(ts, m_GridMaterial);

		m_Framebuffer->Unbind();

		m_FinalPresentBuffer->Bind();
		m_HDRShader->Bind();
		m_HDRShader->SetFloat("u_Exposure", m_Exposure);
		m_Framebuffer->BindTexture();
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
		Renderer::DrawIndexed(m_IndexBuffer->GetCount(), false);
		m_FinalPresentBuffer->Unbind();
	}

	enum class PropertyFlag
	{
		None = 0, ColorProperty = 1
	};

	void Property(const std::string& name, bool& value)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::SliderFloat(id.c_str(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void Property(const std::string& name, glm::vec3& value, PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void Property(const std::string& name, glm::vec4& value, PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	virtual void OnImGuiRender() override
	{
		static bool p_open = true;

		static bool opt_fullscreen_persistant = true;
		static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
		bool opt_fullscreen = opt_fullscreen_persistant;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Dockspace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
		}

		// Editor Panel ------------------------------------------------------------------------------
		ImGui::Begin("Model");
		ImGui::RadioButton("Spheres", (int*)&m_Scene, (int)Scene::Spheres);
		ImGui::SameLine();
		ImGui::RadioButton("Model", (int*)&m_Scene, (int)Scene::Model);

		ImGui::Begin("Environment");

		ImGui::Columns(2);
		ImGui::AlignTextToFramePadding();
		
		Property("Light Direction", m_Light.Direction);
		Property("Light Radiance", m_Light.Radiance, PropertyFlag::ColorProperty);
		Property("Light Multiplier", m_LightMultiplier, 0.0f, 5.0f);
		Property("Exposure", m_Exposure, 0.0f, 5.0f);

		Property("Mesh Scale", m_MeshScale, 0.0f, 2.0f);

		Property("Radiance Prefiltering", m_RadiancePrefilter);
		Property("Env Map Rotation", m_EnvMapRotation, -360.0f, 360.0f);

		ImGui::Columns(1);

		ImGui::End();

		ImGui::Separator();
		{
			ImGui::Text("Mesh");
			std::string fullpath = m_Mesh ? m_Mesh->GetFilePath() : "None";
			size_t found = fullpath.find_last_of("/\\");
			std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
			ImGui::Text(path.c_str()); ImGui::SameLine();
			if (ImGui::Button("...##Mesh"))
			{
				std::string filename = Hazel::Application::Get().OpenFile("");
				if (filename != "")
				{
					m_Mesh.reset(new Hazel::Mesh(filename));
					m_MeshMaterial.reset(new Hazel::MaterialInstance(m_Mesh->GetMaterial()));
				}
			}
		}
		ImGui::Separator();

		// Textures ------------------------------------------------------------------------------
		{
			// Albedo
			if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_AlbedoInput.TextureMap ? (void*)m_AlbedoInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_AlbedoInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_AlbedoInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_AlbedoInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_AlbedoInput.TextureMap.reset(Hazel::Texture2D::Create(filename, m_AlbedoInput.SRGB));
					}
				}
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Checkbox("Use##AlbedoMap", &m_AlbedoInput.UseTexture);
				if (ImGui::Checkbox("sRGB##AlbedoMap", &m_AlbedoInput.SRGB))
				{
					if (m_AlbedoInput.TextureMap)
						m_AlbedoInput.TextureMap.reset(Hazel::Texture2D::Create(m_AlbedoInput.TextureMap->GetPath(), m_AlbedoInput.SRGB));
				}
				ImGui::EndGroup();
				ImGui::SameLine();
				ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(m_AlbedoInput.Color), ImGuiColorEditFlags_NoInputs);
			}
		}
		{
			// Normals
			if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_NormalInput.TextureMap ? (void*)m_NormalInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_NormalInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_NormalInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_NormalInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_NormalInput.TextureMap.reset(Hazel::Texture2D::Create(filename));
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##NormalMap", &m_NormalInput.UseTexture);
			}
		}
		{
			// Metalness
			if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_MetalnessInput.TextureMap ? (void*)m_MetalnessInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_MetalnessInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_MetalnessInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_MetalnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_MetalnessInput.TextureMap.reset(Hazel::Texture2D::Create(filename));
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##MetalnessMap", &m_MetalnessInput.UseTexture);
				ImGui::SameLine();
				ImGui::SliderFloat("Value##MetalnessInput", &m_MetalnessInput.Value, 0.0f, 1.0f);
			}
		}
		{
			// Roughness
			if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_RoughnessInput.TextureMap ? (void*)m_RoughnessInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_RoughnessInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_RoughnessInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_RoughnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_RoughnessInput.TextureMap.reset(Hazel::Texture2D::Create(filename));
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##RoughnessMap", &m_RoughnessInput.UseTexture);
				ImGui::SameLine();
				ImGui::SliderFloat("Value##RoughnessInput", &m_RoughnessInput.Value, 0.0f, 1.0f);
			}
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Hazel::Shader::s_AllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload();
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}


		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");

		/*float posX = ImGui::GetCursorScreenPos().x;
		float posY = ImGui::GetCursorScreenPos().y;

		auto [wx, wy] = Hazel::Application::Get().GetWindow().GetWindowPos();
		posX -= wx;
		posY -= wy;
		HZ_INFO("{0}, {1}", posX, posY);*/

		auto viewportSize = ImGui::GetContentRegionAvail();
		m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		m_FinalPresentBuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
		m_Camera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		ImGui::Image((void*)m_FinalPresentBuffer->GetColorAttachmentRendererID(), viewportSize, { 0, 1 }, { 1, 0 });
		ImGui::End();
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Docking"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows, 
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

				if (ImGui::MenuItem("Flag: NoSplit", "", (opt_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 opt_flags ^= ImGuiDockNodeFlags_NoSplit;
				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (opt_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  opt_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
				if (ImGui::MenuItem("Flag: NoResize", "", (opt_flags & ImGuiDockNodeFlags_NoResize) != 0))                opt_flags ^= ImGuiDockNodeFlags_NoResize;
				if (ImGui::MenuItem("Flag: PassthruDockspace", "", (opt_flags & ImGuiDockNodeFlags_PassthruDockspace) != 0))       opt_flags ^= ImGuiDockNodeFlags_PassthruDockspace;
				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (opt_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          opt_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
				ImGui::Separator();
				if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
					p_open = false;
				ImGui::EndMenu();
			}
			ImGuiShowHelpMarker(
				"You can _always_ dock _any_ window into another by holding the SHIFT key while moving a window. Try it now!" "\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)"
			);

			ImGui::EndMenuBar();
		}

		ImGui::End();

		if (m_Mesh)
			m_Mesh->OnImGuiRender();

		// static bool o = true;
		// ImGui::ShowDemoWindow(&o);
	}

	virtual void OnEvent(Hazel::Event& event) override
	{
	}
private:
	Hazel::Ref<Hazel::Shader> m_QuadShader;
	Hazel::Ref<Hazel::Shader> m_HDRShader;
	Hazel::Ref<Hazel::Shader> m_GridShader;
	Hazel::Ref<Hazel::Mesh> m_Mesh;
	Hazel::Ref<Hazel::Mesh> m_SphereMesh, m_PlaneMesh;
	Hazel::Ref<Hazel::Texture2D> m_BRDFLUT;

	Hazel::Ref<Hazel::MaterialInstance> m_MeshMaterial;
	Hazel::Ref<Hazel::MaterialInstance> m_GridMaterial;
	std::vector<Hazel::Ref<Hazel::MaterialInstance>> m_MetalSphereMaterialInstances;
	std::vector<Hazel::Ref<Hazel::MaterialInstance>> m_DielectricSphereMaterialInstances;

	float m_GridScale = 16.025f, m_GridSize = 0.025f;
	float m_MeshScale = 1.0f;

	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		Hazel::Ref<Hazel::Texture2D> TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		Hazel::Ref<Hazel::Texture2D> TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		Hazel::Ref<Hazel::Texture2D> TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.5f;
		Hazel::Ref<Hazel::Texture2D> TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

	std::unique_ptr<Hazel::Framebuffer> m_Framebuffer, m_FinalPresentBuffer;

	Hazel::Ref<Hazel::VertexBuffer> m_VertexBuffer;
	Hazel::Ref<Hazel::IndexBuffer> m_IndexBuffer;
	Hazel::Ref<Hazel::TextureCube> m_EnvironmentCubeMap, m_EnvironmentIrradiance;

	Hazel::Camera m_Camera;

	struct Light
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;
	};
	Light m_Light;
	float m_LightMultiplier = 0.3f;

	// PBR params
	float m_Exposure = 1.0f;

	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	enum class Scene : uint32_t
	{
		Spheres = 0, Model = 1
	};
	Scene m_Scene;

	// Editor resources
	Hazel::Ref<Hazel::Texture2D> m_CheckerboardTex;
};

class Sandbox : public Hazel::Application
{
public:
	Sandbox()
	{
		HZ_TRACE("Hello!");
	}

	virtual void OnInit() override
	{
		PushLayer(new EditorLayer());
	}
};

Hazel::Application* Hazel::CreateApplication()
{
	return new Sandbox();
}