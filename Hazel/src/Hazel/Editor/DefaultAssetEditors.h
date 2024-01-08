#pragma once

#include "AssetEditorPanel.h"
#include "Hazel/Renderer/Mesh.h"

namespace Hazel {

	class PhysicsMaterialEditor : public AssetEditor
	{
	public:
		PhysicsMaterialEditor();

		virtual void SetAsset(const Ref<Asset>& asset) override { m_Asset = (Ref<PhysicsMaterial>)asset; }

	private:
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<PhysicsMaterial> m_Asset;
	};

	class TextureViewer : public AssetEditor
	{
	public:
		TextureViewer();

		virtual void SetAsset(const Ref<Asset>& asset) override { m_Asset = (Ref<Texture>)asset; }

	private:
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<Texture> m_Asset;
	};

}
