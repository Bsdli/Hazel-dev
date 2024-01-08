#include "hzpch.h"
#include "AssetManager.h"

#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "yaml-cpp/yaml.h"

#include <filesystem>

namespace Hazel {

	void AssetManager::Init()
	{
		AssetImporter::Init();

		LoadAssetRegistry();
		FileSystem::SetChangeCallback(AssetManager::OnFileSystemChanged);
		ReloadAssets();
		UpdateRegistryCache();
	}

	void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
	{
		s_AssetsChangeCallback = callback;
	}

	void AssetManager::Shutdown()
	{
		s_AssetRegistry.clear();
		s_LoadedAssets.clear();
	}

	std::vector<Ref<Asset>> AssetManager::GetAssetsInDirectory(AssetHandle directoryHandle)
	{
		std::vector<Ref<Asset>> results;

		for (auto& asset : s_LoadedAssets)
		{
			if (asset.second && asset.second->ParentDirectory == directoryHandle && asset.second->Handle != directoryHandle)
				results.push_back(asset.second);
		}

		return results;
	}

	// Utility function to find the parent of an unprocessed directory
	AssetHandle AssetManager::FindParentHandleInChildren(Ref<Directory>& dir, const std::string& dirName)
	{
		if (dir->FileName == dirName)
			return dir->Handle;

		for (AssetHandle childHandle : dir->ChildDirectories)
		{
			Ref<Directory> child = GetAsset<Directory>(childHandle);
			AssetHandle dirHandle = FindParentHandleInChildren(child, dirName);

			if (IsAssetHandleValid(dirHandle))
				return dirHandle;
		}

		return 0;
	}

	AssetHandle AssetManager::FindParentHandle(const std::string& filepath)
	{
		std::vector<std::string> parts = Utils::SplitString(filepath, "/\\");
		std::string parentFolder = parts[parts.size() - 2];
		Ref<Directory>& assetsDirectory = GetAsset<Directory>(GetAssetHandleFromFilePath("assets"));
		return FindParentHandleInChildren(assetsDirectory, parentFolder);
	}

	void AssetManager::OnFileSystemChanged(FileSystemChangedEvent e)
	{
		e.NewName = Utils::RemoveExtension(e.NewName);
		e.OldName = Utils::RemoveExtension(e.OldName);

		AssetHandle parentHandle = FindParentHandle(e.FilePath);

		if (e.Action == FileSystemAction::Added)
		{
			if (e.IsDirectory)
				ProcessDirectory(e.FilePath, parentHandle);
			else
				ImportAsset(e.FilePath, parentHandle);
		}

		// TODO: Re import data if loaded
		/*if (e.Action == FileSystemAction::Modified)
		{
			if (!e.IsDirectory)
				ImportAsset(e.FilePath, parentHandle);
		}*/

		if (e.Action == FileSystemAction::Rename)
		{
			for (auto it = s_LoadedAssets.begin(); it != s_LoadedAssets.end(); it++)
			{
				if (it->second->FileName == e.OldName)
				{
					it->second->FilePath = e.FilePath;
					it->second->FileName = e.NewName;
				}
			}
		}

		if (e.Action == FileSystemAction::Delete)
		{
			for (auto it = s_LoadedAssets.begin(); it != s_LoadedAssets.end(); it++)
			{
				if (it->second->FilePath != e.FilePath)
					continue;

				RemoveAsset(it->first);
				break;
			}
		}

		s_AssetsChangeCallback();
	}

	std::vector<Ref<Asset>> AssetManager::SearchAssets(const std::string& query, const std::string& searchPath, AssetType desiredType)
	{
		std::vector<Ref<Asset>> results;

		if (!searchPath.empty())
		{
			for (const auto& [key, asset] : s_LoadedAssets)
			{
				if (desiredType == AssetType::None && asset->Type == AssetType::Directory)
					continue;

				if (desiredType != AssetType::None && asset->Type != desiredType)
					continue;

				if (asset->FileName.find(query) != std::string::npos && asset->FilePath.find(searchPath) != std::string::npos)
				{
					results.push_back(asset);
				}

				// Search extensions
				if (query[0] == '.')
				{
					if (asset->Extension.find(std::string(&query[1])) != std::string::npos && asset->FilePath.find(searchPath) != std::string::npos)
					{
						results.push_back(asset);
					}
				}
			}
		}

		return results;
	}

