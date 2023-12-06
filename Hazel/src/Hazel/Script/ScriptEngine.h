#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Timestep.h"

#include <string>

#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/Entity.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoClass MonoClass;
}

namespace Hazel {

	enum class FieldType
	{
		None = 0, Float, Int, UnsignedInt, String, Vec2, Vec3, Vec4
	};

	const char* FieldTypeToString(FieldType type);

	struct EntityScriptClass;
	struct EntityInstance
	{
		EntityScriptClass* ScriptClass = nullptr;

		uint32_t Handle = 0;
		Scene* SceneInstance = nullptr;

		MonoObject* GetInstance();
	};

	// TODO: This needs to somehow work for strings...
	struct PublicField
	{
		std::string Name;
		FieldType Type;

		PublicField(const std::string& name, FieldType type);
		PublicField(const PublicField&) = delete;
		PublicField(PublicField&& other);
		~PublicField();

		void CopyStoredValueToRuntime();
		bool IsRuntimeAvailable() const;

		template<typename T>
		T GetStoredValue() const
		{
			T value;
			GetStoredValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetStoredValue(T value) const
		{
			SetStoredValue_Internal(&value);
		}

		template<typename T> 
		T GetRuntimeValue() const
		{
			T value;
			GetRuntimeValue_Internal(&value);
			return value;
		}

		template<typename T>
		void SetRuntimeValue(T value) const
		{
			SetRuntimeValue_Internal(&value);
		}

		void SetStoredValueRaw(void* src);
	private:
		EntityInstance* m_EntityInstance;
		MonoClassField* m_MonoClassField;
		uint8_t* m_StoredValueBuffer = nullptr;

		uint8_t* AllocateBuffer(FieldType type);
		void SetStoredValue_Internal(void* value) const;
		void GetStoredValue_Internal(void* outValue) const;
		void SetRuntimeValue_Internal(void* value) const;
		void GetRuntimeValue_Internal(void* outValue) const;

		friend class ScriptEngine;
	};

	using ScriptModuleFieldMap = std::unordered_map<std::string, std::unordered_map<std::string, PublicField>>;

	struct EntityInstanceData
	{
		EntityInstance Instance;
		ScriptModuleFieldMap ModuleFieldMap;
	};
	
	using EntityInstanceMap = std::unordered_map<UUID, std::unordered_map<UUID, EntityInstanceData>>;

	class ScriptEngine
	{
	public:
		static void Init(const std::string& assemblyPath);
		static void Shutdown();

		static void OnSceneDestruct(UUID sceneID);

		static void LoadHazelRuntimeAssembly(const std::string& path);
		static void ReloadAssembly(const std::string& path);

		static void SetSceneContext(const Ref<Scene>& scene);
		static const Ref<Scene>& GetCurrentSceneContext();

		static void CopyEntityScriptData(UUID dst, UUID src);

		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, Timestep ts);
		static void OnPhysicsUpdateEntity(Entity entity, float fixedTimeStep);

		static void OnCollision2DBegin(Entity entity);
		static void OnCollision2DEnd(Entity entity);
		static void OnCollisionBegin(Entity entity);
		static void OnCollisionEnd(Entity entity);
		static void OnTriggerBegin(Entity entity);
		static void OnTriggerEnd(Entity entity);

		static MonoObject* Construct(const std::string& fullName, bool callConstructor = true, void** parameters = nullptr);
		static MonoClass* GetCoreClass(const std::string& fullName);

		static bool IsEntityModuleValid(Entity entity);

		static void OnScriptComponentDestroyed(UUID sceneID, UUID entityID);

		static bool ModuleExists(const std::string& moduleName);
		static void InitScriptEntity(Entity entity);
		static void ShutdownScriptEntity(Entity entity, const std::string& moduleName);
		static void InstantiateEntityClass(Entity entity);

		static EntityInstanceMap& GetEntityInstanceMap();
		static EntityInstanceData& GetEntityInstanceData(UUID sceneID, UUID entityID);

		// Debug
		static void OnImGuiRender();
	};

}