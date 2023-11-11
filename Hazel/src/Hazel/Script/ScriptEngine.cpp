#include "hzpch.h"
#include "ScriptEngine.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>

#include <iostream>
#include <chrono>
#include <thread>

#include <Windows.h>
#include <winioctl.h>

#include "ScriptEngineRegistry.h"

#include "Hazel/Scene/Scene.h"

namespace Hazel {

	static MonoDomain* s_MonoDomain = nullptr;
	static std::string s_AssemblyPath;

	static ScriptModuleFieldMap s_PublicFields;

	static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc);

	struct EntityScriptClass
	{
		std::string FullName;
		std::string ClassName;
		std::string NamespaceName;

		MonoClass* Class;
		MonoMethod* OnCreateMethod;
		MonoMethod* OnDestroyMethod;
		MonoMethod* OnUpdateMethod;

		void InitClassMethods(MonoImage* image)
		{
			OnCreateMethod = GetMethod(image, FullName + ":OnCreate()");
			OnUpdateMethod = GetMethod(image, FullName + ":OnUpdate(single)");
		}
	};

	struct EntityInstance
	{
		EntityScriptClass* ScriptClass;

		uint32_t Handle;
		Scene* SceneInstance;

		MonoObject* GetInstance()
		{
			return mono_gchandle_get_target(Handle);
		}
	};

	static std::unordered_map<std::string, EntityScriptClass> s_EntityClassMap;
	static std::unordered_map<uint32_t, EntityInstance> s_EntityInstanceMap;

	MonoAssembly* LoadAssemblyFromFile(const char* filepath)
	{
		if (filepath == NULL)
		{
			return NULL;
		}

		HANDLE file = CreateFileA(filepath, FILE_READ_ACCESS, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
		{
			return NULL;
		}

		DWORD file_size = GetFileSize(file, NULL);
		if (file_size == INVALID_FILE_SIZE)
		{
			CloseHandle(file);
			return NULL;
		}

		void* file_data = malloc(file_size);
		if (file_data == NULL)
		{
			CloseHandle(file);
			return NULL;
		}

		DWORD read = 0;
		ReadFile(file, file_data, file_size, &read, NULL);
		if (file_size != read)
		{
			free(file_data);
			CloseHandle(file);
			return NULL;
		}

		MonoImageOpenStatus status;
		//MonoImage* image = mono_image_open_from_data_with_name(reinterpret_cast<char*>(file_data), file_size, 1, &status, 0, full_file_path);
		MonoImage* image = mono_image_open_from_data_full(reinterpret_cast<char*>(file_data), file_size, 1, &status, 0);
		if (status != MONO_IMAGE_OK)
		{
			return NULL;
		}
		auto assemb = mono_assembly_load_from_full(image, filepath, &status, 0);
		free(file_data);
		CloseHandle(file);
		mono_image_close(image);
		return assemb;
	}

	static void InitMono()
	{
		mono_set_assemblies_path("mono/lib");
		// mono_jit_set_trace_options("--verbose");
		auto domain = mono_jit_init("Hazel");

		char* name = (char*)"HazelRuntime";
		s_MonoDomain = mono_domain_create_appdomain(name, nullptr);
	}

	static MonoAssembly* LoadAssembly(const std::string& path)
	{
		MonoAssembly* assembly = LoadAssemblyFromFile(path.c_str()); //mono_domain_assembly_open(s_MonoDomain, path.c_str());

		if (!assembly)
			std::cout << "Could not load assembly: " << path << std::endl;
		else
			std::cout << "Successfully loaded assembly: " << path << std::endl;

		return assembly;
	}

	static MonoImage* GetAssemblyImage(MonoAssembly* assembly)
	{
		MonoImage* image = mono_assembly_get_image(assembly);
		if (!image)
			std::cout << "mono_assembly_get_image failed" << std::endl;

		return image;
	}

	static MonoClass* GetClass(MonoImage* image, const EntityScriptClass& scriptClass)
	{
		MonoClass* monoClass = mono_class_from_name(image, scriptClass.NamespaceName.c_str(), scriptClass.ClassName.c_str());
		if (!monoClass)
			std::cout << "mono_class_from_name failed" << std::endl;

		return monoClass;
	}

	static uint32_t Instantiate(EntityScriptClass& scriptClass)
	{
		MonoObject* instance = mono_object_new(s_MonoDomain, scriptClass.Class);
		if (!instance)
			std::cout << "mono_object_new failed" << std::endl;
		
		mono_runtime_object_init(instance);
		uint32_t handle = mono_gchandle_new(instance, false);
		return handle;
	}

	static MonoMethod* GetMethod(MonoImage* image, const std::string& methodDesc)
	{
		MonoMethodDesc* desc = mono_method_desc_new(methodDesc.c_str(), NULL);
		if (!desc)
			std::cout << "mono_method_desc_new failed" << std::endl;

		MonoMethod* method = mono_method_desc_search_in_image(desc, image);
		if (!method)
			std::cout << "mono_method_desc_search_in_image failed" << std::endl;

		return method;
	}

	static MonoObject* CallMethod(MonoObject* object, MonoMethod* method, void** params = nullptr)
	{
		MonoObject* pException = NULL;
		MonoObject* result = mono_runtime_invoke(method, object, params, &pException);
		return result;
	}

	static void PrintClassMethods(MonoClass* monoClass)
	{
		MonoMethod* iter;
		void* ptr = 0;
		while ((iter = mono_class_get_methods(monoClass, &ptr)) != NULL)
		{
			printf("--------------------------------\n");
			const char* name = mono_method_get_name(iter);
			MonoMethodDesc* methodDesc = mono_method_desc_from_method(iter);

			const char* paramNames = "";
			mono_method_get_param_names(iter, &paramNames);

			printf("Name: %s\n", name);
			printf("Full name: %s\n", mono_method_full_name(iter, true));
		}
	}

	static void PrintClassProperties(MonoClass* monoClass)
	{
		MonoProperty* iter;
		void* ptr = 0;
		while ((iter = mono_class_get_properties(monoClass, &ptr)) != NULL)
		{
			printf("--------------------------------\n");
			const char* name = mono_property_get_name(iter);

			printf("Name: %s\n", name);
		}
	}

	static MonoAssembly* s_AppAssembly = nullptr;
	static MonoAssembly* s_CoreAssembly = nullptr;
	MonoImage* s_AppAssemblyImage = nullptr;
	MonoImage* s_CoreAssemblyImage = nullptr;

	static MonoString* GetName()
	{
		return mono_string_new(s_MonoDomain, "Hello!");
	}

	static void LoadHazelRuntimeAssembly(const std::string& path)
	{
		if (s_AppAssembly)
		{
			mono_domain_unload(s_MonoDomain);
			mono_assembly_close(s_AppAssembly);

			char* name = (char*)"HazelRuntime";
			s_MonoDomain = mono_domain_create_appdomain(name, nullptr);

		}

		s_CoreAssembly = LoadAssembly("assets/scripts/Hazel-ScriptCore.dll");
		s_CoreAssemblyImage = GetAssemblyImage(s_CoreAssembly);

		s_AppAssembly = LoadAssembly(path);
		s_AppAssemblyImage = GetAssemblyImage(s_AppAssembly);
		ScriptEngineRegistry::RegisterAll();
	}

	void ScriptEngine::Init(const std::string& assemblyPath)
	{
		s_AssemblyPath = assemblyPath;

		InitMono();

		LoadHazelRuntimeAssembly(s_AssemblyPath);
	}

	void ScriptEngine::Shutdown()
	{
		// shutdown mono
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		EntityInstance& entityInstance = s_EntityInstanceMap[(uint32_t)entity.m_EntityHandle];
		if (entityInstance.ScriptClass->OnCreateMethod)
			CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->OnCreateMethod);
	}

	void ScriptEngine::OnUpdateEntity(uint32_t entityID, Timestep ts)
	{
		HZ_CORE_ASSERT(s_EntityInstanceMap.find(entityID) != s_EntityInstanceMap.end(), "Could not find entity in instance map!");

		auto& entity = s_EntityInstanceMap[entityID];

		if (entity.ScriptClass->OnUpdateMethod)
		{
			void* args[] = { &ts };
			CallMethod(entity.GetInstance(), entity.ScriptClass->OnUpdateMethod, args);
		}
	}

	static FieldType GetHazelFieldType(MonoType* monoType)
	{
		int type = mono_type_get_type(monoType);
		switch (type)
		{
			case MONO_TYPE_R4: return FieldType::Float;
			case MONO_TYPE_I4: return FieldType::Int;
			case MONO_TYPE_U4: return FieldType::UnsignedInt;
			case MONO_TYPE_STRING: return FieldType::String;
			case MONO_TYPE_VALUETYPE:
			{
				char* name = mono_type_get_name(monoType);
				if (strcmp(name, "Hazel.Vector2") == 0) return FieldType::Vec2;
				if (strcmp(name, "Hazel.Vector3") == 0) return FieldType::Vec3;
				if (strcmp(name, "Hazel.Vector4") == 0) return FieldType::Vec4;
			}
		}
		return FieldType::None;
	}

	void ScriptEngine::OnInitEntity(ScriptComponent& script, uint32_t entityID, uint32_t sceneID)
	{
		EntityScriptClass& scriptClass = s_EntityClassMap[script.ModuleName];
		scriptClass.FullName = script.ModuleName;
		if (script.ModuleName.find('.') != std::string::npos)
		{
			scriptClass.NamespaceName = script.ModuleName.substr(0, script.ModuleName.find_last_of('.'));
			scriptClass.ClassName = script.ModuleName.substr(script.ModuleName.find_last_of('.') + 1);
		}
		else
		{
			scriptClass.ClassName = script.ModuleName;
		}

		scriptClass.Class = GetClass(s_AppAssemblyImage, scriptClass);
		scriptClass.InitClassMethods(s_AppAssemblyImage);

		EntityInstance& entityInstance = s_EntityInstanceMap[entityID];
		entityInstance.ScriptClass = &scriptClass;
		entityInstance.Handle = Instantiate(scriptClass);

		MonoProperty* entityIDPropery = mono_class_get_property_from_name(scriptClass.Class, "EntityID");
		mono_property_get_get_method(entityIDPropery);
		MonoMethod* entityIDSetMethod = mono_property_get_set_method(entityIDPropery);
		void* param[] = { &entityID };
		CallMethod(entityInstance.GetInstance(), entityIDSetMethod, param);

		MonoProperty* sceneIDPropery = mono_class_get_property_from_name(scriptClass.Class, "SceneID");
		mono_property_get_get_method(sceneIDPropery);
		MonoMethod* sceneIDSetMethod = mono_property_get_set_method(sceneIDPropery);
		param[0] = { &sceneID };
		CallMethod(entityInstance.GetInstance(), sceneIDSetMethod, param);

		if (scriptClass.OnCreateMethod)
			CallMethod(entityInstance.GetInstance(), scriptClass.OnCreateMethod);

		// Retrieve public fields
		{
			MonoClassField* iter;
			void* ptr = 0;
			while ((iter = mono_class_get_fields(scriptClass.Class, &ptr)) != NULL)
			{
				const char* name = mono_field_get_name(iter);
				uint32_t flags = mono_field_get_flags(iter);
				if (flags & MONO_FIELD_ATTR_PUBLIC == 0)
					continue;

				MonoType* fieldType = mono_field_get_type(iter);
				FieldType hazelFieldType = GetHazelFieldType(fieldType);

				// TODO: Attributes
				MonoCustomAttrInfo* attr = mono_custom_attrs_from_field(scriptClass.Class, iter);

				auto& publicField = s_PublicFields[script.ModuleName].emplace_back(name, hazelFieldType);
				publicField.m_EntityInstance = &entityInstance;
				publicField.m_MonoClassField = iter;
				// mono_field_set_value(entityInstance.Instance, iter, )
			}
		}
	}

	const Hazel::ScriptModuleFieldMap& ScriptEngine::GetFieldMap()
	{
		return s_PublicFields;
	}

	/*Scene* ScriptEngine::GetEntityScene(uint32_t entityID)
	{

	}*/

	void PublicField::SetValue_Internal(void* value) const
	{
		mono_field_set_value(m_EntityInstance->GetInstance(), m_MonoClassField, value);
	}

	void PublicField::GetValue_Internal(void* outValue) const
	{
		mono_field_get_value(m_EntityInstance->GetInstance(), m_MonoClassField, outValue);
	}

}