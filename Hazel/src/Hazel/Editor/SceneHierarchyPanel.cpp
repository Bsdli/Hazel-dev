#include "hzpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include "Hazel/Core/Application.h"
#include "Hazel/Math/Math.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Physics/Physics.h"
#include "Hazel/Physics/PhysicsActor.h"
#include "Hazel/Physics/PhysicsLayer.h"
#include "Hazel/Physics/PXPhysicsWrappers.h"
#include "Hazel/Renderer/MeshFactory.h"

#include "Hazel/Asset/AssetManager.h"

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Hazel/ImGui/ImGui.h"
#include "Hazel/Renderer/Renderer.h"

// TODO:
// - Eventually change imgui node IDs to be entity/asset GUID

namespace Hazel {

	glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix);

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
		: m_Context(context)
	{
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
		if (m_SelectionContext && false)
		{
			// Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = m_SelectionContext.GetUUID();
			if (entityMap.find(selectedEntityID) != entityMap.end())
				m_SelectionContext = entityMap.at(selectedEntityID);
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		m_SelectionContext = entity;

		if (m_SelectionChangedCallback)
			m_SelectionChangedCallback(m_SelectionContext);
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");
		ImRect windowRect = { ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax() };

		if (m_Context)
		{
			uint32_t entityCount = 0, meshCount = 0;
			m_Context->m_Registry.each([&](auto entity)
			{
				Entity e(entity, m_Context.Raw());
				if (e.HasComponent<IDComponent>() && e.GetParentUUID() == 0)
					DrawEntityNode(e);
			});

			if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetCurrentWindow()->ID))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

				if (payload)
				{
					UUID droppedHandle = *((UUID*)payload->Data);
					Entity e = m_Context->FindEntityByUUID(droppedHandle);
					Entity previousParent = m_Context->FindEntityByUUID(e.GetParentUUID());

					if (previousParent)
					{
						auto& children = previousParent.Children();
						children.erase(std::remove(children.begin(), children.end(), droppedHandle), children.end());

						glm::mat4 parentTransform = m_Context->GetTransformRelativeToParent(previousParent);
						glm::vec3 parentTranslation, parentRotation, parentScale;
						Math::DecomposeTransform(parentTransform, parentTranslation, parentRotation, parentScale);

						e.Transform().Translation = e.Transform().Translation + parentTranslation;
					}

					e.SetParentUUID(0);
				}

				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Empty Entity"))
					{
						auto newEntity = m_Context->CreateEntity("Empty Entity");
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Camera"))
					{
						auto newEntity = m_Context->CreateEntity("Camera");
						newEntity.AddComponent<CameraComponent>();
						SetSelected(newEntity);
					}
					if (ImGui::BeginMenu("Mesh"))
					{
						if (ImGui::MenuItem("Empty Mesh"))
						{
							auto newEntity = m_Context->CreateEntity("Empty Mesh");
							newEntity.AddComponent<MeshComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Cube"))
						{
							auto newEntity = m_Context->CreateEntity("Cube");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Cube.fbx"));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Sphere"))
						{
							auto newEntity = m_Context->CreateEntity("Sphere");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Sphere.fbx"));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Capsule"))
						{
							auto newEntity = m_Context->CreateEntity("Capsule");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Capsule.fbx"));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Plane"))
						{
							auto newEntity = m_Context->CreateEntity("Plane");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Plane.fbx"));
							SetSelected(newEntity);
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Physics"))
					{
						if (ImGui::MenuItem("Rigidbody"))
						{
							auto newEntity = m_Context->CreateEntity("Rigidbody");
							newEntity.AddComponent<RigidBodyComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Box"))
						{
							auto newEntity = m_Context->CreateEntity("Cube");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Cube.fbx"));
							newEntity.AddComponent<BoxColliderComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Sphere"))
						{
							auto newEntity = m_Context->CreateEntity("Sphere");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Sphere.fbx"));
							newEntity.AddComponent<SphereColliderComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Capsule"))
						{
							auto newEntity = m_Context->CreateEntity("Capsule");
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("assets/meshes/Default/Capsule.fbx"));
							newEntity.AddComponent<CapsuleColliderComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem("Mesh"))
						{
							auto newEntity = m_Context->CreateEntity("Capsule");
							newEntity.AddComponent<MeshComponent>();
							newEntity.AddComponent<MeshColliderComponent>();
							SetSelected(newEntity);
						}
						ImGui::EndMenu();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Directional Light"))
					{
						auto newEntity = m_Context->CreateEntity("Directional Light");
						newEntity.AddComponent<DirectionalLightComponent>();
						newEntity.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Sky Light"))
					{
						auto newEntity = m_Context->CreateEntity("Sky Light");
						newEntity.AddComponent<SkyLightComponent>();
						SetSelected(newEntity);
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::End();

			ImGui::Begin("Properties");

			if (m_SelectionContext)
				DrawComponents(m_SelectionContext);
		}
		ImGui::End();

#if TODO
		ImGui::Begin("Mesh Debug");
		if (ImGui::CollapsingHeader(mesh->m_FilePath.c_str()))
		{
			if (mesh->m_IsAnimated)
			{
				if (ImGui::CollapsingHeader("Animation"))
				{
					if (ImGui::Button(mesh->m_AnimationPlaying ? "Pause" : "Play"))
						mesh->m_AnimationPlaying = !mesh->m_AnimationPlaying;

					ImGui::SliderFloat("##AnimationTime", &mesh->m_AnimationTime, 0.0f, (float)mesh->m_Scene->mAnimations[0]->mDuration);
					ImGui::DragFloat("Time Scale", &mesh->m_TimeMultiplier, 0.05f, 0.0f, 10.0f);
				}
			}
		}
		ImGui::End();
#endif
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const char* name = "Unnamed Entity";
		if (entity.HasComponent<TagComponent>())
			name = entity.GetComponent<TagComponent>().Tag.c_str();

		ImGuiTreeNodeFlags flags = (entity == m_SelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		if (entity.Children().empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		// TODO(Peter): This should probably be a function that checks that the entities components are valid
		bool missingMesh = entity.HasComponent<MeshComponent>() && (entity.GetComponent<MeshComponent>().Mesh && entity.GetComponent<MeshComponent>().Mesh->Type == AssetType::Missing);
		if (missingMesh)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));

		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
			if (m_SelectionChangedCallback)
				m_SelectionChangedCallback(m_SelectionContext);
		}

		if (missingMesh)
			ImGui::PopStyleColor();

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			UUID entityId = entity.GetUUID();
			ImGui::Text(entity.GetComponent<TagComponent>().Tag.c_str());
			ImGui::SetDragDropPayload("scene_entity_hierarchy", &entityId, sizeof(UUID));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

			if (payload)
			{
				UUID droppedHandle = *((UUID*)payload->Data);
				Entity e = m_Context->FindEntityByUUID(droppedHandle);

				if (!entity.IsDescendantOf(e))
				{
					// Remove from previous parent
					Entity previousParent = m_Context->FindEntityByUUID(e.GetParentUUID());
					if (previousParent)
					{
						auto& parentChildren = previousParent.Children();
						parentChildren.erase(std::remove(parentChildren.begin(), parentChildren.end(), droppedHandle), parentChildren.end());
					}

					glm::mat4 parentTransform = m_Context->GetTransformRelativeToParent(entity);
					glm::vec3 parentTranslation, parentRotation, parentScale;
					Math::DecomposeTransform(parentTransform, parentTranslation, parentRotation, parentScale);

					e.Transform().Translation = e.Transform().Translation - parentTranslation;
					e.SetParentUUID(entity.GetUUID());
					entity.Children().push_back(droppedHandle);
				}

			}

			ImGui::EndDragDropTarget();
		}

		if (opened)
		{
			for (auto child : entity.Children())
			{
				Entity e = m_Context->FindEntityByUUID(child);
				if (e)
					DrawEntityNode(e);
			}

			ImGui::TreePop();
		}

		// Defer deletion until end of node UI
		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (entity == m_SelectionContext)
				m_SelectionContext = {};

			m_EntityDeletedCallback(entity);
		}
	}

	void SceneHierarchyPanel::DrawMeshNode(const Ref<Mesh>& mesh, uint32_t& imguiMeshID)
	{
		static char imguiName[128];
		memset(imguiName, 0, 128);
		sprintf(imguiName, "Mesh##%d", imguiMeshID++);

		// Mesh Hierarchy
		if (ImGui::TreeNode(imguiName))
		{
			auto rootNode = mesh->m_Scene->mRootNode;
			MeshNodeHierarchy(mesh, rootNode);
			ImGui::TreePop();
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

	void SceneHierarchyPanel::MeshNodeHierarchy(const Ref<Mesh>& mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		if (ImGui::TreeNode(node->mName.C_Str()))
		{
			{
				auto [translation, rotation, scale] = GetTransformDecomposition(transform);
				ImGui::Text("World Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
			{
				auto [translation, rotation, scale] = GetTransformDecomposition(localTransform);
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}

			for (uint32_t i = 0; i < node->mNumChildren; i++)
				MeshNodeHierarchy(mesh, node->mChildren[i], transform, level + 1);

			ImGui::TreePop();
		}

	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction, bool canBeRemoved = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			// NOTE(Peter):
			//	This fixes an issue where the first "+" button would display the "Remove" buttons for ALL components on an Entity.
			//	This is due to ImGui::TreeNodeEx only pushing the id for it's children if it's actually open
			ImGui::PushID((void*)typeid(T).hash_code());
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, name.c_str());
			bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
			ImGui::PopStyleVar();

			bool resetValues = false;
			bool removeComponent = false;

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }) || right_clicked)
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Reset"))
					resetValues = true;

				if (canBeRemoved)
				{
					if (ImGui::MenuItem("Remove component"))
						removeComponent = true;
				}

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent || resetValues)
				entity.RemoveComponent<T>();

			if (resetValues)
				entity.AddComponent<T>();

			ImGui::PopID();
		}
	}

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		bool modified = false;

		const ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return modified;
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ImGui::AlignTextToFramePadding();

		auto id = entity.GetComponent<IDComponent>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, 256);
			memcpy(buffer, tag.c_str(), tag.length());
			ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
			if (ImGui::InputText("##Tag", buffer, 256))
			{
				tag = std::string(buffer);
			}
			ImGui::PopItemWidth();
		}

		// ID
		ImGui::SameLine();
		ImGui::TextDisabled("%llx", id);
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 textSize = ImGui::CalcTextSize("Add Component");
		ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPanel");

		if (ImGui::BeginPopup("AddComponentPanel"))
		{
			if (!m_SelectionContext.HasComponent<CameraComponent>())
			{
				if (ImGui::Button("Camera"))
				{
					m_SelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<MeshComponent>())
			{
				if (ImGui::Button("Mesh"))
				{
					MeshComponent& component = m_SelectionContext.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<DirectionalLightComponent>())
			{
				if (ImGui::Button("Directional Light"))
				{
					m_SelectionContext.AddComponent<DirectionalLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SkyLightComponent>())
			{
				if (ImGui::Button("Sky Light"))
				{
					m_SelectionContext.AddComponent<SkyLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<ScriptComponent>())
			{
				if (ImGui::Button("Script"))
				{
					m_SelectionContext.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SpriteRendererComponent>())
			{
				if (ImGui::Button("Sprite Renderer"))
				{
					m_SelectionContext.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<RigidBody2DComponent>())
			{
				if (ImGui::Button("Rigidbody 2D"))
				{
					m_SelectionContext.AddComponent<RigidBody2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<BoxCollider2DComponent>())
			{
				if (ImGui::Button("Box Collider 2D"))
				{
					m_SelectionContext.AddComponent<BoxCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<CircleCollider2DComponent>())
			{
				if (ImGui::Button("Circle Collider 2D"))
				{
					m_SelectionContext.AddComponent<CircleCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<RigidBodyComponent>())
			{
				if (ImGui::Button("Rigidbody"))
				{
					m_SelectionContext.AddComponent<RigidBodyComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<BoxColliderComponent>())
			{
				if (ImGui::Button("Box Collider"))
				{
					m_SelectionContext.AddComponent<BoxColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SphereColliderComponent>())
			{
				if (ImGui::Button("Sphere Collider"))
				{
					m_SelectionContext.AddComponent<SphereColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<CapsuleColliderComponent>())
			{
				if (ImGui::Button("Capsule Collider"))
				{
					m_SelectionContext.AddComponent<CapsuleColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<MeshColliderComponent>())
			{
				if (ImGui::Button("Mesh Collider"))
				{
					MeshColliderComponent& component = m_SelectionContext.AddComponent<MeshColliderComponent>();
					if (m_SelectionContext.HasComponent<MeshComponent>())
					{
						component.CollisionMesh = m_SelectionContext.GetComponent<MeshComponent>().Mesh;
						PXPhysicsWrappers::CreateTriangleMesh(component);
					}

					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, 1.0f);
		}, false);

		DrawComponent<MeshComponent>("Mesh", entity, [&](MeshComponent& mc)
		{
			UI::BeginPropertyGrid();
			if (UI::PropertyAssetReference("Mesh", mc.Mesh, AssetType::Mesh))
			{
				if (entity.HasComponent<MeshColliderComponent>())
				{
					auto& mcc = entity.GetComponent<MeshColliderComponent>();
					mcc.CollisionMesh = mc.Mesh;
					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
				}
			}
			UI::EndPropertyGrid();
		});

		DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& cc)
		{
			UI::BeginPropertyGrid();

			// Projection Type
			const char* projTypeStrings[] = { "Perspective", "Orthographic" };
			int currentProj = (int)cc.Camera.GetProjectionType();
			if (UI::PropertyDropdown("Projection", projTypeStrings, 2, &currentProj))
			{
				cc.Camera.SetProjectionType((SceneCamera::ProjectionType)currentProj);
			}

			// Perspective parameters
			if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = cc.Camera.GetPerspectiveVerticalFOV();
				if (UI::Property("Vertical FOV", verticalFOV))
					cc.Camera.SetPerspectiveVerticalFOV(verticalFOV);

				float nearClip = cc.Camera.GetPerspectiveNearClip();
				if (UI::Property("Near Clip", nearClip))
					cc.Camera.SetPerspectiveNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetPerspectiveFarClip();
				if (UI::Property("Far Clip", farClip))
					cc.Camera.SetPerspectiveFarClip(farClip);
			}

			// Orthographic parameters
			else if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = cc.Camera.GetOrthographicSize();
				if (UI::Property("Size", orthoSize))
					cc.Camera.SetOrthographicSize(orthoSize);

				float nearClip = cc.Camera.GetOrthographicNearClip();
				if (UI::Property("Near Clip", nearClip))
					cc.Camera.SetOrthographicNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetOrthographicFarClip();
				if (UI::Property("Far Clip", farClip))
					cc.Camera.SetOrthographicFarClip(farClip);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& mc)
		{
		});

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor("Radiance", dlc.Radiance);
			UI::Property("Intensity", dlc.Intensity);
			UI::Property("Cast Shadows", dlc.CastShadows);
			UI::Property("Soft Shadows", dlc.SoftShadows);
			UI::Property("Source Size", dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyAssetReference("Environment Map", slc.SceneEnvironment, AssetType::EnvMap);
			UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
			ImGui::Separator();
			UI::Property("Dynamic Sky", slc.DynamicSky);
			if (slc.DynamicSky)
			{
				bool changed = UI::Property("Turbidity", slc.TurbidityAzimuthInclination.x, 0.01f);
				changed |= UI::Property("Azimuth", slc.TurbidityAzimuthInclination.y, 0.01f);
				changed |= UI::Property("Inclination", slc.TurbidityAzimuthInclination.z, 0.01f);
				if (changed)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(slc.TurbidityAzimuthInclination.x, slc.TurbidityAzimuthInclination.y, slc.TurbidityAzimuthInclination.z);
					slc.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
				}
			}
			UI::EndPropertyGrid();
		});

		DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
		{
			UI::BeginPropertyGrid();
			std::string oldName = sc.ModuleName;
			if (UI::Property("Module Name", sc.ModuleName, !ScriptEngine::ModuleExists(sc.ModuleName))) // TODO: no live edit
			{
				// Shutdown old script
				if (ScriptEngine::ModuleExists(oldName))
					ScriptEngine::ShutdownScriptEntity(entity, oldName);

				if (ScriptEngine::ModuleExists(sc.ModuleName))
					ScriptEngine::InitScriptEntity(entity);
			}

			// Public Fields
			if (ScriptEngine::ModuleExists(sc.ModuleName))
			{
				EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneUUID(), id);
				auto& moduleFieldMap = entityInstanceData.ModuleFieldMap;
				if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
				{
					auto& publicFields = moduleFieldMap.at(sc.ModuleName);
					for (auto& [name, field] : publicFields)
					{
						bool isRuntime = m_Context->m_IsPlaying && field.IsRuntimeAvailable();
						switch (field.Type)
						{
						case FieldType::Int:
						{
							int value = isRuntime ? field.GetRuntimeValue<int>() : field.GetStoredValue<int>();
							if (UI::Property(field.Name.c_str(), value))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Float:
						{
							float value = isRuntime ? field.GetRuntimeValue<float>() : field.GetStoredValue<float>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec2:
						{
							glm::vec2 value = isRuntime ? field.GetRuntimeValue<glm::vec2>() : field.GetStoredValue<glm::vec2>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec3:
						{
							glm::vec3 value = isRuntime ? field.GetRuntimeValue<glm::vec3>() : field.GetStoredValue<glm::vec3>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec4:
						{
							glm::vec4 value = isRuntime ? field.GetRuntimeValue<glm::vec4>() : field.GetStoredValue<glm::vec4>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						/*case FieldType::ClassReference:
						{
							Ref<Asset>* asset = (Ref<Asset>*)(isRuntime ? field.GetRuntimeValueRaw() : field.GetStoredValueRaw());
							std::string label = field.Name + "(" + field.TypeName + ")";

							if (!AssetManager::IsAssetHandleValid((*asset)->Handle))
								break;

							if (UI::PropertyAssetReference(label.c_str(), *asset))
							{
								if (isRuntime)
									field.SetRuntimeValueRaw(asset);
								else
									field.SetStoredValueRaw(asset);
							}
							break;
						}*/
						}
					}
				}
			}

			UI::EndPropertyGrid();
#if TODO
			if (ImGui::Button("Run Script"))
			{
				ScriptEngine::OnCreateEntity(entity);
			}
#endif
		});

		DrawComponent<RigidBody2DComponent>("Rigidbody 2D", entity, [](RigidBody2DComponent& rb2dc)
		{
			UI::BeginPropertyGrid();

			// Rigidbody2D Type
			const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			UI::PropertyDropdown("Type", rb2dTypeStrings, 3, (int*)&rb2dc.BodyType);

			if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Fixed Rotation", rb2dc.FixedRotation);
				UI::EndPropertyGrid();
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", bc2dc.Offset);
			UI::Property("Size", bc2dc.Size);
			UI::Property("Density", bc2dc.Density);
			UI::Property("Friction", bc2dc.Friction);

			UI::EndPropertyGrid();
		});
	
		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", cc2dc.Offset);
			UI::Property("Radius", cc2dc.Radius);
			UI::Property("Density", cc2dc.Density);
			UI::Property("Friction", cc2dc.Friction);

			UI::EndPropertyGrid();
		});

		DrawComponent<RigidBodyComponent>("Rigidbody", entity, [](RigidBodyComponent& rbc)
		{
			UI::BeginPropertyGrid();

			// Rigidbody Type
			const char* rbTypeStrings[] = { "Static", "Dynamic" };
			UI::PropertyDropdown("Type", rbTypeStrings, 2, (int*)&rbc.BodyType);

			// Layer has been removed, set to Default layer
			if (!PhysicsLayerManager::IsLayerValid(rbc.Layer))
				rbc.Layer = 0;

			int layerCount = PhysicsLayerManager::GetLayerCount();
			const auto& layerNames = PhysicsLayerManager::GetLayerNames();
			UI::PropertyDropdown("Layer", layerNames, layerCount, (int*)&rbc.Layer);

			if (rbc.BodyType == RigidBodyComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Mass", rbc.Mass);
				UI::Property("Linear Drag", rbc.LinearDrag);
				UI::Property("Angular Drag", rbc.AngularDrag);
				UI::Property("Disable Gravity", rbc.DisableGravity);
				UI::Property("Is Kinematic", rbc.IsKinematic);
				UI::EndPropertyGrid();

				if (UI::BeginTreeNode("Constraints", false))
				{
					UI::BeginPropertyGrid();

					UI::BeginCheckboxGroup("Freeze Position");
					UI::PropertyCheckboxGroup("X", rbc.LockPositionX);
					UI::PropertyCheckboxGroup("Y", rbc.LockPositionY);
					UI::PropertyCheckboxGroup("Z", rbc.LockPositionZ);
					UI::EndCheckboxGroup();

					UI::BeginCheckboxGroup("Freeze Rotation");
					UI::PropertyCheckboxGroup("X", rbc.LockRotationX);
					UI::PropertyCheckboxGroup("Y", rbc.LockRotationY);
					UI::PropertyCheckboxGroup("Z", rbc.LockRotationZ);
					UI::EndCheckboxGroup();
					
					UI::EndPropertyGrid();
					UI::EndTreeNode();
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<BoxColliderComponent>("Box Collider", entity, [](BoxColliderComponent& bcc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Size", bcc.Size))
				bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);

			//Property("Offset", bcc.Offset);
			UI::Property("Is Trigger", bcc.IsTrigger);
			UI::PropertyAssetReference("Material", bcc.Material, AssetType::PhysicsMat);

			UI::EndPropertyGrid();
		});

		DrawComponent<SphereColliderComponent>("Sphere Collider", entity, [](SphereColliderComponent& scc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Radius", scc.Radius))
			{
				scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
			}

			UI::Property("Is Trigger", scc.IsTrigger);
			UI::PropertyAssetReference("Material", scc.Material, AssetType::PhysicsMat);

			UI::EndPropertyGrid();
		});

		DrawComponent<CapsuleColliderComponent>("Capsule Collider", entity, [=](CapsuleColliderComponent& ccc)
		{
			UI::BeginPropertyGrid();

			bool changed = false;

			if (UI::Property("Radius", ccc.Radius))
				changed = true;

			if (UI::Property("Height", ccc.Height))
				changed = true;

			UI::Property("Is Trigger", ccc.IsTrigger);
			UI::PropertyAssetReference("Material", ccc.Material, AssetType::PhysicsMat);

			if (changed)
			{
				ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<MeshColliderComponent>("Mesh Collider", entity, [&](MeshColliderComponent& mcc)
		{
			UI::BeginPropertyGrid();

			if (mcc.OverrideMesh)
			{
				if (UI::PropertyAssetReference("Mesh", mcc.CollisionMesh, AssetType::Mesh))
				{
					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
				}
			}

			if (UI::Property("Is Convex", mcc.IsConvex))
			{
				if (mcc.IsConvex)
					PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
				else
					PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
			}

			UI::Property("Is Trigger", mcc.IsTrigger);
			UI::PropertyAssetReference("Material", mcc.Material, AssetType::PhysicsMat);

			if (UI::Property("Override Mesh", mcc.OverrideMesh))
			{
				if (!mcc.OverrideMesh && entity.HasComponent<MeshComponent>())
				{
					mcc.CollisionMesh = entity.GetComponent<MeshComponent>().Mesh;

					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
				}
			}
			UI::EndPropertyGrid();
		});

	}

}
