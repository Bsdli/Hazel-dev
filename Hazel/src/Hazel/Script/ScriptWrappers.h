#pragma once

#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Core/KeyCodes.h"

#include <glm/glm.hpp>

extern "C" {
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
}

namespace Hazel { namespace Script {

	// Math
	float Hazel_Noise_PerlinNoise(float x, float y);

	// Input
	bool Hazel_Input_IsKeyPressed(KeyCode key);

	// Entity
	void Hazel_Entity_GetTransform(uint64_t entityID, glm::mat4* outTransform);
	void Hazel_Entity_SetTransform(uint64_t entityID, glm::mat4* inTransform);
	void Hazel_Entity_CreateComponent(uint64_t entityID, void* type);
	bool Hazel_Entity_HasComponent(uint64_t entityID, void* type);

	void* Hazel_MeshComponent_GetMesh(uint64_t entityID);
	void Hazel_MeshComponent_SetMesh(uint64_t entityID, Ref<Mesh>* inMesh);

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
	void Hazel_MaterialInstance_SetTexture(Ref<MaterialInstance>* _this, MonoString* uniform, Ref<Texture2D>* texture);

	// Mesh
	Ref<Mesh>* Hazel_Mesh_Constructor(MonoString* filepath);
	void Hazel_Mesh_Destructor(Ref<Mesh>* _this);
	Ref<Material>* Hazel_Mesh_GetMaterial(Ref<Mesh>* inMesh);
	Ref<MaterialInstance>* Hazel_Mesh_GetMaterialByIndex(Ref<Mesh>* inMesh, int index);
	int Hazel_Mesh_GetMaterialCount(Ref<Mesh>* inMesh);

	void* Hazel_MeshFactory_CreatePlane(float width, float height);
} }