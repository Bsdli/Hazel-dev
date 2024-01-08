#include "hzpch.h"
#include "ObjectsPanel.h"
#include "Hazel/ImGui/ImGui.h"

namespace Hazel {

	ObjectsPanel::ObjectsPanel()
	{
		m_CubeImage = Texture2D::Create("assets/editor/asset.png");
	}

	void ObjectsPanel::DrawObject(const char* label, AssetHandle handle)
	{
		UI::Image(m_CubeImage, ImVec2(30, 30));
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
		ImGui::Selectable(label);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			UI::Image(m_CubeImage, ImVec2(20, 20));
			ImGui::SameLine();

			ImGui::Text(label);

			ImGui::SetDragDropPayload("asset_payload", &handle, sizeof(AssetHandle));
			ImGui::EndDragDropSource();
		}
	}

	void ObjectsPanel::OnImGuiRender()
	{
		static const AssetHandle CubeHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Cube.fbx");
		static const AssetHandle CapsuleHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Capsule.fbx");
		static const AssetHandle SphereHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Sphere.fbx");
		static const AssetHandle CylinderHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Cylinder.fbx");
		static const AssetHandle TorusHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Torus.fbx");
		static const AssetHandle PlaneHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Plane.fbx");
		static const AssetHandle ConeHandle = AssetManager::GetAssetHandleFromFilePath("assets/meshes/Default/Cone.fbx");

		ImGui::Begin("Objects");
		{
			ImGui::BeginChild("##objects_window");
			DrawObject("Cube", CubeHandle);
			DrawObject("Capsule", CapsuleHandle);
			DrawObject("Sphere", SphereHandle);
			DrawObject("Cylinder", CylinderHandle);
			DrawObject("Torus", TorusHandle);
			DrawObject("Plane", PlaneHandle);
			DrawObject("Cone", ConeHandle);
			ImGui::EndChild();
		}

		ImGui::End();
	}

}