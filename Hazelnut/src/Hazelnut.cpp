#include <Hazel.h>
#include <Hazel/EntryPoint.h>

#include "EditorLayer.h"

class HazelnutApplication : public Hazel::Application
{
public:
	HazelnutApplication(const Hazel::ApplicationProps& props)
		: Application(props)
	{
	}

	virtual void OnInit() override
	{
		PushLayer(new Hazel::EditorLayer());
	}
};

Hazel::Application* Hazel::CreateApplication()
{
	return new HazelnutApplication({"Hazelnut", 1600, 900});
}