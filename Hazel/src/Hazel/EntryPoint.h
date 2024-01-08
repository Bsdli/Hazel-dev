#pragma once

#ifdef HZ_PLATFORM_WINDOWS

extern Hazel::Application* Hazel::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

int main(int argc, char** argv)
{
	while (g_ApplicationRunning)
	{
		Hazel::InitializeCore();
		Hazel::Application* app = Hazel::CreateApplication(argc, argv);
		HZ_CORE_ASSERT(app, "Client Application is null!");
		app->Run();
		delete app;
		Hazel::ShutdownCore();
	}
}

#endif
