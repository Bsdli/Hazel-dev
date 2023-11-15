#pragma once

#include <glm/glm.hpp>

#include "Scene.h"
#include "Hazel/Renderer/Mesh.h"

#include "Components.h"

namespace Hazel {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle), m_Scene(scene) {}

		~Entity() {}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		glm::mat4& Transform() { return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle); }
		const glm::mat4& Transform() const { return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle); }

		operator uint32_t () const { return (uint32_t)m_EntityHandle; }
		operator entt::entity () const { return m_EntityHandle; }
		operator bool () const { return (uint32_t)m_EntityHandle && m_Scene; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		UUID GetSceneUUID() { return m_Scene->GetUUID(); }
	private:
		Entity(const std::string& name);
	private:
		entt::entity m_EntityHandle;
		Scene* m_Scene = nullptr;

		friend class Scene;
		friend class SceneSerializer;
		friend class ScriptEngine;
	};

}