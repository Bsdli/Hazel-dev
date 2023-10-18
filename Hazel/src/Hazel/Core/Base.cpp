#include "hzpch.h"
#include "Base.h"

#include "Log.h"

#define HAZEL_BUILD_ID "v0.1a"

namespace Hazel {

	void InitializeCore()
	{
		Hazel::Log::Init();

		HZ_CORE_TRACE("Hazel Engine {}", HAZEL_BUILD_ID);
		HZ_CORE_TRACE("Initializing...");
	}

	void ShutdownCore()
	{
		HZ_CORE_TRACE("Shutting down...");
	}

}