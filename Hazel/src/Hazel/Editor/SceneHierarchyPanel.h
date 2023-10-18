#pragma once

#include "Hazel/Scene/Scene.h"

namespace Hazel {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void OnImGuiRender();
	private:
		void DrawEntityNode(Entity* entity, uint32_t& imguiEntityID, uint32_t& imguiMeshID);
		void DrawMeshNode(const Ref<Mesh>& mesh, uint32_t& imguiMeshID);
		void MeshNodeHierarchy(const Ref<Mesh>& mesh, aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
	private:
		Ref<Scene> m_Context;
		Ref<Mesh> m_SelectionContext;
	};

}