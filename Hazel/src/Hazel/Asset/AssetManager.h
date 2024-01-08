#pragma once

#include "AssetImporter.h"
#include "Hazel/Utilities/FileSystem.h"
#include "Hazel/Utilities/StringUtils.h"

#include <map>
#include <unordered_map>

namespace Hazel {

	class AssetManager
	{
	public:
		using AssetsChangeEventFn = std::function<void()>;

		struct AssetMetadata
		{
			AssetHandle Handle;
			std::string FilePath;
			AssetType Type;
		};
	public:
		static void Init();
		static void SetAssetChangeCallback(const AssetsChangeEventFn& callback);
		static void Shutdown();

		static std::vector<Ref<Asset>> GetAssetsInDirectory(AssetHandle directoryHandle);
		static std::vector<Ref<Asset>> SearchAssets(const std::string& query, const std::string& searchPath, AssetType desiredTypes = AssetType::None);

		static bool IsDirectory(const std::string& filepath);

		static AssetHandle GetAssetHandleFromFilePath(const std::string& filepath);
		static bool IsAssetHandleValid(AssetHandle assetHandle);

		static void Rename(AssetHandle assetHandle, const std::string& newName);
		static void RemoveAsset(AssetHandle assetHandle);

		static AssetType GetAssetTypeForFileType(const std::string& extension);

		template<typename T, typename... Args>
		static Ref<T> CreateNewAsset(const std::string& filename, AssetType type, AssetHandle directoryHandle, Args&&... args)
		{
			static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

			auto& directory = GetAsset<Directory>(directoryHandle);

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Type = type;
			asset->FilePath = directory->FilePath + "/" + filename;
			asset->FileName = Utils::RemoveExtension(Utils::GetFilename(asset->FilePath));
			asset->Extension = Utils::GetFilename(filename);
			asset->ParentDirectory = directoryHandle;
			asset->Handle = AssetHandle();
			asset->IsDataLoaded = true;
			s_LoadedAssets[asset->Handle] = asset;
			AssetImporter::Serialize(asset);

			AssetMetadata metadata;
			metadata.Handle = asset->Handle;
			metadata.FilePath = asset->FilePath;
			metadata.Type = asset->Type;
			s_AssetRegistry[asset->FilePath] = metadata;
			UpdateRegistryCache();

			return asset;
		}

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle, bool loadData = true)
		{
			HZ_CORE_ASSERT(s_LoadedAssets.find(assetHandle) != s_LoadedAssets.end());
			Ref<Asset>& asset = s_LoadedAssets[assetHandle];

			if (!asset->IsDataLoaded && loadData)
				AssetImporter::TryLoadData(asset);

			return asset.As<T>();
		}

		template<typename T>
		static Ref<T> GetAsset(const std::string& filepath, bool loadData = true)
		{
			return GetAsset<T>(GetAssetHandleFromFilePath(filepath), loadData);
		}

	private:
		static void LoadAssetRegistry();
		static Ref<Asset> CreateAsset(const std::string& filepath, AssetType type, AssetHandle parentHandle);
		static void ImportAsset(const std::string& filepath, AssetHandle parentHandle);
		static AssetHandle ProcessDirectory(const std::string& directoryPath, AssetHandle parentHandle);
		static void ReloadAssets();
		static void UpdateRegistryCache();

		static void OnFileSystemChanged(FileSystemChangedEvent e);

		static AssetHandle FindParentHandleInChildren(Ref<Directory>& dir, const std::string& dirName);
		static AssetHandle FindParentHandle(const std::string& filepath);

	private:
		static std::unordered_map<AssetHandle, Ref<Asset>> s_LoadedAssets;
		static std::unordered_map<std::string, AssetMetadata> s_AssetRegistry;
		static AssetsChangeEventFn s_AssetsChangeCallback;
	};

}
