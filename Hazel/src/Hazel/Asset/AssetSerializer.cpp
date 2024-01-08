#include "hzpch.h"
#include "AssetSerializer.h"
#include "Hazel/Utilities/StringUtils.h"
#include "Hazel/Utilities/FileSystem.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "yaml-cpp/yaml.h"

namespace Hazel {

	void AssetSerializer::CopyMetadata(const Ref<Asset>& from, Ref<Asset>& to) const
	{
		to->Handle = from->Handle;
		to->FilePath = from->FilePath;
		to->FileName = from->FileName;
		to->Extension = from->Extension;
		to->ParentDirectory = from->ParentDirectory;
		to->Type = from->Type;
		to->IsDataLoaded = true;
	}

	bool TextureSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Texture2D::Create(asset->FilePath);
		CopyMetadata(temp, asset);
		return (asset.As<Texture2D>())->Loaded();
	}

	bool MeshSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Ref<Mesh>::Create(asset->FilePath);
		CopyMetadata(temp, asset);
		return (asset.As<Mesh>())->GetStaticVertices().size() > 0; // Maybe?
	}

	bool EnvironmentSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(asset->FilePath);

		if (!radiance || !irradiance)
			return false;

		Ref<Asset> temp = asset;
		asset = Ref<Environment>::Create(radiance, irradiance);
		CopyMetadata(temp, asset);
		return true;
	}

	void PhysicsMaterialSerializer::Serialize(const Ref<Asset>& asset) const
	{
		Ref<PhysicsMaterial> material = asset.As<PhysicsMaterial>();

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "StaticFriction" << material->StaticFriction;
		out << YAML::Key << "DynamicFriction" << material->DynamicFriction;
		out << YAML::Key << "Bounciness" << material->Bounciness;
		out << YAML::EndMap;

		std::ofstream fout(asset->FilePath);
		fout << out.c_str();
	}

	bool PhysicsMaterialSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		std::ifstream stream(asset->FilePath);
		if (!stream.is_open())
			return false;

		Ref<Asset> temp = asset;
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		float staticFriction = data["StaticFriction"].as<float>();
		float dynamicFriction = data["DynamicFriction"].as<float>();
		float bounciness = data["Bounciness"].as<float>();

		asset = Ref<PhysicsMaterial>::Create(staticFriction, dynamicFriction, bounciness);
		CopyMetadata(temp, asset);
		return true;
	}

}