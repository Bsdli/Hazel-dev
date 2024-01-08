#pragma once

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/ImGui/ImGui.h"

#include <map>

#define MAX_INPUT_BUFFER_LENGTH 128

namespace Hazel {

	struct SelectionStack
	{
	public:
		void Select(AssetHandle item)
		{
			m_Selections.push_back(item);
		}

		void Deselect(AssetHandle item)
		{
			for (auto it = m_Selections.begin(); it != m_Selections.end(); it++)
			{
				if (*it == item)
				{
					m_Selections.erase(it);
					break;
				}
			}
		}

		bool IsSelected(AssetHandle item) const
		{
			for (auto selection : m_Selections)
			{
				if (selection == item)
					return true;
			}

			return false;
		}

		void Clear()
		{
			m_Selections.clear();
		}

		size_t SelectionCount() const
		{
			return m_Selections.size();
		}

		AssetHandle* GetSelectionData()
		{
			return m_Selections.data();
		}

	private:
		std::vector<AssetHandle> m_Selections;
	};

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();

	private:
		void DrawDirectoryInfo(AssetHandle directory);

		void RenderAsset(Ref<Asset>& assetHandle);
		void HandleDragDrop(Ref<Image2D> icon, Ref<Asset>& asset);
		void RenderBreadCrumbs();

		void HandleRenaming(Ref<Asset>& asset);

		void UpdateCurrentDirectory(AssetHandle directoryHandle);

	private:
		Ref<Texture2D> m_FileTex;
		Ref<Texture2D> m_BackbtnTex;
		Ref<Texture2D> m_FwrdbtnTex;

		bool m_IsDragging = false;
		bool m_UpdateBreadCrumbs = true;
		bool m_IsAnyItemHovered = false;
		bool m_UpdateDirectoryNextFrame = false;
		bool m_RenamingSelected = false;

		char m_RenameBuffer[MAX_INPUT_BUFFER_LENGTH];
		char m_SearchBuffer[MAX_INPUT_BUFFER_LENGTH];

		AssetHandle m_CurrentDirHandle;
		AssetHandle m_BaseDirectoryHandle;
		AssetHandle m_PrevDirHandle;
		AssetHandle m_NextDirHandle;

		Ref<Directory> m_CurrentDirectory;
		Ref<Directory> m_BaseDirectory;

		std::vector<Ref<Asset>> m_CurrentDirFiles;
		std::vector<Ref<Asset>> m_CurrentDirFolders;
		std::vector<Ref<Directory>> m_BreadCrumbData;

		SelectionStack m_SelectedAssets;

		std::map<std::string, Ref<Texture2D>> m_AssetIconMap;
	};

}
