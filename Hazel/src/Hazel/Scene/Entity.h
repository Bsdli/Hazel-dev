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
			HZ_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			HZ_CORE_ASSERT(HasComponent<T>(), "Entity doesn't have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			HZ_CORE_ASSERT(HasComponent<T>(), "Entity doesn't have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		TransformComponent& Transform() { return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle); }
		const glm::mat4& Transform() const { return m_Scene->m_Registry.get<TransformComponent>(m_EntityHandle).GetTransform(); }

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

		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().ParentHandle = parent; }
		UUID GetParentUUID() { return GetComponent<RelationshipComponent>().ParentHandle; }
		std::vector<UUID>& Children() { return GetComponent<RelationshipComponent>().Children; }

		bool HasParent() { return m_Scene->FindEntityByUUID(GetParentUUID()); }

		bool IsAncesterOf(Entity entity)
		{
			const auto& children = Children();

			if (children.size() == 0)
				return false;

			for (UUID child : children)
			{
				if (child == entity.GetUUID())
					return true;
			}

			for (UUID child : children)
			{
				if (m_Scene->FindEntityByUUID(child).IsAncesterOf(entity))
					return true;
			}

			return false;
		}

		bool IsDescendantOf(Entity entity)
		{
			return entity.IsAncesterOf(*this);
		}

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		UUID GetSceneUUID() { return m_Scene->GetUUID(); }
	private:
		Entity(const std::string& name);
	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;

		friend class Scene;
		friend class SceneSerializer;
		friend class ScriptEngine;
	};

}