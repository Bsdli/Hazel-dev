#include "EditorLayer.h"

#include "Hazel/ImGui/ImGuizmo.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Editor/PhysicsSettingsWindow.h"
#include "Hazel/Editor/AssetEditorPanel.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Hazel/Physics/Physics.h"
#include "Hazel/Math/Math.h"
#include "Hazel/Utilities/FileSystem.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Platform/OpenGL/OpenGLFramebuffer.h"

#include "imgui_internal.h"
#include "Hazel/ImGui/ImGui.h"

namespace Hazel {

	/*static void ImGuiShowHelpMarker(const char* desc)
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
	}*/

	EditorLayer::EditorLayer()
		: m_SceneType(SceneType::Model), m_EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
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
		m_PauseButtonTex = Texture2D::Create("assets/editor/PauseButton.png");
		m_StopButtonTex = Texture2D::Create("assets/editor/StopButton.png");

		m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(m_EditorScene);
		m_SceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EditorLayer::SelectEntity, this, std::placeholders::_1));
		m_SceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EditorLayer::OnEntityDeleted, this, std::placeholders::_1));

		m_ContentBrowserPanel = CreateScope<ContentBrowserPanel>();
		m_ObjectsPanel = CreateScope<ObjectsPanel>();

		NewScene();
		//OpenScene("assets/scenes/ShadowTest.hsc");

		AssetEditorPanel::RegisterDefaultEditors();
		FileSystem::StartWatching();
	}

	void EditorLayer::OnDetach()
	{
		FileSystem::StopWatching();
		AssetEditorPanel::UnregisterAllEditors();
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
		m_CurrentScene = m_RuntimeScene;
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
		m_CurrentScene = m_EditorScene;
	}

	void EditorLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string rendererAPI = RendererAPI::Current() == RendererAPIType::Vulkan ? "Vulkan" : "OpenGL";
		std::string title = sceneName + " - Hazelnut - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ") Renderer: " + rendererAPI;
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
		auto [x, y] = GetMouseViewportSpace();

		//SceneRenderer::SetFocusPoint({ x * 0.5f + 0.5f, y * 0.5f + 0.5f });

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
						Renderer::DrawAABB(selection.Mesh->BoundingBox, selection.Entity.Transform().GetTransform() * selection.Mesh->Transform, color);
						Renderer2D::EndScene();
						Renderer::EndRenderPass();
					}
				}

				if (m_SelectionContext.size())
				{
					auto& selection = m_SelectionContext[0];

					if (selection.Entity.HasComponent<BoxCollider2DComponent>() && false)
					{
						const auto& size = selection.Entity.GetComponent<BoxCollider2DComponent>().Size;
						const TransformComponent& transform = selection.Entity.GetComponent<TransformComponent>();

						Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
						auto viewProj = m_EditorCamera.GetViewProjection();
						Renderer2D::BeginScene(viewProj, false);
						Renderer2D::DrawRotatedRect({ transform.Translation.x, transform.Translation.y }, size * 2.0f, transform.Rotation.z, { 0.0f, 1.0f, 1.0f, 1.0f });
						Renderer2D::EndScene();
						Renderer::EndRenderPass();
					}

					if (selection.Entity.HasComponent<CircleCollider2DComponent>())
					{
						const auto& size = selection.Entity.GetComponent<CircleCollider2DComponent>().Radius;
						const TransformComponent& transform = selection.Entity.GetComponent<TransformComponent>();

						Renderer::BeginRenderPass(SceneRenderer::GetFinalRenderPass(), false);
						auto viewProj = m_EditorCamera.GetViewProjection();
						Renderer2D::BeginScene(viewProj, false);
						Renderer2D::DrawCircle({ transform.Translation.x, transform.Translation.y }, size, { 0.0f, 1.0f, 1.0f, 1.0f });
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

	void EditorLayer::ShowBoundingBoxes(bool show, bool onTop)
	{
		SceneRenderer::GetOptions().ShowBoundingBoxes = show && !onTop;
		m_DrawOnTopBoundingBoxes = show && onTop;
	}

	void EditorLayer::SelectEntity(Entity entity)
	{
		if (!entity)
		{
			return;
		}

		SelectedSubmesh selection;
		if (entity.HasComponent<MeshComponent>())
		{
			auto& meshComp = entity.GetComponent<MeshComponent>();

			if (meshComp.Mesh && meshComp.Mesh->Type == AssetType::Mesh)
			{
				selection.Mesh = &meshComp.Mesh->GetSubmeshes()[0];
			}
		}
		selection.Entity = entity;
		m_SelectionContext.clear();
		m_SelectionContext.push_back(selection);

		m_EditorScene->SetSelectedEntity(entity);

		m_CurrentScene = m_EditorScene;
	}

	void EditorLayer::NewScene()
	{
		// Clear
		m_SelectionContext = {};

		m_EditorScene = Ref<Scene>::Create("Empty Scene", true);
		m_SceneHierarchyPanel->SetContext(m_EditorScene);
		ScriptEngine::SetSceneContext(m_EditorScene);
		UpdateWindowTitle("Untitled Scene");
		m_SceneFilePath = std::string();

		m_EditorCamera = EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
		m_CurrentScene = m_EditorScene;
	}

	void EditorLayer::OpenScene()
	{
		auto& app = Application::Get();
		std::string filepath = app.OpenFile("Hazel Scene (*.hsc)\0*.hsc\0");
		if (!filepath.empty())
			OpenScene(filepath);
	}

	void EditorLayer::OpenScene(const std::string& filepath)
	{
		Ref<Scene> newScene = Ref<Scene>::Create("New Scene", true);
		SceneSerializer serializer(newScene);
		serializer.Deserialize(filepath);
		m_EditorScene = newScene;
		m_SceneFilePath = filepath;

		std::filesystem::path path = filepath;
		UpdateWindowTitle(path.filename().string());
		m_SceneHierarchyPanel->SetContext(m_EditorScene);
		ScriptEngine::SetSceneContext(m_EditorScene);

		m_EditorScene->SetSelectedEntity({});
		m_SelectionContext.clear();

		m_CurrentScene = m_EditorScene;
	}

	void EditorLayer::SaveScene()
	{
		if (!m_SceneFilePath.empty())
		{
			SceneSerializer serializer(m_EditorScene);
			serializer.Serialize(m_SceneFilePath);
		}
		else
		{
			SaveSceneAs();
		}
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

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto boldFont = io.Fonts->Fonts[0];
		auto largeFont = io.Fonts->Fonts[1];

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
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		// Editor Panel ------------------------------------------------------------------------------
		ImGui::Begin("Settings");
		{
			auto& rendererConfig = Renderer::GetConfig();

			UI::BeginPropertyGrid();
			ImGui::AlignTextToFramePadding();

			UI::PropertySlider("Skybox LOD", m_EditorScene->GetSkyboxLod(), 0.0f, Utils::CalculateMipCount(rendererConfig.EnvironmentMapResolution, rendererConfig.EnvironmentMapResolution));
			UI::PropertySlider("Exposure", m_EditorCamera.GetExposure(), 0.0f, 5.0f);
			UI::PropertySlider("Env Map Rotation", m_EnvMapRotation, -360.0f, 360.0f);

			if (m_SceneState == SceneState::Edit)
			{
				float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
				if (UI::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f))
				{
					m_EditorScene->SetPhysics2DGravity(physics2DGravity);
				}
			}
			else if (m_SceneState == SceneState::Play)
			{
				float physics2DGravity = m_RuntimeScene->GetPhysics2DGravity();
				if (UI::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f))
				{
					m_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
				}
			}

			if (UI::Property("Show Bounding Boxes", m_UIShowBoundingBoxes))
				ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);
			if (m_UIShowBoundingBoxes && UI::Property("On Top", m_UIShowBoundingBoxesOnTop))
				ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);

			char* label = m_SelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
			if (ImGui::Button(label))
			{
				m_SelectionMode = m_SelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
			}

			UI::EndPropertyGrid();

			ImGui::Separator();
			ImGui::PushFont(boldFont);
			ImGui::Text("Renderer Settings");
			ImGui::PopFont();
			UI::BeginPropertyGrid();
			UI::Property("Enable HDR environment maps", rendererConfig.ComputeEnvironmentMaps);

			{
				const char* environmentMapSizes[] = { "128", "256", "512", "1024", "2048", "4096" };
				int currentSize = (int)glm::log2((float)rendererConfig.EnvironmentMapResolution) - 7;
				if (UI::PropertyDropdown("Environment Map Size", environmentMapSizes, 6, &currentSize))
				{
					rendererConfig.EnvironmentMapResolution = glm::pow(2, currentSize + 7);
				}
			}

			{
				const char* irradianceComputeSamples[] = { "128", "256", "512", "1024", "2048", "4096" };
				int currentSamples = (int)glm::log2((float)rendererConfig.IrradianceMapComputeSamples) - 7;
				if (UI::PropertyDropdown("Irradiance Map Compute Samples", irradianceComputeSamples, 6, &currentSamples))
				{
					rendererConfig.IrradianceMapComputeSamples = glm::pow(2, currentSamples + 7);
				}
			}
			UI::EndPropertyGrid();
		}
		ImGui::End();
		
		m_ContentBrowserPanel->OnImGuiRender();
		m_ObjectsPanel->OnImGuiRender();
		AssetEditorPanel::OnImGuiRender();

		// ImGui::ShowDemoWindow();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.305f, 0.31f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.1505f, 0.151f, 0.5f));

		ImGui::Begin("##tool_bar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			float size = ImGui::GetWindowHeight() - 4.0F;
			ImGui::SameLine((ImGui::GetWindowContentRegionMax().x / 2.0f) - (1.5f * (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x)) - (size / 2.0f));
			Ref<Texture2D> buttonTex = m_SceneState == SceneState::Play ? m_StopButtonTex : m_PlayButtonTex;
			if (UI::ImageButton(buttonTex, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				if (m_SceneState == SceneState::Edit)
					OnScenePlay();
				else
					OnSceneStop();
			}

			ImGui::SameLine();

			if (UI::ImageButton(m_PauseButtonTex, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				if (m_SceneState == SceneState::Play)
				{
					//OnScenePause();
					m_SceneState = SceneState::Pause;
				}
				else if (m_SceneState == SceneState::Pause)
				{
					//OnSceneResume();
					m_SceneState = SceneState::Play;
				}
			}
		}
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		ImGui::End();

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
		m_EditorCamera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 1000.0f));
		m_EditorCamera.SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

		// Render viewport image
		UI::Image(SceneRenderer::GetFinalPassImage(), viewportSize, { 0, 1 }, { 1, 0 });

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

			TransformComponent& entityTransform = selection.Entity.Transform();
			glm::mat4 transform = m_CurrentScene->GetTransformRelativeToParent(selection.Entity);
			float snapValue = GetSnapValue();
			float snapValues[3] = { snapValue, snapValue, snapValue };

			if (m_SelectionMode == SelectionMode::Entity)
			{
				ImGuizmo::Manipulate(glm::value_ptr(m_EditorCamera.GetViewMatrix()),
					glm::value_ptr(m_EditorCamera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)m_GizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(transform),
					nullptr,
					snap ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					Math::DecomposeTransform(transform, translation, rotation, scale);

					Entity parent = m_CurrentScene->FindEntityByUUID(selection.Entity.GetParentUUID());
					if (parent)
					{
						glm::vec3 parentTranslation, parentRotation, parentScale;
						Math::DecomposeTransform(m_CurrentScene->GetTransformRelativeToParent(parent), parentTranslation, parentRotation, parentScale);

						glm::vec3 deltaRotation = (rotation - parentRotation) - entityTransform.Rotation;
						entityTransform.Translation = translation - parentTranslation;
						entityTransform.Rotation += deltaRotation;
						entityTransform.Scale = scale;
					}
					else
					{
						glm::vec3 deltaRotation = rotation - entityTransform.Rotation;
						entityTransform.Translation = translation;
						entityTransform.Rotation += deltaRotation;
						entityTransform.Scale = scale;
					}
				}
			}
			else
			{
				glm::mat4 transformBase = transform * selection.Mesh->Transform;
				ImGuizmo::Manipulate(glm::value_ptr(m_EditorCamera.GetViewMatrix()),
					glm::value_ptr(m_EditorCamera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)m_GizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(transformBase),
					nullptr,
					snap ? snapValues : nullptr);

				selection.Mesh->Transform = glm::inverse(transform) * transformBase;
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("asset_payload");
			if (data)
			{
				int count = data->DataSize / sizeof(AssetHandle);

				for (int i = 0; i < count; i++)
				{
					AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);

					// We can't really support dragging and dropping scenes when we're dropping multiple assets
					if (count == 1 && asset->Type == AssetType::Scene)
					{
						OpenScene(asset->FilePath);
					}

					if (asset->Type == AssetType::Mesh)
					{
						Entity entity = m_EditorScene->CreateEntity(asset->FileName);
						entity.AddComponent<MeshComponent>(Ref<Mesh>(asset));
						SelectEntity(entity);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					NewScene();
				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
					OpenScene();
				ImGui::Separator();
				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();
				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();

				ImGui::Separator();
				std::string otherRenderer = RendererAPI::Current() == RendererAPIType::Vulkan ? "OpenGL" : "Vulkan";
				std::string label = std::string("Restart with ") + otherRenderer;
				if (ImGui::MenuItem(label.c_str()))
				{
					RendererAPI::SetAPI(RendererAPI::Current() == RendererAPIType::Vulkan ? RendererAPIType::OpenGL : RendererAPIType::Vulkan);
					Application::Get().Close();
				}
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

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::MenuItem("Physics Settings", nullptr, &m_ShowPhysicsSettings);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("About"))
					m_ShowAboutPopup = true;
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
				if (mesh && mesh->Type == AssetType::Mesh)
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

								auto& albedoColor = materialInstance->GetVector3("u_MaterialUniforms.AlbedoColor");
								bool useAlbedoMap = true;// materialInstance->GetFloat("u_MaterialUniforms.AlbedoTexToggle");
								Ref<Texture2D> albedoMap = materialInstance->TryGetTexture2D("u_AlbedoTexture");
								bool hasAlbedoMap = !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) && albedoMap->GetImage();
								Ref<Texture2D> albedoUITexture = hasAlbedoMap ? albedoMap : m_CheckerboardTex;
								UI::Image(albedoUITexture, ImVec2(64, 64));

								if (ImGui::BeginDragDropTarget())
								{
									auto data = ImGui::AcceptDragDropPayload("asset_payload");
									if (data)
									{
										int count = data->DataSize / sizeof(AssetHandle);

										for (int i = 0; i < count; i++)
										{
											if (count > 1)
												break;

											AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
											Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
											if (asset->Type != AssetType::Texture)
												break;

											albedoMap = asset.As<Texture2D>();
											materialInstance->Set("u_AlbedoTexture", albedoMap);
											// NOTE: Uncomment when u_MaterialUniforms.AlbedoTexToggle is a thing
											//materialInstance->Set("u_MaterialUniforms.AlbedoTexToggle", true);
										}
									}

									ImGui::EndDragDropTarget();
								}

								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (hasAlbedoMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(albedoMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										UI::Image(albedoUITexture, ImVec2(384, 384));
										ImGui::EndTooltip();
									}
									if (ImGui::IsItemClicked())
									{
										std::string filename = Application::Get().OpenFile("");
										if (!filename.empty())
										{
											TextureProperties props;
											props.SRGB = true;
											albedoMap = Texture2D::Create(filename, props);
											materialInstance->Set("u_AlbedoTexture", albedoMap);
										}
									}
								}
								ImGui::SameLine();
								ImGui::BeginGroup();
								ImGui::Checkbox("Use##AlbedoMap", &useAlbedoMap);

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
								bool useNormalMap = materialInstance->GetFloat("u_MaterialUniforms.UseNormalMap");
								Ref<Texture2D> normalMap = materialInstance->TryGetTexture2D("u_NormalTexture");
								UI::Image((normalMap && normalMap->GetImage()) ? normalMap : m_CheckerboardTex, ImVec2(64, 64));

								if (ImGui::BeginDragDropTarget())
								{
									auto data = ImGui::AcceptDragDropPayload("asset_payload");
									if (data)
									{
										int count = data->DataSize / sizeof(AssetHandle);

										for (int i = 0; i < count; i++)
										{
											if (count > 1)
												break;

											AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
											Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
											if (asset->Type != AssetType::Texture)
												break;

											normalMap = asset.As<Texture2D>();
											materialInstance->Set("u_NormalTexture", normalMap);
											materialInstance->Set("u_MaterialUniforms.UseNormalMap", true);
										}
									}

									ImGui::EndDragDropTarget();
								}

								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (normalMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(normalMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										UI::Image(normalMap, ImVec2(384, 384));
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
									materialInstance->Set("u_MaterialUniforms.UseNormalMap", useNormalMap);
							}
						}
						{
							// Metalness
							if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
								float& metalnessValue = materialInstance->GetFloat("u_MaterialUniforms.Metalness");
								bool useMetalnessMap = true;// materialInstance->GetFloat("u_MaterialUniforms.MetalnessTexToggle");
								Ref<Texture2D> metalnessMap = materialInstance->TryGetTexture2D("u_MetalnessTexture");
								UI::Image((metalnessMap && metalnessMap->GetImage()) ? metalnessMap : m_CheckerboardTex, ImVec2(64, 64));

								if (ImGui::BeginDragDropTarget())
								{
									auto data = ImGui::AcceptDragDropPayload("asset_payload");
									if (data)
									{
										int count = data->DataSize / sizeof(AssetHandle);

										for (int i = 0; i < count; i++)
										{
											if (count > 1)
												break;

											AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
											Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
											if (asset->Type != AssetType::Texture)
												break;

											metalnessMap = asset.As<Texture2D>();
											materialInstance->Set("u_MetalnessTexture", metalnessMap);
											// NOTE: Uncomment when u_MaterialUniforms.MetalnessTexToggle is a thing
											//materialInstance->Set("u_MaterialUniforms.MetalnessTexToggle", true);
										}
									}

									ImGui::EndDragDropTarget();
								}

								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (metalnessMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(metalnessMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										UI::Image(metalnessMap, ImVec2(384, 384));
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
								ImGui::Checkbox("Use##MetalnessMap", &useMetalnessMap);
								ImGui::SameLine();
								ImGui::SliderFloat("Value##MetalnessInput", &metalnessValue, 0.0f, 1.0f);
							}
						}
						{
							// Roughness
							if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
							{
								ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
								float& roughnessValue = materialInstance->GetFloat("u_MaterialUniforms.Roughness");
								bool useRoughnessMap = true;// materialInstance->GetFloat("u_MaterialUniforms.RoughnessTexToggle");
								Ref<Texture2D> roughnessMap = materialInstance->TryGetTexture2D("u_RoughnessTexture");
								UI::Image((roughnessMap && roughnessMap->GetImage()) ? roughnessMap : m_CheckerboardTex, ImVec2(64, 64));

								if (ImGui::BeginDragDropTarget())
								{
									auto data = ImGui::AcceptDragDropPayload("asset_payload");
									if (data)
									{
										int count = data->DataSize / sizeof(AssetHandle);

										for (int i = 0; i < count; i++)
										{
											if (count > 1)
												break;

											AssetHandle assetHandle = *(((AssetHandle*)data->Data) + i);
											Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
											if (asset->Type != AssetType::Texture)
												break;

											roughnessMap = asset.As<Texture2D>();
											materialInstance->Set("u_RoughnessTexture", roughnessMap);
											// NOTE: Uncomment when u_MaterialUniforms.RoughnessTexToggle is a thing
											//materialInstance->Set("u_MaterialUniforms.RoughnessTexToggle", true);
										}
									}

									ImGui::EndDragDropTarget();
								}

								ImGui::PopStyleVar();
								if (ImGui::IsItemHovered())
								{
									if (roughnessMap)
									{
										ImGui::BeginTooltip();
										ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
										ImGui::TextUnformatted(roughnessMap->GetPath().c_str());
										ImGui::PopTextWrapPos();
										UI::Image(roughnessMap, ImVec2(384, 384));
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
								ImGui::Checkbox("Use##RoughnessMap", &useRoughnessMap);
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
		SceneRenderer::OnImGuiRender();
		PhysicsSettingsWindow::OnImGuiRender(m_ShowPhysicsSettings);

		ImGui::End();

		if (m_ShowWelcomePopup)
		{
			ImGui::OpenPopup("Welcome");
			m_ShowWelcomePopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 400,0 });
		if (ImGui::BeginPopupModal("Welcome", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Welcome to Hazel!");
			ImGui::Separator();
			ImGui::TextWrapped("Environment maps are currently disabled because they're a little unstable on certain GPU drivers.");

			UI::BeginPropertyGrid();
			UI::Property("Enable environment maps?", Renderer::GetConfig().ComputeEnvironmentMaps);
			UI::EndPropertyGrid();

			if (ImGui::Button("OK"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		if (m_ShowAboutPopup)
		{
			ImGui::OpenPopup("About##AboutPopup");
			m_ShowAboutPopup = false;
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 600,0 });
		if (ImGui::BeginPopupModal("About##AboutPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushFont(largeFont);
			ImGui::Text("Hazel Engine");
			ImGui::PopFont();

			ImGui::Separator();
			ImGui::TextWrapped("Hazel is an early-stage interactive application and rendering engine for Windows.");
			ImGui::Separator();
			ImGui::PushFont(boldFont);
			ImGui::Text("Hazel Core Team");
			ImGui::PopFont();
			ImGui::Text("Yan Chernikov");
			ImGui::Text("Peter Nilsson");
			ImGui::Text("Karim Sayed");
			ImGui::Text("Vineet Nair");
			ImGui::Separator();
			ImGui::TextColored(ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f }, "This software contains source code provided by NVIDIA Corporation.");

			if (ImGui::Button("OK"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
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
		if (GImGui->ActiveId == 0)
		{
			if (m_ViewportPanelMouseOver)
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
				case KeyCode::F:
				{
					if (m_SelectionContext.size() == 0)
						break;

					Entity selectedEntity = m_SelectionContext[0].Entity;
					m_EditorCamera.Focus(selectedEntity.Transform().Translation);
					break;
				}
				}

			}
			switch (e.GetKeyCode())
			{
			case KeyCode::Escape:
				if (m_SelectionContext.size())
				{
					m_SelectionContext.clear();
					m_EditorScene->SetSelectedEntity({});
					m_SceneHierarchyPanel->SetSelected({});
				}
				break;
			case KeyCode::Delete: // TODO: this should be in the scene hierarchy panel
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
				case KeyCode::N:
					NewScene();
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
		if (e.GetMouseButton() == HZ_MOUSE_BUTTON_LEFT && m_ViewportPanelMouseOver && !Input::IsKeyPressed(KeyCode::LeftAlt) && !ImGuizmo::IsOver() && m_SceneState != SceneState::Play)
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
						glm::mat4 transform = m_CurrentScene->GetTransformRelativeToParent(entity);
						Ray ray = {
							glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
							glm::inverse(glm::mat3(transform) * glm::mat3(submesh.Transform)) * direction
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
		if (m_SelectionContext.size() > 0 && m_SelectionContext[0].Entity == e)
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
