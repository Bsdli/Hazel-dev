#pragma once

#include "Hazel/Renderer/Texture.h"
#include "Hazel/Asset/AssetManager.h"

namespace Hazel {

	class ObjectsPanel
	{
	public:
		ObjectsPanel();

		void OnImGuiRender();

	private:
		void DrawObject(const char* label, AssetHandle handle);

	private:
		Ref<Texture2D> m_CubeImage;
	};

}
