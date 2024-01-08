#pragma once

#include "Hazel/Core/Window.h"
#include "Hazel/Renderer/RendererContext.h"

#include <GLFW/glfw3.h>

namespace Hazel {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		virtual void ProcessEvents() override;
		virtual void SwapBuffers() override;

		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Height; }

		virtual std::pair<uint32_t, uint32_t> GetSize() const override { return { m_Data.Width, m_Data.Height }; }
		virtual std::pair<float, float> GetWindowPos() const override;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		virtual void SetVSync(bool enabled) override;
		virtual bool IsVSync() const override;

		virtual void Maximize() override;

		virtual const std::string& GetTitle() const override { return m_Data.Title; }
		virtual void SetTitle(const std::string& title) override;

		inline void* GetNativeWindow() const { return m_Window; }

		virtual Ref<RendererContext> GetRenderContext() override { return m_RendererContext; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		GLFWcursor* m_ImGuiMouseCursors[9] = { 0 };

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		float m_LastFrameTime = 0.0f;

		Ref<RendererContext> m_RendererContext;
	};

}