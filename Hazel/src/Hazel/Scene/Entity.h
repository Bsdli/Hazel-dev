#pragma once

#include <glm/glm.hpp>

#include "Hazel/Renderer/Mesh.h"

namespace Hazel {

	class Entity
	{
	public:
		Entity();
		~Entity();

		// TODO: Move to Component
		void SetMesh(const Ref<Mesh>& mesh) { m_Mesh = mesh; }
		Ref<Mesh> GetMesh() { return m_Mesh; }

		void SetMaterial(const Ref<MaterialInstance>& material) { m_Material = material; }
		Ref<MaterialInstance> GetMaterial() { return m_Material; }

		const glm::mat4& GetTransform() const { return m_Transform; }
		glm::mat4& Transform() { return m_Transform; }
	private:
		glm::mat4 m_Transform;

		// TODO: Temp
		Ref<Mesh> m_Mesh;
		Ref<MaterialInstance> m_Material;
	};

}