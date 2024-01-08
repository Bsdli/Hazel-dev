#pragma once

#include "Hazel.h"

#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Editor/EditorCamera.h"
#include "imgui/imgui_internal.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <string>

#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Editor/ContentBrowserPanel.h"
#include "Hazel/Editor/ObjectsPanel.h"

namespace Hazel {

	class EditorLayer : public Layer
	{
	public:
		enum class PropertyFlag
		{
			None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
		};
	public:
		EditorLayer();
		virtual ~EditorLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void ShowBoundingBoxes(bool show, bool onTop = false);
		void SelectEntity(Entity entity);

		void NewScene();
		void OpenScene();
		void OpenScene(const std::string& filepath);
		void SaveScene();
		void SaveSceneAs();
	private:
		std::pair<float, float> GetMouseViewportSpace();
		std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my);

		struct SelectedSubmesh
		{
			Hazel::Entity Entity;
			Submesh* Mesh = nullptr;
			float Distance = 0.0f;
		};

		void OnSelected(const SelectedSubmesh& selectionContext);
		void OnEntityDeleted(Entity e);
		Ray CastMouseRay();

		void OnScenePlay();
		void OnSceneStop();

		void UpdateWindowTitle(const std::string& sceneName);

		float GetSnapValue();
	private:
		Scope<SceneHierarchyPanel> m_SceneHierarchyPanel;
		Scope<ContentBrowserPanel> m_ContentBrowserPanel;
		Scope<ObjectsPanel> m_ObjectsPanel;

		Ref<Scene> m_RuntimeScene, m_EditorScene, m_CurrentScene;
		std::string m_SceneFilePath;
		bool m_ReloadScriptOnPlay = true;

		EditorCamera m_EditorCamera;

		Ref<Shader> m_BrushShader;
		Ref<Material> m_SphereBaseMaterial;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
			Ref<Texture2D> TextureMap;
			bool SRGB = true;
			bool UseTexture = false;
		};
		//AlbedoInput m_AlbedoInput;

		struct NormalInput
		{
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		//NormalInput m_NormalInput;

		struct MetalnessInput
		{
			float Value = 1.0f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		//MetalnessInput m_MetalnessInput;

		struct RoughnessInput
		{
			float Value = 0.2f;
			Ref<Texture2D> TextureMap;
			bool UseTexture = false;
		};
		//RoughnessInput m_RoughnessInput;

		float m_EnvMapRotation = 0.0f;

		enum class SceneType : uint32_t
		{
			Spheres = 0, Model = 1
		};
		SceneType m_SceneType;

		// Editor resources
		Ref<Texture2D> m_CheckerboardTex;
		Ref<Texture2D> m_PlayButtonTex, m_StopButtonTex, m_PauseButtonTex;

		glm::vec2 m_ViewportBounds[2];
		int m_GizmoType = -1; // -1 = no gizmo
		float m_SnapValue = 0.5f;
		float m_RotationSnapValue = 45.0f;
		bool m_AllowViewportCameraEvents = false;
		bool m_DrawOnTopBoundingBoxes = false;

		bool m_UIShowBoundingBoxes = false;
		bool m_UIShowBoundingBoxesOnTop = false;

		bool m_ViewportPanelMouseOver = false;
		bool m_ViewportPanelFocused = false;

		bool m_ShowPhysicsSettings = false;

		bool m_ShowWelcomePopup = true;
		bool m_ShowAboutPopup = false;

		enum class SceneState
		{
			Edit = 0, Play = 1, Pause = 2
		};
		SceneState m_SceneState = SceneState::Edit;

		enum class SelectionMode
		{
			None = 0, Entity = 1, SubMesh = 2
		};

		SelectionMode m_SelectionMode = SelectionMode::Entity;
		std::vector<SelectedSubmesh> m_SelectionContext;
		glm::mat4* m_RelativeTransform = nullptr;
		glm::mat4* m_CurrentlySelectedTransform = nullptr;
	};

}
