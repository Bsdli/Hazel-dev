#include "hzpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"
#include "Hazel/Script/ScriptEngine.h"

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

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity";
		out << YAML::Value << uuid;

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

			auto& transform = entity.GetComponent<TransformComponent>().Transform;
			auto[pos, rot, scale] = GetTransformDecomposition(transform);
			out << YAML::Key << "Position" << YAML::Value << pos;
			out << YAML::Key << "Rotation" << YAML::Value << rot;
			out << YAML::Key << "Scale" << YAML::Value << scale;

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
			out << YAML::Key << "AssetPath" << YAML::Value << mesh->GetFilePath();

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto cameraComponent = entity.GetComponent<CameraComponent>();
			out << YAML::Key << "Camera" << YAML::Value << "some camera data...";
			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "TextureAssetPath" << YAML::Value << "path/to/asset";
			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		out << YAML::EndMap; // Entity
	}

	static void SerializeEnvironment(YAML::Emitter& out, const Ref<Scene>& scene)
	{
		out << YAML::Key << "Environment";
		out << YAML::Value;
		out << YAML::BeginMap; // Environment
		out << YAML::Key << "AssetPath" << YAML::Value << scene->GetEnvironment().FilePath;
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
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		HZ_CORE_INFO("Deserializing scene '{0}'", sceneName);

		auto environment = data["Environment"];
		if (environment)
		{
			std::string envPath = environment["AssetPath"].as<std::string>();
			m_Scene->SetEnvironment(Environment::Load(envPath));

			auto lightNode = environment["Light"];
			if (lightNode)
			{
				auto& light = m_Scene->GetLight();
				light.Direction = lightNode["Direction"].as<glm::vec3>();
				light.Radiance = lightNode["Radiance"].as<glm::vec3>();
				light.Multiplier = lightNode["Multiplier"].as<float>();
			}
		}

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

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& transform = deserializedEntity.GetComponent<TransformComponent>().Transform;
					glm::vec3 translation = transformComponent["Position"].as<glm::vec3>();
					glm::quat rotation = transformComponent["Rotation"].as<glm::quat>();
					glm::vec3 scale = transformComponent["Scale"].as<glm::vec3>();

					transform = glm::translate(glm::mat4(1.0f), translation) *
						glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);

					HZ_CORE_INFO("  Entity Transform:");
					HZ_CORE_INFO("    Translation: {0}, {1}, {2}", translation.x, translation.y, translation.z);
					HZ_CORE_INFO("    Rotation: {0}, {1}, {2}, {3}", rotation.w, rotation.x, rotation.y, rotation.z);
					HZ_CORE_INFO("    Scale: {0}, {1}, {2}", scale.x, scale.y, scale.z);
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
								FieldType type = (FieldType)field["Type"].as<uint32_t>();
								EntityInstanceData& data = ScriptEngine::GetEntityInstanceData(m_Scene->GetUUID(), uuid);
								auto& moduleFieldMap = data.ModuleFieldMap;
								auto& publicFields = moduleFieldMap[moduleName];
								if (publicFields.find(name) == publicFields.end())
								{
									PublicField pf = { name, type };
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
					std::string meshPath = meshComponent["AssetPath"].as<std::string>();
					// TEMP (because script creates mesh component...)
					if (!deserializedEntity.HasComponent<MeshComponent>())
						deserializedEntity.AddComponent<MeshComponent>(Ref<Mesh>::Create(meshPath));

					HZ_CORE_INFO("  Mesh Asset Path: {0}", meshPath);
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& component = deserializedEntity.AddComponent<CameraComponent>();
					component.Camera = SceneCamera();
					component.Primary = cameraComponent["Primary"].as<bool>();

					HZ_CORE_INFO("  Primary Camera: {0}", component.Primary);
				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& component = deserializedEntity.AddComponent<SpriteRendererComponent>();
					component.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					component.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();

					HZ_CORE_INFO("  SpriteRendererComponent present.");
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