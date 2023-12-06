#include "hzpch.h"
#include "ScriptEngineRegistry.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Hazel/Scene/Entity.h"
#include "ScriptWrappers.h"

namespace Hazel {

	std::unordered_map<MonoType*, std::function<bool(Entity&)>> s_HasComponentFuncs;
	std::unordered_map<MonoType*, std::function<void(Entity&)>> s_CreateComponentFuncs;

	extern MonoImage* s_CoreAssemblyImage;

#define Component_RegisterType(Type) \
	{\
		MonoType* type = mono_reflection_type_from_name("Hazel." #Type, s_CoreAssemblyImage);\
		if (type) {\
			uint32_t id = mono_type_get_type(type);\
			s_HasComponentFuncs[type] = [](Entity& entity) { return entity.HasComponent<Type>(); };\
			s_CreateComponentFuncs[type] = [](Entity& entity) { entity.AddComponent<Type>(); };\
		} else {\
			HZ_CORE_ERROR("No C# component class found for " #Type "!");\
		}\
	}

	static void InitComponentTypes()
	{
		Component_RegisterType(TagComponent);
		Component_RegisterType(TransformComponent);
		Component_RegisterType(MeshComponent);
		Component_RegisterType(ScriptComponent);
		Component_RegisterType(CameraComponent);
		Component_RegisterType(SpriteRendererComponent);
		Component_RegisterType(RigidBody2DComponent);
		Component_RegisterType(BoxCollider2DComponent);
		Component_RegisterType(RigidBodyComponent);
		Component_RegisterType(BoxColliderComponent);
		Component_RegisterType(SphereColliderComponent);
	}

	void ScriptEngineRegistry::RegisterAll()
	{
		InitComponentTypes();

		mono_add_internal_call("Hazel.Noise::PerlinNoise_Native", Hazel::Script::Hazel_Noise_PerlinNoise);

		mono_add_internal_call("Hazel.Physics::Raycast_Native", Hazel::Script::Hazel_Physics_Raycast);
		mono_add_internal_call("Hazel.Physics::OverlapBox_Native", Hazel::Script::Hazel_Physics_OverlapBox);
		mono_add_internal_call("Hazel.Physics::OverlapCapsule_Native", Hazel::Script::Hazel_Physics_OverlapCapsule);
		mono_add_internal_call("Hazel.Physics::OverlapSphere_Native", Hazel::Script::Hazel_Physics_OverlapSphere);
		mono_add_internal_call("Hazel.Physics::OverlapBoxNonAlloc_Native", Hazel::Script::Hazel_Physics_OverlapBoxNonAlloc);
		mono_add_internal_call("Hazel.Physics::OverlapCapsuleNonAlloc_Native", Hazel::Script::Hazel_Physics_OverlapCapsuleNonAlloc);
		mono_add_internal_call("Hazel.Physics::OverlapSphereNonAlloc_Native", Hazel::Script::Hazel_Physics_OverlapSphereNonAlloc);

		mono_add_internal_call("Hazel.Entity::CreateComponent_Native", Hazel::Script::Hazel_Entity_CreateComponent);
		mono_add_internal_call("Hazel.Entity::HasComponent_Native", Hazel::Script::Hazel_Entity_HasComponent);
		mono_add_internal_call("Hazel.Entity::FindEntityByTag_Native", Hazel::Script::Hazel_Entity_FindEntityByTag);

		mono_add_internal_call("Hazel.TransformComponent::GetTransform_Native", Hazel::Script::Hazel_TransformComponent_GetTransform);
		mono_add_internal_call("Hazel.TransformComponent::SetTransform_Native", Hazel::Script::Hazel_TransformComponent_SetTransform);

		mono_add_internal_call("Hazel.MeshComponent::GetMesh_Native", Hazel::Script::Hazel_MeshComponent_GetMesh);
		mono_add_internal_call("Hazel.MeshComponent::SetMesh_Native", Hazel::Script::Hazel_MeshComponent_SetMesh);

		mono_add_internal_call("Hazel.RigidBody2DComponent::ApplyLinearImpulse_Native", Hazel::Script::Hazel_RigidBody2DComponent_ApplyLinearImpulse);
		mono_add_internal_call("Hazel.RigidBody2DComponent::GetLinearVelocity_Native", Hazel::Script::Hazel_RigidBody2DComponent_GetLinearVelocity);
		mono_add_internal_call("Hazel.RigidBody2DComponent::SetLinearVelocity_Native", Hazel::Script::Hazel_RigidBody2DComponent_SetLinearVelocity);

		mono_add_internal_call("Hazel.RigidBodyComponent::GetBodyType_Native", Hazel::Script::Hazel_RigidBodyComponent_GetBodyType);
		mono_add_internal_call("Hazel.RigidBodyComponent::AddForce_Native", Hazel::Script::Hazel_RigidBodyComponent_AddForce);
		mono_add_internal_call("Hazel.RigidBodyComponent::AddTorque_Native", Hazel::Script::Hazel_RigidBodyComponent_AddTorque);
		mono_add_internal_call("Hazel.RigidBodyComponent::GetLinearVelocity_Native", Hazel::Script::Hazel_RigidBodyComponent_GetLinearVelocity);
		mono_add_internal_call("Hazel.RigidBodyComponent::SetLinearVelocity_Native", Hazel::Script::Hazel_RigidBodyComponent_SetLinearVelocity);
		mono_add_internal_call("Hazel.RigidBodyComponent::GetAngularVelocity_Native", Hazel::Script::Hazel_RigidBodyComponent_GetAngularVelocity);
		mono_add_internal_call("Hazel.RigidBodyComponent::SetAngularVelocity_Native", Hazel::Script::Hazel_RigidBodyComponent_SetAngularVelocity);
		mono_add_internal_call("Hazel.RigidBodyComponent::Rotate_Native", Hazel::Script::Hazel_RigidBodyComponent_Rotate);
		mono_add_internal_call("Hazel.RigidBodyComponent::GetLayer_Native", Hazel::Script::Hazel_RigidBodyComponent_GetLayer);
		mono_add_internal_call("Hazel.RigidBodyComponent::GetMass_Native", Hazel::Script::Hazel_RigidBodyComponent_GetMass);
		mono_add_internal_call("Hazel.RigidBodyComponent::SetMass_Native", Hazel::Script::Hazel_RigidBodyComponent_SetMass);

		mono_add_internal_call("Hazel.Input::IsKeyPressed_Native", Hazel::Script::Hazel_Input_IsKeyPressed);
		mono_add_internal_call("Hazel.Input::IsMouseButtonPressed_Native", Hazel::Script::Hazel_Input_IsMouseButtonPressed);
		mono_add_internal_call("Hazel.Input::GetMousePosition_Native", Hazel::Script::Hazel_Input_GetMousePosition);
		mono_add_internal_call("Hazel.Input::SetCursorMode_Native", Hazel::Script::Hazel_Input_SetCursorMode);
		mono_add_internal_call("Hazel.Input::GetCursorMode_Native", Hazel::Script::Hazel_Input_GetCursorMode);

		mono_add_internal_call("Hazel.Texture2D::Constructor_Native", Hazel::Script::Hazel_Texture2D_Constructor);
		mono_add_internal_call("Hazel.Texture2D::Destructor_Native", Hazel::Script::Hazel_Texture2D_Destructor);
		mono_add_internal_call("Hazel.Texture2D::SetData_Native", Hazel::Script::Hazel_Texture2D_SetData);

		mono_add_internal_call("Hazel.Material::Destructor_Native", Hazel::Script::Hazel_Material_Destructor);
		mono_add_internal_call("Hazel.Material::SetFloat_Native", Hazel::Script::Hazel_Material_SetFloat);
		mono_add_internal_call("Hazel.Material::SetTexture_Native", Hazel::Script::Hazel_Material_SetTexture);

		mono_add_internal_call("Hazel.MaterialInstance::Destructor_Native", Hazel::Script::Hazel_MaterialInstance_Destructor);
		mono_add_internal_call("Hazel.MaterialInstance::SetFloat_Native", Hazel::Script::Hazel_MaterialInstance_SetFloat);
		mono_add_internal_call("Hazel.MaterialInstance::SetVector3_Native", Hazel::Script::Hazel_MaterialInstance_SetVector3);
		mono_add_internal_call("Hazel.MaterialInstance::SetVector4_Native", Hazel::Script::Hazel_MaterialInstance_SetVector4);
		mono_add_internal_call("Hazel.MaterialInstance::SetTexture_Native", Hazel::Script::Hazel_MaterialInstance_SetTexture);

		mono_add_internal_call("Hazel.Mesh::Constructor_Native", Hazel::Script::Hazel_Mesh_Constructor);
		mono_add_internal_call("Hazel.Mesh::Destructor_Native", Hazel::Script::Hazel_Mesh_Destructor);
		mono_add_internal_call("Hazel.Mesh::GetMaterial_Native", Hazel::Script::Hazel_Mesh_GetMaterial);
		mono_add_internal_call("Hazel.Mesh::GetMaterialByIndex_Native", Hazel::Script::Hazel_Mesh_GetMaterialByIndex);
		mono_add_internal_call("Hazel.Mesh::GetMaterialCount_Native", Hazel::Script::Hazel_Mesh_GetMaterialCount);

		mono_add_internal_call("Hazel.MeshFactory::CreatePlane_Native", Hazel::Script::Hazel_MeshFactory_CreatePlane);
	}

}