	bool AssetManager::IsDirectory(const std::string& filepath)
	{
		for (auto& [handle, asset] : s_LoadedAssets)
		{
			if (asset->Type == AssetType::Directory && asset->FilePath == filepath)
				return true;
		}

		return false;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::string& filepath)
	{
		std::string fixedFilepath = filepath;
		std::replace(fixedFilepath.begin(), fixedFilepath.end(), '\\', '/');
		for (auto& [id, asset] : s_LoadedAssets)
		{
			if (asset->FilePath == fixedFilepath)
				return id;
		}

		return 0;
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return assetHandle != 0 && s_LoadedAssets.find(assetHandle) != s_LoadedAssets.end();
	}

	void AssetManager::Rename(AssetHandle assetHandle, const std::string& newName)
	{
		Ref<Asset>& asset = s_LoadedAssets[assetHandle];
		AssetMetadata& metadata = s_AssetRegistry[asset->FilePath];
		std::string newFilePath = FileSystem::Rename(asset->FilePath, newName);
		asset->FilePath = newFilePath;
		asset->FileName = newName;
		metadata.FilePath = newFilePath;
		UpdateRegistryCache();
	}

	void AssetManager::RemoveAsset(AssetHandle assetHandle)
	{
		Ref<Asset> asset = s_LoadedAssets[assetHandle];
		if (asset->Type == AssetType::Directory)
		{
			if (IsAssetHandleValid(asset->ParentDirectory))
			{
				auto& childList = s_LoadedAssets[asset->ParentDirectory].As<Directory>()->ChildDirectories;
				childList.erase(std::remove(childList.begin(), childList.end(), assetHandle), childList.end());
			}

			for (auto child : asset.As<Directory>()->ChildDirectories)
				RemoveAsset(child);

			for (auto it = s_LoadedAssets.begin(); it != s_LoadedAssets.end(); )
			{
				if (it->second->ParentDirectory != assetHandle)
				{
					it++;
					continue;
				}

				s_AssetRegistry.erase(it->second->FilePath);
				it = s_LoadedAssets.erase(it);
			}
		}

		s_AssetRegistry.erase(asset->FilePath);
		s_LoadedAssets.erase(assetHandle);

		UpdateRegistryCache();
	}

	AssetType AssetManager::GetAssetTypeForFileType(const std::string& extension)
	{
		if (extension == "hsc") return AssetType::Scene;
		if (extension == "fbx") return AssetType::Mesh;
		if (extension == "obj") return AssetType::Mesh;
		if (extension == "png") return AssetType::Texture;
		if (extension == "hdr") return AssetType::EnvMap;
		if (extension == "hpm") return AssetType::PhysicsMat;
		if (extension == "wav") return AssetType::Audio;
		if (extension == "ogg") return AssetType::Audio;
		if (extension == "cs")  return AssetType::Script;
		return AssetType::None;
	}

