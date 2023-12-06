#pragma once

namespace Hazel {

	class PhysicsSettingsWindow
	{
	public:
		static void OnImGuiRender(bool& show);

	private:
		static void RenderWorldSettings();
		static void RenderLayerList();
		static void RenderSelectedLayer();

	private:
		static bool Property(const char* label, const char** options, int32_t optionCount, int32_t* selected);
		static bool Property(const char* label, float& value, float min = -1.0F, float max = 1.0F);
		static bool Property(const char* label, uint32_t& value, uint32_t min = 0.0F, uint32_t max = 0.0F);
		static bool Property(const char* label, glm::vec3& value, float min = 0.0F, float max = 0.0F);
	};

}
