#include "hzpch.h"
#include "Hazel/Core/Input.h"
#include "WindowsWindow.h"

#include "Hazel/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Hazel {

	bool Input::IsKeyPressed(int keycode)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(int button)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());

		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), button);
		return state == GLFW_PRESS;
	}

	float Input::GetMouseX()
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());

		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);

		return (float)xpos;
	}

	float Input::GetMouseY()
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());

		double xpos, ypos;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);

		return (float)ypos;
	}

}