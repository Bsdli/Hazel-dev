#include "hzpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Physics/PhysicsLayer.h"
#include "Hazel/Physics/PXPhysicsWrappers.h"
#include "Hazel/Renderer/MeshFactory.h"

#include "Hazel/Asset/AssetManager.h"

#include "yaml-cpp/yaml.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::quat>
	{
		static Node encode(const glm::quat& rhs)
		{
			Node node;
			node.push_back(rhs.w);
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::quat& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.w = node[0].as<float>();
			rhs.x = node[1].as<float>();
			rhs.y = node[2].as<float>();
			rhs.z = node[3].as<float>();
			return true;
		}
	};
}

namespace Hazel {

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}


	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.w << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	/*static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}*/

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity";
		out << YAML::Value << uuid;

		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& relationshipComponent = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << relationshipComponent.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq;

			for (auto child : relationshipComponent.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << child;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent

			auto& moduleName = entity.GetComponent<ScriptComponent>().ModuleName;
			out << YAML::Key << "ModuleName" << YAML::Value << moduleName;

			EntityInstanceData& data = ScriptEngine::GetEntityInstanceData(entity.GetSceneUUID(), uuid);
			const auto& moduleFieldMap = data.ModuleFieldMap;
			if (moduleFieldMap.find(moduleName) != moduleFieldMap.end())
			{
				const auto& fields = moduleFieldMap.at(moduleName);
				out << YAML::Key << "StoredFields" << YAML::Value;
				out << YAML::BeginSeq;
				for (const auto& [name, field] : fields)
				{
					out << YAML::BeginMap; // Field
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Type" << YAML::Value << (uint32_t)field.Type;
					out << YAML::Key << "Data" << YAML::Value;

					switch (field.Type)
					{
					case FieldType::Int:
						out << field.GetStoredValue<int>();
						break;
					case FieldType::UnsignedInt:
						out << field.GetStoredValue<uint32_t>();
						break;
					case FieldType::Float:
						out << field.GetStoredValue<float>();
						break;
					case FieldType::Vec2:
						out << field.GetStoredValue<glm::vec2>();
						break;
					case FieldType::Vec3:
						out << field.GetStoredValue<glm::vec3>();
						break;
					case FieldType::Vec4:
						out << field.GetStoredValue<glm::vec4>();
						break;
					}
					out << YAML::EndMap; // Field
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			auto mesh = entity.GetComponent<MeshComponent>().Mesh;
			if (mesh)
				out << YAML::Key << "AssetID" << YAML::Value << mesh->Handle;
			else
				out << YAML::Key << "AssetID" << YAML::Value << 0;

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;
			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera
			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Radiance" << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "LightSize" << YAML::Value << directionalLightComponent.LightSize;

			out << YAML::EndMap; // DirectionalLightComponent
		}

		if (entity.HasComponent<SkyLightComponent>())
		{
			out << YAML::Key << "SkyLightComponent";
			out << YAML::BeginMap; // SkyLightComponent

			auto& skyLightComponent = entity.GetComponent<SkyLightComponent>();
			out << YAML::Key << "EnvironmentMap" << YAML::Value << skyLightComponent.SceneEnvironment->Handle;
			out << YAML::Key << "Intensity" << YAML::Value << skyLightComponent.Intensity;
			out << YAML::Key << "Angle" << YAML::Value << skyLightComponent.Angle;

			out << YAML::EndMap; // SkyLightComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "TextureAssetPath" << YAML::Value << "path/to/asset";
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<RigidBody2DComponent>())
		{
			out << YAML::Key << "RigidBody2DComponent";
			out << YAML::BeginMap; // RigidBody2DComponent

			auto& rigidbody2DComponent = entity.GetComponent<RigidBody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbody2DComponent.BodyType;
			out << YAML::Key << "FixedRotation" << YAML::Value << rigidbody2DComponent.FixedRotation;

			out << YAML::EndMap; // RigidBody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent

			auto& boxCollider2DComponent = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxCollider2DComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << boxCollider2DComponent.Size;
			out << YAML::Key << "Density" << YAML::Value << boxCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << boxCollider2DComponent.Friction;

			out << YAML::EndMap; // BoxCollider2DComponent
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap; // CircleCollider2DComponent

			auto& circleCollider2DComponent = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << circleCollider2DComponent.Offset;
			out << YAML::Key << "Radius" << YAML::Value << circleCollider2DComponent.Radius;
			out << YAML::Key << "Density" << YAML::Value << circleCollider2DComponent.Density;
			out << YAML::Key << "Friction" << YAML::Value << circleCollider2DComponent.Friction;

			out << YAML::EndMap; // CircleCollider2DComponent
		}

		if (entity.HasComponent<RigidBodyComponent>())
		{
			out << YAML::Key << "RigidBodyComponent";
			out << YAML::BeginMap; // RigidBodyComponent

			auto& rigidbodyComponent = entity.GetComponent<RigidBodyComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbodyComponent.BodyType;
			out << YAML::Key << "Mass" << YAML::Value << rigidbodyComponent.Mass;
			out << YAML::Key << "LinearDrag" << YAML::Value << rigidbodyComponent.LinearDrag;
			out << YAML::Key << "AngularDrag" << YAML::Value << rigidbodyComponent.AngularDrag;
			out << YAML::Key << "DisableGravity" << YAML::Value << rigidbodyComponent.DisableGravity;
			out << YAML::Key << "IsKinematic" << YAML::Value << rigidbodyComponent.IsKinematic;
			out << YAML::Key << "Layer" << YAML::Value << rigidbodyComponent.Layer;

			out << YAML::Key << "Constraints";
			out << YAML::BeginMap; // Constraints

			out << YAML::Key << "LockPositionX" << YAML::Value << rigidbodyComponent.LockPositionX;
			out << YAML::Key << "LockPositionY" << YAML::Value << rigidbodyComponent.LockPositionY;
			out << YAML::Key << "LockPositionZ" << YAML::Value << rigidbodyComponent.LockPositionZ;
			out << YAML::Key << "LockRotationX" << YAML::Value << rigidbodyComponent.LockRotationX;
			out << YAML::Key << "LockRotationY" << YAML::Value << rigidbodyComponent.LockRotationY;
			out << YAML::Key << "LockRotationZ" << YAML::Value << rigidbodyComponent.LockRotationZ;

			out << YAML::EndMap;

			out << YAML::EndMap; // RigidBodyComponent
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::BeginMap; // BoxColliderComponent

			auto& boxColliderComponent = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "Offset" << YAML::Value << boxColliderComponent.Offset;
			out << YAML::Key << "Size" << YAML::Value << boxColliderComponent.Size;
			out << YAML::Key << "IsTrigger" << YAML::Value << boxColliderComponent.IsTrigger;

			if (boxColliderComponent.Material)
				out << YAML::Key << "Material" << YAML::Value << boxColliderComponent.Material->Handle;
			else
				out << YAML::Key << "Material" << YAML::Value << 0;

			out << YAML::EndMap; // BoxColliderComponent
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap; // SphereColliderComponent

			auto& sphereColliderComponent = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << sphereColliderComponent.Radius;
			out << YAML::Key << "IsTrigger" << YAML::Value << sphereColliderComponent.IsTrigger;

			if (sphereColliderComponent.Material)
				out << YAML::Key << "Material" << YAML::Value << sphereColliderComponent.Material->Handle;
			else
				out << YAML::Key << "Material" << YAML::Value << 0;

			out << YAML::EndMap; // SphereColliderComponent
		}

		if (entity.HasComponent<CapsuleColliderComponent>())
		{
			out << YAML::Key << "CapsuleColliderComponent";
			out << YAML::BeginMap; // CapsuleColliderComponent

			auto& capsuleColliderComponent = entity.GetComponent<CapsuleColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << capsuleColliderComponent.Radius;
			out << YAML::Key << "Height" << YAML::Value << capsuleColliderComponent.Height;
			out << YAML::Key << "IsTrigger" << YAML::Value << capsuleColliderComponent.IsTrigger;

			if (capsuleColliderComponent.Material)
				out << YAML::Key << "Material" << YAML::Value << capsuleColliderComponent.Material->Handle;
			else
				out << YAML::Key << "Material" << YAML::Value << 0;

			out << YAML::EndMap; // CapsuleColliderComponent
		}

		if (entity.HasComponent<MeshColliderComponent>())
		{
			out << YAML::Key << "MeshColliderComponent";
			out << YAML::BeginMap; // MeshColliderComponent

			auto& meshColliderComponent = entity.GetComponent<MeshColliderComponent>();

			if (meshColliderComponent.OverrideMesh)
				out << YAML::Key << "AssetID" << YAML::Value << meshColliderComponent.CollisionMesh->Handle;
			out << YAML::Key << "IsConvex" << YAML::Value << meshColliderComponent.IsConvex;
			out << YAML::Key << "IsTrigger" << YAML::Value << meshColliderComponent.IsTrigger;
			out << YAML::Key << "OverrideMesh" << YAML::Value << meshColliderComponent.OverrideMesh;

			if (meshColliderComponent.Material)
				out << YAML::Key << "Material" << YAML::Value << meshColliderComponent.Material->Handle;
			else
				out << YAML::Key << "Material" << YAML::Value << 0;

			out << YAML::EndMap; // MeshColliderComponent
		}

		out << YAML::EndMap; // Entity
	}

	static void SerializeEnvironment(YAML::Emitter& out, const Ref<Scene>& scene)
	{
		out << YAML::Key << "Environment";
		out << YAML::Value;
		out << YAML::BeginMap; // Environment
		out << YAML::Key << "AssetHandle" << YAML::Value << scene->GetEnvironment()->Handle;
		const auto& light = scene->GetLight();
		out << YAML::Key << "Light" << YAML::Value;
		out << YAML::BeginMap; // Light
		out << YAML::Key << "Direction" << YAML::Value << light.Direction;
		out << YAML::Key << "Radiance" << YAML::Value << light.Radiance;
		out << YAML::Key << "Multiplier" << YAML::Value << light.Multiplier;
		out << YAML::EndMap; // Light
		out << YAML::EndMap; // Environment
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << "Scene Name";

		if (m_Scene->GetEnvironment())
			SerializeEnvironment(out, m_Scene);

		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, m_Scene.Raw() };
			if (!entity || !entity.HasComponent<IDComponent>())
				return;

			SerializeEntity(out, entity);

		});
		out << YAML::EndSeq;

		out << YAML::Key << "PhysicsLayers";
		out << YAML::Value << YAML::BeginSeq;

		for (const auto& layer : PhysicsLayerManager::GetLayers())
		{
			// Never serialize the Default layer
			if (layer.LayerID == 0)
				continue;

			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << layer.Name;

			out << YAML::Key << "CollidesWith" << YAML::Value;
			out << YAML::BeginSeq;
			for (const auto& collidingLayer : PhysicsLayerManager::GetLayerCollisions(layer.LayerID))
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << collidingLayer.Name;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		HZ_CORE_ASSERT(false);
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		HZ_CORE_INFO("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				HZ_CORE_INFO("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithID(uuid, name);

				auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
				uint64_t parentHandle = entity["Parent"] ? entity["Parent"].as<uint64_t>() : 0;
				relationshipComponent.ParentHandle = parentHandle;

				auto children = entity["Children"];
				if (children)
				{
					for (auto child : children)
					{
						uint64_t childHandle = child["Handle"].as<uint64_t>();
						relationshipComponent.Children.push_back(childHandle);
					}
				}

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& transform = deserializedEntity.GetComponent<TransformComponent>();
					transform.Translation = transformComponent["Position"].as<glm::vec3>();
					auto& rotationNode = transformComponent["Rotation"];
					// Rotations used to be stored as quaternions
					if (rotationNode.size() == 4)
					{
						glm::quat rotation = transformComponent["Rotation"].as<glm::quat>();
						transform.Rotation = glm::eulerAngles(rotation);
					}
					else
					{
						HZ_CORE_ASSERT(rotationNode.size() == 3);
						transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					}
					transform.Scale = transformComponent["Scale"].as<glm::vec3>();

					HZ_CORE_INFO("  Entity Transform:");
					HZ_CORE_INFO("    Translation: {0}, {1}, {2}", transform.Translation.x, transform.Translation.y, transform.Translation.z);
					HZ_CORE_INFO("    Rotation: {0}, {1}, {2}", transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
					HZ_CORE_INFO("    Scale: {0}, {1}, {2}", transform.Scale.x, transform.Scale.y, transform.Scale.z);
				}

				auto scriptComponent = entity["ScriptComponent"];
				if (scriptComponent)
				{
					std::string moduleName = scriptComponent["ModuleName"].as<std::string>();
					deserializedEntity.AddComponent<ScriptComponent>(moduleName);

					HZ_CORE_INFO("  Script Module: {0}", moduleName);

					if (ScriptEngine::ModuleExists(moduleName))
					{
						auto storedFields = scriptComponent["StoredFields"];
						if (storedFields)
						{
							for (auto field : storedFields)
							{
								std::string name = field["Name"].as<std::string>();
								std::string typeName = field["TypeName"] ? field["TypeName"].as<std::string>() : "";
								FieldType type = (FieldType)field["Type"].as<uint32_t>();
								EntityInstanceData& data = ScriptEngine::GetEntityInstanceData(m_Scene->GetUUID(), uuid);
								auto& moduleFieldMap = data.ModuleFieldMap;
								auto& publicFields = moduleFieldMap[moduleName];
								if (publicFields.find(name) == publicFields.end())
								{
									PublicField pf = { name, typeName, type };
									publicFields.emplace(name, std::move(pf));
								}
								auto dataNode = field["Data"];
								switch (type)
								{
									case FieldType::Float:
									{
										publicFields.at(name).SetStoredValue(dataNode.as<float>());
										break;
									}
									case FieldType::Int:
									{
										publicFields.at(name).SetStoredValue(dataNode.as<int32_t>());
										break;
									}
									case FieldType::UnsignedInt:
									{
										publicFields.at(name).SetStoredValue(dataNode.as<uint32_t>());
										break;
									}
									case FieldType::String:
									{
										HZ_CORE_ASSERT(false, "Unimplemented");
										break;
									}
									case FieldType::Vec2:
									{
										publicFields.at(name).SetStoredValue(dataNode.as<glm::vec2>());
										break;
									}
									case FieldType::Vec3:
									{
										publicFields.at(name).SetStoredValue(dataNode.as<glm::vec3>());
										break;
									}
									case FieldType::Vec4:
									{
										publicFields.at(name).SetStoredValue(dataNode.as<glm::vec4>());
										break;
									}
								}
							}
						}
					}
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto& component = deserializedEntity.AddComponent<MeshComponent>();

					AssetHandle assetHandle = 0;
					if (meshComponent["AssetPath"])
						assetHandle = AssetManager::GetAssetHandleFromFilePath(meshComponent["AssetPath"].as<std::string>());
					else
						assetHandle = meshComponent["AssetID"].as<uint64_t>();

					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.Mesh = AssetManager::GetAsset<Mesh>(assetHandle);
					}
					else
					{
						component.Mesh = Ref<Asset>::Create().As<Mesh>();
						component.Mesh->Type = AssetType::Missing;

						std::string filepath = meshComponent["AssetPath"] ? meshComponent["AssetPath"].as<std::string>() : "";
						if (filepath.empty())
							HZ_CORE_ERROR("Tried to load non-existent mesh in Entity: {0}", deserializedEntity.GetUUID());
						else
							HZ_CORE_ERROR("Tried to load invalid mesh '{0}' in Entity {1}", filepath, deserializedEntity.GetUUID());
					}
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& component = deserializedEntity.AddComponent<CameraComponent>();
					auto& cameraNode = cameraComponent["Camera"];

					component.Camera = SceneCamera();
					auto& camera = component.Camera;
					if (cameraNode["ProjectionType"])
						camera.SetProjectionType((SceneCamera::ProjectionType)cameraNode["ProjectionType"].as<int>());
					if (cameraNode["PerspectiveFOV"])
						camera.SetPerspectiveVerticalFOV(cameraNode["PerspectiveFOV"].as<float>());
					if (cameraNode["PerspectiveNear"])
						camera.SetPerspectiveNearClip(cameraNode["PerspectiveNear"].as<float>());
					if (cameraNode["PerspectiveFar"])
						camera.SetPerspectiveFarClip(cameraNode["PerspectiveFar"].as<float>());
					if (cameraNode["OrthographicSize"])
						camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>());
					if (cameraNode["OrthographicNear"])
						camera.SetOrthographicNearClip(cameraNode["OrthographicNear"].as<float>());
					if (cameraNode["OrthographicFar"])
						camera.SetOrthographicFarClip(cameraNode["OrthographicFar"].as<float>());

					component.Primary = cameraComponent["Primary"].as<bool>();
				}

				auto directionalLightComponent = entity["DirectionalLightComponent"];
				if (directionalLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<DirectionalLightComponent>();
					component.Radiance = directionalLightComponent["Radiance"].as<glm::vec3>();
					component.CastShadows = directionalLightComponent["CastShadows"].as<bool>();
					component.SoftShadows = directionalLightComponent["SoftShadows"].as<bool>();
					component.LightSize = directionalLightComponent["LightSize"].as<float>();
				}

				auto skyLightComponent = entity["SkyLightComponent"];
				if (skyLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<SkyLightComponent>();

					AssetHandle assetHandle = 0;
					if (skyLightComponent["EnvironmentAssetPath"])
						assetHandle = AssetManager::GetAssetHandleFromFilePath(skyLightComponent["EnvironmentAssetPath"].as<std::string>());
					else
						assetHandle = skyLightComponent["EnvironmentMap"].as<uint64_t>();

					if (AssetManager::IsAssetHandleValid(assetHandle))
					{
						component.SceneEnvironment = AssetManager::GetAsset<Environment>(assetHandle);
					}
					else
					{
						std::string filepath = meshComponent["EnvironmentAssetPath"] ? meshComponent["EnvironmentAssetPath"].as<std::string>() : "";
						if (filepath.empty())
							HZ_CORE_ERROR("Tried to load non-existent environment map in Entity: {0}", deserializedEntity.GetUUID());
						else
							HZ_CORE_ERROR("Tried to load invalid environment map '{0}' in Entity {1}", filepath, deserializedEntity.GetUUID());
					}

					component.Intensity = skyLightComponent["Intensity"].as<float>();
					component.Angle = skyLightComponent["Angle"].as<float>();
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& component = deserializedEntity.AddComponent<SpriteRendererComponent>();
					component.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					component.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
				}

				auto rigidBody2DComponent = entity["RigidBody2DComponent"];
				if (rigidBody2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<RigidBody2DComponent>();
					component.BodyType = (RigidBody2DComponent::Type)rigidBody2DComponent["BodyType"].as<int>();
					component.FixedRotation = rigidBody2DComponent["FixedRotation"] ? rigidBody2DComponent["FixedRotation"].as<bool>() : false;
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<BoxCollider2DComponent>();
					component.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
					component.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
					component.Density = boxCollider2DComponent["Density"] ? boxCollider2DComponent["Density"].as<float>() : 1.0f;
					component.Friction = boxCollider2DComponent["Friction"] ? boxCollider2DComponent["Friction"].as<float>() : 1.0f;
				}

				auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
				if (circleCollider2DComponent)
				{
					auto& component = deserializedEntity.AddComponent<CircleCollider2DComponent>();
					component.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
					component.Radius = circleCollider2DComponent["Radius"].as<float>();
					component.Density = circleCollider2DComponent["Density"] ? circleCollider2DComponent["Density"].as<float>() : 1.0f; 
					component.Friction = circleCollider2DComponent["Friction"] ? circleCollider2DComponent["Friction"].as<float>() : 1.0f;
				}

				auto rigidBodyComponent = entity["RigidBodyComponent"];
				if (rigidBodyComponent)
				{
					auto& component = deserializedEntity.AddComponent<RigidBodyComponent>();
					component.BodyType = (RigidBodyComponent::Type)rigidBodyComponent["BodyType"].as<int>();
					component.Mass = rigidBodyComponent["Mass"].as<float>();
					component.LinearDrag = rigidBodyComponent["LinearDrag"] ? rigidBodyComponent["LinearDrag"].as<float>() : 0.0f;
					component.AngularDrag = rigidBodyComponent["AngularDrag"] ? rigidBodyComponent["AngularDrag"].as<float>() : 0.05f;
					component.DisableGravity = rigidBodyComponent["DisableGravity"] ? rigidBodyComponent["DisableGravity"].as<bool>() : false;
					component.IsKinematic = rigidBodyComponent["IsKinematic"] ? rigidBodyComponent["IsKinematic"].as<bool>() : false;
					component.Layer = rigidBodyComponent["Layer"] ? rigidBodyComponent["Layer"].as<uint32_t>() : 0;

					component.LockPositionX = rigidBodyComponent["Constraints"]["LockPositionX"].as<bool>();
					component.LockPositionY = rigidBodyComponent["Constraints"]["LockPositionY"].as<bool>();
					component.LockPositionZ = rigidBodyComponent["Constraints"]["LockPositionZ"].as<bool>();
					component.LockRotationX = rigidBodyComponent["Constraints"]["LockRotationX"].as<bool>();
					component.LockRotationY = rigidBodyComponent["Constraints"]["LockRotationY"].as<bool>();
					component.LockRotationZ = rigidBodyComponent["Constraints"]["LockRotationZ"].as<bool>();
				}

				auto boxColliderComponent = entity["BoxColliderComponent"];
				if (boxColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<BoxColliderComponent>();
					component.Offset = boxColliderComponent["Offset"].as<glm::vec3>();
					component.Size = boxColliderComponent["Size"].as<glm::vec3>();
					component.IsTrigger = boxColliderComponent["IsTrigger"] ? boxColliderComponent["IsTrigger"].as<bool>() : false;
					
					auto material = boxColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							HZ_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetUUID());
						}
					}

					component.DebugMesh = MeshFactory::CreateBox(component.Size);
				}

				auto sphereColliderComponent = entity["SphereColliderComponent"];
				if (sphereColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<SphereColliderComponent>();
					component.Radius = sphereColliderComponent["Radius"].as<float>();
					component.IsTrigger = sphereColliderComponent["IsTrigger"] ? sphereColliderComponent["IsTrigger"].as<bool>() : false;
				
					auto material = sphereColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							HZ_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetUUID());
						}
					}

					component.DebugMesh = MeshFactory::CreateSphere(component.Radius);
				}

				auto capsuleColliderComponent = entity["CapsuleColliderComponent"];
				if (capsuleColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<CapsuleColliderComponent>();
					component.Radius = capsuleColliderComponent["Radius"].as<float>();
					component.Height = capsuleColliderComponent["Height"].as<float>();
					component.IsTrigger = capsuleColliderComponent["IsTrigger"] ? capsuleColliderComponent["IsTrigger"].as<bool>() : false;

					auto material = capsuleColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							HZ_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetUUID());
						}
					}

					component.DebugMesh = MeshFactory::CreateCapsule(component.Radius, component.Height);
				}

				auto meshColliderComponent = entity["MeshColliderComponent"];
				if (meshColliderComponent)
				{
					auto& component = deserializedEntity.AddComponent<MeshColliderComponent>();
					component.IsConvex = meshColliderComponent["IsConvex"] ? meshColliderComponent["IsConvex"].as<bool>() : false;
					component.IsTrigger = meshColliderComponent["IsTrigger"] ? meshColliderComponent["IsTrigger"].as<bool>() : false;

					component.CollisionMesh = deserializedEntity.HasComponent<MeshComponent>() ? deserializedEntity.GetComponent<MeshComponent>().Mesh : nullptr;
					bool overrideMesh = meshColliderComponent["OverrideMesh"] ? meshColliderComponent["OverrideMesh"].as<bool>() : false;

					if (overrideMesh)
					{
						AssetHandle assetHandle = meshColliderComponent["AssetID"] ? meshColliderComponent["AssetID"].as<uint64_t>() : 0;

						if (AssetManager::IsAssetHandleValid(assetHandle))
							component.CollisionMesh = AssetManager::GetAsset<Mesh>(assetHandle);
						else
							overrideMesh = false;
					}

					if (component.CollisionMesh)
					{
						component.OverrideMesh = overrideMesh;

						if (component.IsConvex)
							PXPhysicsWrappers::CreateConvexMesh(component, deserializedEntity.Transform().Scale);
						else
							PXPhysicsWrappers::CreateTriangleMesh(component, deserializedEntity.Transform().Scale);
					}
					else
					{
						HZ_CORE_WARN("MeshColliderComponent in use without valid mesh!");
					}

					auto material = meshColliderComponent["Material"];
					if (material)
					{
						if (AssetManager::IsAssetHandleValid(material.as<uint64_t>()))
						{
							component.Material = AssetManager::GetAsset<PhysicsMaterial>(material.as<uint64_t>());
						}
						else
						{
							HZ_CORE_ERROR("Tried to load invalid Physics Material in Entity {0}", deserializedEntity.GetUUID());
						}
					}
				}

				// NOTE(Peter): Compatibility fix for older scenes
				auto physicsMaterialComponent = entity["PhysicsMaterialComponent"];
				if (physicsMaterialComponent)
				{
					Ref<PhysicsMaterial> material = Ref<PhysicsMaterial>::Create();
					material->StaticFriction = physicsMaterialComponent["StaticFriction"].as<float>();
					material->DynamicFriction = physicsMaterialComponent["DynamicFriction"].as<float>();
					material->Bounciness = physicsMaterialComponent["Bounciness"].as<float>();

					if (deserializedEntity.HasComponent<BoxColliderComponent>())
						deserializedEntity.GetComponent<BoxColliderComponent>().Material = material;

					if (deserializedEntity.HasComponent<SphereColliderComponent>())
						deserializedEntity.GetComponent<SphereColliderComponent>().Material = material;

					if (deserializedEntity.HasComponent<CapsuleColliderComponent>())
						deserializedEntity.GetComponent<CapsuleColliderComponent>().Material = material;

					if (deserializedEntity.HasComponent<MeshColliderComponent>())
						deserializedEntity.GetComponent<MeshColliderComponent>().Material = material;
				}
			}
		}

		auto physicsLayers = data["PhysicsLayers"];
		if (physicsLayers)
		{
			for (auto layer : physicsLayers)
			{
				PhysicsLayerManager::AddLayer(layer["Name"].as<std::string>(), false);
			}

			for (auto layer : physicsLayers)
			{
				const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layer["Name"].as<std::string>());

				auto collidesWith = layer["CollidesWith"];
				if (collidesWith)
				{
					for (auto collisionLayer : collidesWith)
					{
						const auto& otherLayer = PhysicsLayerManager::GetLayer(collisionLayer["Name"].as<std::string>());
						PhysicsLayerManager::SetLayerCollision(layerInfo.LayerID, otherLayer.LayerID, true);
					}
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		HZ_CORE_ASSERT(false);
		return false;
	}

}