	void AssetManager::LoadAssetRegistry()
	{
		if (!FileSystem::Exists("assets/cache/AssetRegistryCache.hzr"))
			return;

		std::ifstream stream("assets/cache/AssetRegistryCache.hzr");
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["Assets"];
		if (!handles)
		{
			HZ_CORE_ERROR("Failed to read Asset Registry file.");
			return;
		}

		for (auto entry : handles)
		{
			AssetMetadata metadata;
			metadata.Handle = entry["Handle"].as<uint64_t>();
			metadata.FilePath = entry["FilePath"].as<std::string>();
			metadata.Type = (AssetType)entry["Type"].as<int>();

			if (!FileSystem::Exists(metadata.FilePath))
			{
				HZ_CORE_WARN("Tried to load metadata for non-existing asset: {0}", metadata.FilePath);
				continue;
			}

			if (metadata.Handle == 0)
			{
				HZ_CORE_WARN("AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath);
				continue;
			}

			s_AssetRegistry[metadata.FilePath] = metadata;
		}
	}

	Ref<Asset> AssetManager::CreateAsset(const std::string& filepath, AssetType type, AssetHandle parentHandle)
	{
		Ref<Asset> asset = Ref<Asset>::Create();

		if (type == AssetType::Directory)
			asset = Ref<Directory>::Create();

		std::string extension = Utils::GetExtension(filepath);
		asset->FilePath = filepath;
		std::replace(asset->FilePath.begin(), asset->FilePath.end(), '\\', '/');

		if (s_AssetRegistry.find(asset->FilePath) != s_AssetRegistry.end())
		{
			asset->Handle = s_AssetRegistry[asset->FilePath].Handle;
			asset->Type = s_AssetRegistry[asset->FilePath].Type;

			if (asset->Type != type)
			{
				HZ_CORE_WARN("AssetType for '{0}' was different than the metadata. Did the file type change?", asset->FilePath);
				asset->Type = AssetType::None;
			}
		}
		else
		{
			asset->Handle = AssetHandle();
			asset->Type = type;
		}

		asset->FileName = Utils::RemoveExtension(Utils::GetFilename(asset->FilePath));
		asset->Extension = extension;
		asset->ParentDirectory = parentHandle;
		asset->IsDataLoaded = false;
		return asset;
	}

	void AssetManager::ImportAsset(const std::string& filepath, AssetHandle parentHandle)
	{
		std::string extension = Utils::GetExtension(filepath);
		AssetType type = GetAssetTypeForFileType(extension);
		Ref<Asset> asset = CreateAsset(filepath, type, parentHandle);

		if (asset->Type == AssetType::None)
			return;

		if (s_AssetRegistry.find(asset->FilePath) == s_AssetRegistry.end())
		{
			AssetMetadata metadata;
			metadata.Handle = asset->Handle;
			metadata.FilePath = asset->FilePath;
			metadata.Type = asset->Type;
			s_AssetRegistry[asset->FilePath] = metadata;
		}

		s_LoadedAssets[asset->Handle] = asset;
	}

	AssetHandle AssetManager::ProcessDirectory(const std::string& directoryPath, AssetHandle parentHandle)
	{
		Ref<Directory> dirInfo = CreateAsset(directoryPath, AssetType::Directory, parentHandle).As<Directory>();
		dirInfo->IsDataLoaded = true;

		if (s_AssetRegistry.find(dirInfo->FilePath) == s_AssetRegistry.end())
		{
			AssetMetadata metadata;
			metadata.Handle = dirInfo->Handle;
			metadata.FilePath = dirInfo->FilePath;
			metadata.Type = dirInfo->Type;
			s_AssetRegistry[dirInfo->FilePath] = metadata;
		}

		s_LoadedAssets[dirInfo->Handle] = dirInfo;

		if (IsAssetHandleValid(parentHandle))
			s_LoadedAssets[parentHandle].As<Directory>()->ChildDirectories.push_back(dirInfo->Handle);

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path().string(), dirInfo->Handle);
			else
				ImportAsset(entry.path().string(), dirInfo->Handle);
		}

		return dirInfo->Handle;
	}

	void AssetManager::ReloadAssets()
	{
		ProcessDirectory("assets", 0);

		// Sort the assets alphabetically (not the best impl)
		std::vector<std::pair<std::string, Ref<Asset>>> sortedVec;
		for (auto& [handle, asset] : s_LoadedAssets)
		{
			std::string filename = asset->FileName;
			std::for_each(filename.begin(), filename.end(), [](char& c)
				{
					c = std::tolower(c);
				});
			sortedVec.push_back(std::make_pair(filename, asset));
		}

		std::sort(sortedVec.begin(), sortedVec.end());
		s_LoadedAssets.clear();

		for (auto& p : sortedVec)
			s_LoadedAssets[p.second->Handle] = p.second;

		// Remove any non-existent assets from the asset registry
		for (auto it = s_AssetRegistry.begin(); it != s_AssetRegistry.end(); )
		{
			if (s_LoadedAssets.find(it->second.Handle) == s_LoadedAssets.end())
			{
				it = s_AssetRegistry.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	void AssetManager::UpdateRegistryCache()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [filepath, metadata] : s_AssetRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << metadata.Handle;
			out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
			out << YAML::Key << "Type" << YAML::Value << (int)metadata.Type;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout("assets/cache/AssetRegistryCache.hzr");
		fout << out.c_str();
	}

	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::s_LoadedAssets;
	std::unordered_map<std::string, AssetManager::AssetMetadata> AssetManager::s_AssetRegistry;
	AssetManager::AssetsChangeEventFn AssetManager::s_AssetsChangeCallback;

}