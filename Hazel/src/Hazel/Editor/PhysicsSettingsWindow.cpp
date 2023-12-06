#include "hzpch.h"
#include "PhysicsSettingsWindow.h"
#include "Hazel/Physics/Physics.h"
#include "Hazel/Physics/PhysicsLayer.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Hazel/ImGui/ImGui.h"

#include <glm/gtc/type_ptr.hpp>

namespace Hazel {

	static int32_t s_SelectedLayer = -1;
	static char s_NewLayerNameBuffer[50];

	void PhysicsSettingsWindow::OnImGuiRender(bool& show)
	{
		if (!show)
			return;

		ImGui::Begin("Physics", &show);
		ImGui::PushID(0);
		ImGui::Columns(2);
		RenderWorldSettings();
		ImGui::EndColumns();
		ImGui::PopID();

		ImGui::Separator();

		UI::BeginPropertyGrid();
		RenderLayerList();
		ImGui::NextColumn();
		RenderSelectedLayer();
		UI::EndPropertyGrid();

		ImGui::End();
	}

	void PhysicsSettingsWindow::RenderWorldSettings()
	{
		PhysicsSettings& settings = Physics::GetSettings();

		UI::Property("Fixed Timestep (Default: 0.02)", settings.FixedTimestep);
		UI::Property("Gravity (Default: -9.81)", settings.Gravity.y);

		static const char* broadphaseTypeStrings[] = { "Sweep And Prune", "Multi Box Pruning", "Automatic Box Pruning" };
		UI::PropertyDropdown("Broadphase Type", broadphaseTypeStrings, 3, (int*)&settings.BroadphaseAlgorithm);

		if (settings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
		{
			UI::Property("World Bounds (Min)", settings.WorldBoundsMin);
			UI::Property("World Bounds (Max)", settings.WorldBoundsMax);
			UI::PropertySlider("Grid Subdivisions", (int&)settings.WorldBoundsSubdivisions, 1, 10000);
		}

		static const char* frictionTypeStrings[] = { "Patch", "One Directional", "Two Directional" };
		UI::PropertyDropdown("Friction Model", frictionTypeStrings, 3, (int*)&settings.FrictionModel);

		UI::PropertySlider("Solver Iterations", (int&)settings.SolverIterations, 1, 512);
		UI::PropertySlider("Solver Velocity Iterations", (int&)settings.SolverVelocityIterations, 1, 512);
	}

	void PhysicsSettingsWindow::RenderLayerList()
	{
		if (ImGui::Button("New Layer"))
		{
			ImGui::OpenPopup("NewLayerNamePopup");
		}

		if (ImGui::BeginPopup("NewLayerNamePopup"))
		{
			ImGui::InputText("##LayerNameID", s_NewLayerNameBuffer, 50);

			if (ImGui::Button("Add"))
			{
				PhysicsLayerManager::AddLayer(std::string(s_NewLayerNameBuffer));
				memset(s_NewLayerNameBuffer, 0, 50);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		uint32_t buttonId = 0;

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (ImGui::Button(layer.Name.c_str()))
			{
				s_SelectedLayer = layer.LayerID;
			}

			if (layer.Name != "Default")
			{
				ImGui::SameLine();
				ImGui::PushID(buttonId++);
				if (ImGui::Button("X"))
				{
					PhysicsLayerManager::RemoveLayer(layer.LayerID);
				}
				ImGui::PopID();
			}
		}
	}

	static std::string s_IDString = "##";
	void PhysicsSettingsWindow::RenderSelectedLayer()
	{
		if (s_SelectedLayer == -1)
			return;

		const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(s_SelectedLayer);

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			if (layer.LayerID == s_SelectedLayer)
				continue;

			const PhysicsLayer& otherLayerInfo = PhysicsLayerManager::GetLayer(layer.LayerID);
			bool shouldCollide;

			if (layerInfo.CollidesWith == 0 || otherLayerInfo.CollidesWith == 0)
			{
				shouldCollide = false;
			}
			else
			{
				shouldCollide = layerInfo.CollidesWith & otherLayerInfo.BitValue;
			}

			// NOTE(Peter): We don't use UI::Property here since the label and checkbox should be in the second column, and shouldn't be separated
			ImGui::TextUnformatted(otherLayerInfo.Name.c_str());
			ImGui::SameLine();
			if (ImGui::Checkbox((s_IDString + otherLayerInfo.Name).c_str(), &shouldCollide))
			{
				PhysicsLayerManager::SetLayerCollision(s_SelectedLayer, layer.LayerID, shouldCollide);
			}
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
		{
			s_SelectedLayer = -1;
		}
	}
}