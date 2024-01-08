#include "hzpch.h"
#include "DefaultAssetEditors.h"
#include "Hazel/Asset/AssetImporter.h"

namespace Hazel {
	
	PhysicsMaterialEditor::PhysicsMaterialEditor()
		: AssetEditor("Edit Physics Material") {}

	void PhysicsMaterialEditor::OnClose()
	{
		AssetImporter::Serialize(m_Asset);
		m_Asset = nullptr;
	}

	void PhysicsMaterialEditor::Render()
	{
		if (!m_Asset)
			SetOpen(false);

		UI::BeginPropertyGrid();
		UI::Property("Static Friction", m_Asset->StaticFriction);
		UI::Property("Dynamic Friction", m_Asset->DynamicFriction);
		UI::Property("Bounciness", m_Asset->Bounciness);
		UI::EndPropertyGrid();
	}

	TextureViewer::TextureViewer()
		: AssetEditor("Edit Texture")
	{
		SetMinSize(200, 600);
		SetMaxSize(500, 1000);
	}

	void TextureViewer::OnClose()
	{
		m_Asset = nullptr;
	}

	void TextureViewer::Render()
	{
		if (!m_Asset)
			SetOpen(false);

		float textureWidth = m_Asset->GetWidth();
		float textureHeight = m_Asset->GetHeight();
		//float bitsPerPixel = Texture::GetBPP(m_Asset->GetFormat());
		float imageSize = ImGui::GetWindowWidth() - 40;
		imageSize = glm::min(imageSize, 500.0f);
		
		ImGui::SetCursorPosX(20);
		//ImGui::Image((ImTextureID)m_Asset->GetRendererID(), { imageSize, imageSize });

		UI::BeginPropertyGrid();
		UI::Property("Width", textureWidth, 0.1f, 0.0f, 0.0f, true);
		UI::Property("Height", textureHeight, 0.1f, 0.0f, 0.0f, true);
		// UI::Property("Bits", bitsPerPixel, 0.1f, 0.0f, 0.0f, true); // TODO: Format
		UI::EndPropertyGrid();
	}

}
