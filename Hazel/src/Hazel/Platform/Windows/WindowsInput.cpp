#include "hzpch.h"
#include "Hazel/Core/Input.h"
#include "WindowsWindow.h"

#include "Hazel/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Hazel {

	bool Input::IsKeyPressed(KeyCode keycode)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(keycode));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());

		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return (float)x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return (float)y;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());

		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &x, &y);
		return { (float)x, (float)y };
	}

	// TODO: A better way to do this is to handle it internally, and simply move the cursor the opposite side
	//		of the screen when it reaches the edge
	void Input::SetCursorMode(CursorMode mode)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		glfwSetInputMode(static_cast<GLFWwindow*>(window.GetNativeWindow()), GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
	}

	CursorMode Input::GetCursorMode()
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		return (CursorMode)(glfwGetInputMode(static_cast<GLFWwindow*>(window.GetNativeWindow()), GLFW_CURSOR) - GLFW_CURSOR_NORMAL);
	}

}