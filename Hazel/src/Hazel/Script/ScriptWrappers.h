#pragma once

#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Physics/Physics.h"

#include <glm/glm.hpp>

extern "C" {
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
}

namespace Hazel { namespace Script {

	struct ScriptTransform
	{
		glm::vec3 Translation;
		glm::vec3 Rotation;
		glm::vec3 Scale;
		glm::vec3 Up, Right, Forward;
	};

	// Math
	float Hazel_Noise_PerlinNoise(float x, float y);

	// Input
	bool Hazel_Input_IsKeyPressed(KeyCode key);
	bool Hazel_Input_IsMouseButtonPressed(MouseButton button);
	void Hazel_Input_GetMousePosition(glm::vec2* outPosition);
	void Hazel_Input_SetCursorMode(CursorMode mode);
	CursorMode Hazel_Input_GetCursorMode();

	// Physics
	bool Hazel_Physics_Raycast(glm::vec3* origin, glm::vec3* direction, float maxDistance, RaycastHit* hit);
	MonoArray* Hazel_Physics_OverlapBox(glm::vec3* origin, glm::vec3* halfSize);
	MonoArray* Hazel_Physics_OverlapCapsule(glm::vec3* origin, float radius, float halfHeight);
	MonoArray* Hazel_Physics_OverlapSphere(glm::vec3* origin, float radius);
	int32_t Hazel_Physics_OverlapBoxNonAlloc(glm::vec3* origin, glm::vec3* halfSize, MonoArray* outColliders);
	int32_t Hazel_Physics_OverlapCapsuleNonAlloc(glm::vec3* origin, float radius, float halfHeight, MonoArray* outColliders);
	int32_t Hazel_Physics_OverlapSphereNonAlloc(glm::vec3* origin, float radius, MonoArray* outColliders);

	// Entity
	void Hazel_Entity_CreateComponent(uint64_t entityID, void* type);
	bool Hazel_Entity_HasComponent(uint64_t entityID, void* type);
	uint64_t Hazel_Entity_FindEntityByTag(MonoString* tag);

	void Hazel_TransformComponent_GetTransform(uint64_t entityID, ScriptTransform* outTransform);
	void Hazel_TransformComponent_SetTransform(uint64_t entityID, ScriptTransform* inTransform);

	void* Hazel_MeshComponent_GetMesh(uint64_t entityID);
	void Hazel_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh);

	void Hazel_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
	void Hazel_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity);
	void Hazel_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* velocity);

	RigidBodyComponent::Type Hazel_RigidBodyComponent_GetBodyType(uint64_t entityID);
	void Hazel_RigidBodyComponent_AddForce(uint64_t entityID, glm::vec3* force, ForceMode foceMode);
	void Hazel_RigidBodyComponent_AddTorque(uint64_t entityID, glm::vec3* torque, ForceMode forceMode);
	void Hazel_RigidBodyComponent_GetLinearVelocity(uint64_t entityID, glm::vec3* outVelocity);
	void Hazel_RigidBodyComponent_SetLinearVelocity(uint64_t entityID, glm::vec3* velocity);
	void Hazel_RigidBodyComponent_GetAngularVelocity(uint64_t entityID, glm::vec3* outVelocity);
	void Hazel_RigidBodyComponent_SetAngularVelocity(uint64_t entityID, glm::vec3* velocity);
	void Hazel_RigidBodyComponent_Rotate(uint64_t entityID, glm::vec3* rotation);
	uint32_t Hazel_RigidBodyComponent_GetLayer(uint64_t entityID);
	float Hazel_RigidBodyComponent_GetMass(uint64_t entityID);
	void Hazel_RigidBodyComponent_SetMass(uint64_t entityID, float mass);

	// Renderer
	// Texture2D
	void* Hazel_Texture2D_Constructor(uint32_t width, uint32_t height);
	void Hazel_Texture2D_Destructor(Ref<Texture2D>* _this);
	void Hazel_Texture2D_SetData(Ref<Texture2D>* _this, MonoArray* inData, int32_t count);

	// Material
	void Hazel_Material_Destructor(Ref<Material>* _this);
	void Hazel_Material_SetFloat(Ref<Material>* _this, MonoString* uniform, float value);
	void Hazel_Material_SetTexture(Ref<Material>* _this, MonoString* uniform, Ref<Texture2D>* texture);

	void Hazel_MaterialInstance_Destructor(Ref<MaterialInstance>* _this);
	void Hazel_MaterialInstance_SetFloat(Ref<MaterialInstance>* _this, MonoString* uniform, float value);
	void Hazel_MaterialInstance_SetVector3(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec3* value);
	void Hazel_MaterialInstance_SetVector4(Ref<MaterialInstance>* _this, MonoString* uniform, glm::vec4* value);
	void Hazel_MaterialInstance_SetTexture(Ref<MaterialInstance>* _this, MonoString* uniform, Ref<Texture2D>* texture);

	// Mesh
	Ref<Mesh>* Hazel_Mesh_Constructor(MonoString* filepath);
	void Hazel_Mesh_Destructor(Ref<Mesh>* _this);
	Ref<Material>* Hazel_Mesh_GetMaterial(Ref<Mesh>* inMesh);
	Ref<MaterialInstance>* Hazel_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
	int Hazel_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

	void* Hazel_MeshFactory_CreatePlane(float width, float height);
} }