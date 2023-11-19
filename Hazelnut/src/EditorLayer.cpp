#include "EditorLayer.h"

#include "Hazel/ImGui/ImGuizmo.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Script/ScriptEngine.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Hazel {

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

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}

	EditorLayer::EditorLayer()
		: m_SceneType(SceneType::Model), m_EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f))
	{
	}

	EditorLayer::~EditorLayer()
	{
	}

	void EditorLayer::OnAttach()
	{
		using namespace glm;

		// Editor
		m_CheckerboardTex = Texture2D::Create("assets/editor/Checkerboard.tga");
		m_PlayButtonTex = Texture2D::Create("assets/editor/PlayButton.png");

		m_EditorScene = Ref<Scene>::Create();
		UpdateWindowTitle("Untitled Scene");
		ScriptEngine::SetSceneContext(m_EditorScene);
		m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(m_EditorScene);
		m_SceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EditorLayer::SelectEntity, this, std::placeholders::_1));
		m_SceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EditorLayer::OnEntityDeleted, this, std::placeholders::_1));
		
		SceneSerializer serializer(m_EditorScene);
		serializer.Deserialize("assets/scenes/levels/Physics2D-Game.hsc");
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnScenePlay()
	{
		m_SelectionContext.clear();

		m_SceneState = SceneState::Play;

		if (m_ReloadScriptOnPlay)
			ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");

		m_RuntimeScene = Ref<Scene>::Create();
		m_EditorScene->CopyTo(m_RuntimeScene);

		m_RuntimeScene->OnRuntimeStart();
		m_SceneHierarchyPanel->SetContext(m_RuntimeScene);
	}

	void EditorLayer::OnSceneStop()
	{
		m_RuntimeScene->OnRuntimeStop();
		m_SceneState = SceneState::Edit;

		// Unload runtime scene
		m_RuntimeScene = nullptr;

		m_SelectionContext.clear();
		ScriptEngine::SetSceneContext(m_EditorScene);
		m_SceneHierarchyPanel->SetContext(m_EditorScene);
	}

	void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string title = sceneName + " - Hazelnut - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
		Application::Get().GetWindow().SetTitle(title);
	}

	float EditorLayer::GetSnapValue()
	{
		switch (m_GizmoType)
		{
			case  ImGuizmo::OPERATION::TRANSLATE: return 0.5f;
			case  ImGuizmo::OPERATION::ROTATE: return 45.0f;
			case  ImGuizmo::OPERATION::SCALE: return 0.5f;
		}
		return 0.0f;
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				//if (m_ViewportPanelFocused)
					m_EditorCamera.OnUpdate(ts);

				m_EditorScene->OnRenderEditor(ts, m_EditorCamera);

				if (m_DrawOnTopBoundingBoxes)
				{
					Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
					auto viewProj = m_EditorCamera.GetViewProjection();
					Renderer2D::BeginScene(viewProj, false);
					// TODO: Renderer::DrawAABB(m_MeshEntity.GetComponent<MeshComponent>(), m_MeshEntity.GetComponent<TransformComponent>());
					Renderer2D::EndScene();
					Renderer::EndRenderPass();
				}

				if (m_SelectionContext.size() && false)
				{
					auto& selection = m_SelectionContext[0];

					if (selection.Mesh && selection.Entity.HasComponent<MeshComponent>())
					{
						Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
						auto viewProj = m_EditorCamera.GetViewProjection();
						Renderer2D::BeginScene(viewProj, false);
						glm::vec4 color = (m_SelectionMode == SelectionMode::Entity) ? glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f } : glm::vec4{ 0.2f, 0.9f, 0.2f, 1.0f };
						Renderer::DrawAABB(selection.Mesh->BoundingBox, selection.Entity.GetComponent<TransformComponent>().Transform * selection.Mesh->Transform, color);
						Renderer2D::EndScene();
						Renderer::EndRenderPass();
					}
				}

				if (m_SelectionContext.size())
				{
					auto& selection = m_SelectionContext[0];

					if (selection.Entity.HasComponent<BoxCollider2DComponent>())
					{
						const auto& size = selection.Entity.GetComponent<BoxCollider2DComponent>().Size;
						auto [translation, rotationQuat, scale] = GetTransformDecomposition(selection.Entity.GetComponent<TransformComponent>().Transform);
						glm::vec3 rotation = glm::eulerAngles(rotationQuat);

						Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
						auto viewProj = m_EditorCamera.GetViewProjection();
						Renderer2D::BeginScene(viewProj, false);
						Renderer2D::DrawRotatedQuad({ translation.x, translation.y }, size * 2.0f, glm::degrees(rotation.z), { 1.0f, 0.0f, 1.0f, 1.0f });
						Renderer2D::EndScene();
						Renderer::EndRenderPass();
					}
				}

				break;
			}
			case SceneState::Play:
			{
				if (m_ViewportPanelFocused)
					m_EditorCamera.OnUpdate(ts);

				m_RuntimeScene->OnUpdate(ts);
				m_RuntimeScene->OnRenderRuntime(ts);
				break;
			}
			case SceneState::Pause:
			{
				if (m_ViewportPanelFocused)
					m_EditorCamera.OnUpdate(ts);

				m_RuntimeScene->OnRenderRuntime(ts);
				break;
			}
		}
	}

	bool EditorLayer::Property(const std::string& name, bool& value)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		bool result = ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		
		return result;
	}

	bool EditorLayer::Property(const std::string& name, float& value, float min, float max, PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		bool changed = false;
		if (flags == PropertyFlag::SliderProperty)
			changed = ImGui::SliderFloat(id.c_str(), &value, min, max);
		else
			changed = ImGui::DragFloat(id.c_str(), &value, 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
		
		return changed;
	}

	bool EditorLayer::Property(const std::string& name, glm::vec2& value, EditorLayer::PropertyFlag flags)
	{
		return Property(name, value, -1.0f, 1.0f, flags);
	}

	bool EditorLayer::Property(const std::string& name, glm::vec2& value, float min, float max, PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		bool changed = false;
		if (flags == PropertyFlag::SliderProperty)
			changed = ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);
		else
			changed = ImGui::DragFloat2(id.c_str(), glm::value_ptr(value), 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

	bool EditorLayer::Property(const std::string& name, glm::vec3& value, EditorLayer::PropertyFlag flags)
	{
		return Property(name, value, -1.0f, 1.0f, flags);
	}

	bool EditorLayer::Property(const std::string& name, glm::vec3& value, float min, float max, EditorLayer::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		bool changed = false;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			changed = ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else if (flags == PropertyFlag::SliderProperty)
			changed = ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);
		else
			changed = ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

	bool EditorLayer::Property(const std::string& name, glm::vec4& value, EditorLayer::PropertyFlag flags)
	{
		return Property(name, value, -1.0f, 1.0f, flags);
	}

	bool EditorLayer::Property(const std::string& name, glm::vec4& value, float min, float max, EditorLayer::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		bool changed = false;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			changed = ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else if (flags == PropertyFlag::SliderProperty)
			changed = ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);
		else
			changed = ImGui::DragFloat4(id.c_str(), glm::value_ptr(value), 1.0f, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}

	void EditorLayer::ShowBoundingBoxes(bool show, bool onTop)
	{
		SceneRenderer::GetOptions().ShowBoundingBoxes = show && !onTop;
		m_DrawOnTopBoundingBoxes = show && onTop;
	}

	void EditorLayer::SelectEntity(Entity entity)
	{
		SelectedSubmesh selection;
		if (entity.HasComponent<MeshComponent>())
		{
			selection.Mesh = &entity.GetComponent<MeshComponent>().Mesh->GetSubmeshes()[0];
		}
		selection.Entity = entity;
		m_SelectionContext.clear();
		m_SelectionContext.push_back(selection);

		m_EditorScene->SetSelectedEntity(entity);
	}

	void EditorLayer::OpenScene()
	{
		auto& app = Application::Get();
		std::string filepath = app.OpenFile("Hazel Scene (*.hsc)\0*.hsc\0");
		if (!filepath.empty())
		{
			Ref<Scene> newScene = Ref<Scene>::Create();
			SceneSerializer serializer(newScene);
			serializer.Deserialize(filepath);
			m_EditorScene = newScene;
			std::filesystem::path path = filepath;
			UpdateWindowTitle(path.filename().string());
			m_SceneHierarchyPanel->SetContext(m_EditorScene);
			ScriptEngine::SetSceneContext(m_EditorScene);

			m_EditorScene->SetSelectedEntity({});
			m_SelectionContext.clear();

			m_SceneFilePath = filepath;
		}
	}

	void EditorLayer::SaveScene()
	{
		SceneSerializer serializer(m_EditorScene);
		serializer.Serialize(m_SceneFilePath);
	}

	void EditorLayer::SaveSceneAs()
	{
		auto& app = Application::Get();
		std::string filepath = app.SaveFile("Hazel Scene (*.hsc)\0*.hsc\0");
		if (!filepath.empty())
		{
			SceneSerializer serializer(m_EditorScene);
			serializer.Serialize(filepath);

			std::filesystem::path path = filepath;
			UpdateWindowTitle(path.filename().string());
			m_SceneFilePath = filepath;
		}
	}

	void EditorLayer::OnImGuiRender()
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
		//if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
		//	window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Dockspace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		// Editor Panel ------------------------------------------------------------------------------
		ImGui::Begin("Model");
		ImGui::Begin("Environment");

		if (ImGui::Button("Load Environment Map"))
		{
			std::string filename = Application::Get().OpenFile("*.hdr");
			if (filename != "")
				m_EditorScene->SetEnvironment(Environment::Load(filename));
		}

		ImGui::SliderFloat("Skybox LOD", &m_EditorScene->GetSkyboxLod(), 0.0f, 11.0f);

		ImGui::Columns(2);
		ImGui::AlignTextToFramePadding();

		auto& light = m_EditorScene->GetLight();
		Property("Light Direction", light.Direction, PropertyFlag::SliderProperty);
		Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
		Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f, PropertyFlag::SliderProperty);

		Property("Exposure", m_EditorCamera.GetExposure(), 0.0f, 5.0f, PropertyFlag::SliderProperty);

		Property("Radiance Prefiltering", m_RadiancePrefilter);
		Property("Env Map Rotation", m_EnvMapRotation, -360.0f, 360.0f, PropertyFlag::SliderProperty);

		if (m_SceneState == SceneState::Edit)
		{
			float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
			if (Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
			{
				m_EditorScene->SetPhysics2DGravity(physics2DGravity);
			}
		}
		else if (m_SceneState == SceneState::Play)
		{
			float physics2DGravity = m_RuntimeScene->GetPhysics2DGravity();
			if (Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
			{
				m_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
			}
		}

		if (Property("Show Bounding Boxes", m_UIShowBoundingBoxes))
			ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);
		if (m_UIShowBoundingBoxes && Property("On Top", m_UIShowBoundingBoxesOnTop))
			ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);

		char* label = m_SelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
		if (ImGui::Button(label))
		{
			m_SelectionMode = m_SelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
		}

		ImGui::Columns(1);

		ImGui::End();

		ImGui::Separator();
		{
			ImGui::Text("Mesh");
			/*auto meshComponent = m_MeshEntity.GetComponent<MeshComponent>();
			std::string fullpath = meshComponent.Mesh ? meshComponent.Mesh->GetFilePath() : "None";
			size_t found = fullpath.find_last_of("/\\");
			std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
			ImGui::Text(path.c_str()); ImGui::SameLine();
			if (ImGui::Button("...##Mesh"))
			{
				std::string filename = Application::Get().OpenFile("");
				if (filename != "")
				{
					auto newMesh = Ref<Mesh>::Create(filename);
					// m_MeshMaterial.reset(new MaterialInstance(newMesh->GetMaterial()));
					// m_MeshEntity->SetMaterial(m_MeshMaterial);
					meshComponent.Mesh = newMesh;
				}
			}*/
		}
		ImGui::Separator();

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::s_AllShaders;
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

		// ImGui::ShowDemoWindow();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.8f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::Begin("Toolbar");
		if (m_SceneState == SceneState::Edit)
		{
			if (ImGui::ImageButton((ImTextureID)(m_PlayButtonTex->GetRendererID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0,0,0,0), ImVec4(0.9f, 0.9f, 0.9f, 1.0f)))
			{
				OnScenePlay();
			}
		}
		else if (m_SceneState == SceneState::Play)
		{
			if (ImGui::ImageButton((ImTextureID)(m_PlayButtonTex->GetRendererID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1.0f, 1.0f, 1.0f, 0.2f)))
			{
				OnSceneStop();
			}
		}
		ImGui::SameLine();
		if (ImGui::ImageButton((ImTextureID)(m_PlayButtonTex->GetRendererID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(1.0f, 1.0f, 1.0f, 0.6f)))
		{
			HZ_CORE_INFO("PLAY!");
		}
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");

		m_ViewportPanelMouseOver = ImGui::IsWindowHovered();
		m_ViewportPanelFocused = ImGui::IsWindowFocused();

		auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
		auto viewportSize = ImGui::GetContentRegionAvail();
		SceneRenderer::SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		m_EditorScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		if (m_RuntimeScene)
			m_RuntimeScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		m_EditorCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
		m_EditorCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		ImGui::Image((void*)SceneRenderer::GetFinalColorBufferRendererID(), viewportSize, { 0, 1 }, { 1, 0 });

		static int counter = 0;
		auto windowSize = ImGui::GetWindowSize();
		ImVec2 minBound = ImGui::GetWindowPos();
		minBound.x += viewportOffset.x;
		minBound.y += viewportOffset.y;

		ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
		m_ViewportBounds[0] = { minBound.x, minBound.y };
		m_ViewportBounds[1] = { maxBound.x, maxBound.y };
		m_AllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound);

		// Gizmos
		if (m_GizmoType != -1 && m_SelectionContext.size())
		{
			auto& selection = m_SelectionContext[0];

			float rw = (float)ImGui::GetWindowWidth();
			float rh = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

			bool snap = Input::IsKeyPressed(HZ_KEY_LEFT_CONTROL);

			auto& entityTransform = selection.Entity.Transform();
			float snapValue = GetSnapValue();
			float snapValues[3] = { snapValue, snapValue, snapValue };
			if (m_SelectionMode == SelectionMode::Entity)
			{
				ImGuizmo::Manipulate(glm::value_ptr(m_EditorCamera.GetViewMatrix()),
					glm::value_ptr(m_EditorCamera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)m_GizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(entityTransform),
					nullptr,
					snap ? snapValues : nullptr);
			}
			else
			{
				glm::mat4 transformBase = entityTransform * selection.Mesh->Transform;
				ImGuizmo::Manipulate(glm::value_ptr(m_EditorCamera.GetViewMatrix()),
					glm::value_ptr(m_EditorCamera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)m_GizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(transformBase),
					nullptr,
					snap ? snapValues : nullptr);

				selection.Mesh->Transform = glm::inverse(entityTransform) * transformBase;
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
				{
					// TODO:
				}
				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
					OpenScene();
				ImGui::Separator();
				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();
				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();
			
				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
					p_open = false;
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem("Reload C# Assembly"))
					ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");

				ImGui::MenuItem("Reload assembly on play", nullptr, &m_ReloadScriptOnPlay);
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		m_SceneHierarchyPanel->OnImGuiRender();
		
		ImGui::Begin("Materials");

		if (m_SelectionContext.size())
		{
			Entity selectedEntity = m_SelectionContext.front().Entity;
			if (selectedEntity.HasComponent<MeshComponent>())
			{
				Ref<Mesh> mesh = selectedEntity.GetComponent<MeshComponent>().Mesh;
				if (mesh)
				{
					auto& materials = mesh->GetMaterials();
					static uint32_t selectedMaterialIndex = 0;
					for (uint32_t i = 0; i < materials.size(); i++)
					{
						auto& materialInstance = materials[i];

						ImGuiTreeNodeFlags node_flags = (selectedMaterialIndex == i ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
						bool opened = ImGui::TreeNodeEx((void*)(&materialInstance), node_flags, materialInstance->GetName().c_str());
						if (ImGui::IsItemClicked())
						{
							selectedMaterialIndex = i;
						}
						if (opened)
							ImGui::TreePop();

					}

					ImGui::Separator();

					// Selected material
					if (selectedMaterialIndex < materials.size())
					{
						auto& materialInstance = materials[selectedMaterialIndex];
						ImGui::Text("Shader: %s", materialInstance->GetShader()->GetName().c_str());
						// Textures ------------------------------------------------------------------------------
						{
							// Albedo
							if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

								auto& albedoColor = materialInstance->Get<glm::vec3>("u_AlbedoColor");
								bool useAlbedoMap = materialInstance->Get<float>("u_AlbedoTexToggle");
								Ref<Texture2D> albedoMap = materialInstance->TryGetResource<Texture2D>("u_AlbedoTexture");
								ImGui::Image(albedoMap ? (void*)albedoMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (albedoMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(albedoMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										ImGui::Image((void*)albedoMap->GetRendererID(), ImVec2(384, 384));
										ImGui::EndTooltip();
									}
									if (ImGui::IsItemClicked())
									{
										std::string filename = Application::Get().OpenFile("");
										if (filename != "")
										{
											albedoMap = Texture2D::Create(filename, true/*m_AlbedoInput.SRGB*/);
											materialInstance->Set("u_AlbedoTexture", albedoMap);
										}
									}
								}
								ImGui::SameLine();
								ImGui::BeginGroup();
								if (ImGui::Checkbox("Use##AlbedoMap", &useAlbedoMap))
									materialInstance->Set<float>("u_AlbedoTexToggle", useAlbedoMap ? 1.0f : 0.0f);

								/*if (ImGui::Checkbox("sRGB##AlbedoMap", &m_AlbedoInput.SRGB))
								{
									if (m_AlbedoInput.TextureMap)
										m_AlbedoInput.TextureMap = Texture2D::Create(m_AlbedoInput.TextureMap->GetPath(), m_AlbedoInput.SRGB);
								}*/
								ImGui::EndGroup();
								ImGui::SameLine();
								ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(albedoColor), ImGuiColorEditFlags_NoInputs);
							}
						}
						{
							// Normals
							if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
								bool useNormalMap = materialInstance->Get<float>("u_NormalTexToggle");
								Ref<Texture2D> normalMap = materialInstance->TryGetResource<Texture2D>("u_NormalTexture");
								ImGui::Image(normalMap ? (void*)normalMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (normalMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(normalMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										ImGui::Image((void*)normalMap->GetRendererID(), ImVec2(384, 384));
										ImGui::EndTooltip();
									}
									if (ImGui::IsItemClicked())
									{
										std::string filename = Application::Get().OpenFile("");
										if (filename != "")
										{
											normalMap = Texture2D::Create(filename);
											materialInstance->Set("u_NormalTexture", normalMap);
										}
									}
								}
								ImGui::SameLine();
								if (ImGui::Checkbox("Use##NormalMap", &useNormalMap))
									materialInstance->Set<float>("u_NormalTexToggle", useNormalMap ? 1.0f : 0.0f);
							}
						}
						{
							// Metalness
							if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
								float& metalnessValue = materialInstance->Get<float>("u_Metalness");
								bool useMetalnessMap = materialInstance->Get<float>("u_MetalnessTexToggle");
								Ref<Texture2D> metalnessMap = materialInstance->TryGetResource<Texture2D>("u_MetalnessTexture");
								ImGui::Image(metalnessMap ? (void*)metalnessMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (metalnessMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(metalnessMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										ImGui::Image((void*)metalnessMap->GetRendererID(), ImVec2(384, 384));
										ImGui::EndTooltip();
									}
									if (ImGui::IsItemClicked())
									{
										std::string filename = Application::Get().OpenFile("");
										if (filename != "")
										{
											metalnessMap = Texture2D::Create(filename);
											materialInstance->Set("u_MetalnessTexture", metalnessMap);
										}
									}
								}
								ImGui::SameLine();
								if (ImGui::Checkbox("Use##MetalnessMap", &useMetalnessMap))
									materialInstance->Set<float>("u_MetalnessTexToggle", useMetalnessMap ? 1.0f : 0.0f);
								ImGui::SameLine();
								ImGui::SliderFloat("Value##MetalnessInput", &metalnessValue, 0.0f, 1.0f);
							}
						}
						{
							// Roughness
							if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
								float& roughnessValue = materialInstance->Get<float>("u_Roughness");
								bool useRoughnessMap = materialInstance->Get<float>("u_RoughnessTexToggle");
								Ref<Texture2D> roughnessMap = materialInstance->TryGetResource<Texture2D>("u_RoughnessTexture");
								ImGui::Image(roughnessMap ? (void*)roughnessMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (roughnessMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(roughnessMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										ImGui::Image((void*)roughnessMap->GetRendererID(), ImVec2(384, 384));
										ImGui::EndTooltip();
									}
									if (ImGui::IsItemClicked())
									{
										std::string filename = Application::Get().OpenFile("");
										if (filename != "")
										{
											roughnessMap = Texture2D::Create(filename);
											materialInstance->Set("u_RoughnessTexture", roughnessMap);
										}
									}
								}
								ImGui::SameLine();
								if (ImGui::Checkbox("Use##RoughnessMap", &useRoughnessMap))
									materialInstance->Set<float>("u_RoughnessTexToggle", useRoughnessMap ? 1.0f : 0.0f);
								ImGui::SameLine();
								ImGui::SliderFloat("Value##RoughnessInput", &roughnessValue, 0.0f, 1.0f);
							}
						}
					}
				}
			}
		}

		ImGui::End();

		ScriptEngine::OnImGuiRender();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (m_SceneState == SceneState::Edit)
		{
			if (m_ViewportPanelMouseOver)
				m_EditorCamera.OnEvent(e);

			m_EditorScene->OnEvent(e);
		}
		else if (m_SceneState == SceneState::Play)
		{
			m_RuntimeScene->OnEvent(e);
		}

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(EditorLayer::OnKeyPressedEvent));
		dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
	}

	bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		if (m_ViewportPanelFocused)
		{
			switch (e.GetKeyCode())
			{
				case KeyCode::Q:
					m_GizmoType = -1;
					break;
				case KeyCode::W:
					m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
					break;
				case KeyCode::E:
					m_GizmoType = ImGuizmo::OPERATION::ROTATE;
					break;
				case KeyCode::R:
					m_GizmoType = ImGuizmo::OPERATION::SCALE;
					break;
				case KeyCode::Delete:
					if (m_SelectionContext.size())
					{
						Entity selectedEntity = m_SelectionContext[0].Entity;
						m_EditorScene->DestroyEntity(selectedEntity);
						m_SelectionContext.clear();
						m_EditorScene->SetSelectedEntity({});
						m_SceneHierarchyPanel->SetSelected({});
					}
					break;
			}
		}

		if (Input::IsKeyPressed(HZ_KEY_LEFT_CONTROL))
		{
			switch (e.GetKeyCode())
			{
				case KeyCode::B:
					// Toggle bounding boxes 
					m_UIShowBoundingBoxes = !m_UIShowBoundingBoxes;
					ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);
					break;
				case KeyCode::D:
					if (m_SelectionContext.size())
					{
						Entity selectedEntity = m_SelectionContext[0].Entity;
						m_EditorScene->DuplicateEntity(selectedEntity);
					}
					break;
				case KeyCode::G:
					// Toggle grid
					SceneRenderer::GetOptions().ShowGrid = !SceneRenderer::GetOptions().ShowGrid;
					break;
				case KeyCode::O:
					OpenScene();
					break;
				case KeyCode::S:
					SaveScene();
					break;
			}

			if (Input::IsKeyPressed(HZ_KEY_LEFT_SHIFT))
			{
				switch (e.GetKeyCode())
				{
				case KeyCode::S:
					SaveSceneAs();
					break;
				}
			}
		}

		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		auto [mx, my] = Input::GetMousePosition();
		if (e.GetMouseButton() == HZ_MOUSE_BUTTON_LEFT && !Input::IsKeyPressed(KeyCode::LeftAlt) && !ImGuizmo::IsOver() && m_SceneState != SceneState::Play)
		{
			auto [mouseX, mouseY] = GetMouseViewportSpace();
			if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
			{
				auto [origin, direction] = CastRay(mouseX, mouseY);

				m_SelectionContext.clear();
				m_EditorScene->SetSelectedEntity({});
				auto meshEntities = m_EditorScene->GetAllEntitiesWith<MeshComponent>();
				for (auto e : meshEntities)
				{
					Entity entity = { e, m_EditorScene.Raw() };
					auto mesh = entity.GetComponent<MeshComponent>().Mesh;
					if (!mesh)
						continue;

					auto& submeshes = mesh->GetSubmeshes();
					float lastT = std::numeric_limits<float>::max();
					for (uint32_t i = 0; i < submeshes.size(); i++)
					{
						auto& submesh = submeshes[i];
						Ray ray = {
							glm::inverse(entity.Transform() * submesh.Transform) * glm::vec4(origin, 1.0f),
							glm::inverse(glm::mat3(entity.Transform()) * glm::mat3(submesh.Transform)) * direction
						};

						float t;
						bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
						if (intersects)
						{
							const auto& triangleCache = mesh->GetTriangleCache(i);
							for (const auto& triangle : triangleCache)
							{
								if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
								{
									HZ_WARN("INTERSECTION: {0}, t={1}", submesh.NodeName, t);
									m_SelectionContext.push_back({ entity, &submesh, t });
									break;
								}
							}
						}
					}
				}
				std::sort(m_SelectionContext.begin(), m_SelectionContext.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });
				if (m_SelectionContext.size())
					OnSelected(m_SelectionContext[0]);

			}
		}
		return false;
	}

	std::pair<float, float> EditorLayer::GetMouseViewportSpace()
	{
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
		auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
	}

	std::pair<glm::vec3, glm::vec3> EditorLayer::CastRay(float mx, float my)
	{
		glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		auto inverseProj = glm::inverse(m_EditorCamera.GetProjectionMatrix());
		auto inverseView = glm::inverse(glm::mat3(m_EditorCamera.GetViewMatrix()));

		glm::vec4 ray = inverseProj * mouseClipPos;
		glm::vec3 rayPos = m_EditorCamera.GetPosition();
		glm::vec3 rayDir = inverseView * glm::vec3(ray);

		return { rayPos, rayDir };
	}

	void EditorLayer::OnSelected(const SelectedSubmesh& selectionContext)
	{
		m_SceneHierarchyPanel->SetSelected(selectionContext.Entity);
		m_EditorScene->SetSelectedEntity(selectionContext.Entity);
	}

	void EditorLayer::OnEntityDeleted(Entity e)
	{
		if (m_SelectionContext[0].Entity == e)
		{
			m_SelectionContext.clear();
			m_EditorScene->SetSelectedEntity({});
		}
	}

	Ray EditorLayer::CastMouseRay()
	{
		auto [mouseX, mouseY] = GetMouseViewportSpace();
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
		{
			auto [origin, direction] = CastRay(mouseX, mouseY);
			return Ray(origin, direction);
		}
		return Ray::Zero();
	}

}