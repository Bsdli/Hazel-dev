#pragma once

#include "hzpch.h"
#include "Hazel/Core/Layer.h"

namespace Hazel {

	class ImGuiLayer : public Layer
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;

		void SetDarkThemeColors();

		static ImGuiLayer* Create();
	};



}
