workspace "Hazel"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	startproject "Hazelnut"
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

group "Dependencies"
include "Hazel/vendor/GLFW"
include "Hazel/vendor/Glad"
include "Hazel/vendor/ImGui"
include "Hazel/vendor/Box2D"
group ""

group "Core"
project "Hazel"
	location "Hazel"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "hzpch.h"
	pchsource "Hazel/src/hzpch.cpp"

	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp",

		"%{prj.name}/vendor/FastNoise/**.cpp",

		"%{prj.name}/vendor/yaml-cpp/src/**.cpp",
		"%{prj.name}/vendor/yaml-cpp/src/**.h",
		"%{prj.name}/vendor/yaml-cpp/include/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.h",
		"%{prj.name}/vendor/VulkanMemoryAllocator/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor",

		"%{IncludeDir.Assimp}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Box2D}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.mono}",
		"%{IncludeDir.FastNoise}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.VulkanSDK}",
		
		"%{IncludeDir.NvidiaAftermath}",
	}
	
	links
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"Box2D",
		"opengl32.lib",

		"%{Library.Vulkan}",
		"%{Library.VulkanUtils}",

		"%{Library.mono}",

		"%{Library.PhysX}",
		"%{Library.PhysXCharacterKinematic}",
		"%{Library.PhysXCommon}",
		"%{Library.PhysXCooking}",
		"%{Library.PhysXExtensions}",
		"%{Library.PhysXFoundation}",
		"%{Library.PhysXPvd}",
		
		"%{Library.NvidiaAftermath}",
	}

	defines
	{
		"PX_PHYSX_STATIC_LIB"
	}
	
	filter "files:Hazel/vendor/FastNoise/**.cpp or files:Hazel/vendor/yaml-cpp/src/**.cpp"
   	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		
		defines 
		{ 
			"HZ_PLATFORM_WINDOWS",
			"HZ_BUILD_DLL"
		}

	filter "configurations:Debug"
		defines "HZ_DEBUG"
		symbols "On"
				
		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			"%{Library.SPIRV_Tools_Debug}",
		}

	filter "configurations:Release"
		defines
		{
			"HZ_RELEASE",
			"NDEBUG" -- PhysX Requires This
		}

		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
		}

		optimize "On"

	filter "configurations:Dist"
		defines "HZ_DIST"
		optimize "On"

project "Hazel-ScriptCore"
	location "Hazel-ScriptCore"
	kind "SharedLib"
	language "C#"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.cs", 
	}
group ""

group "Tools"
project "Hazelnut"
	location "Hazelnut"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	links 
	{ 
		"Hazel"
	}
	
	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp" 
	}
	
	includedirs 
	{
		"%{prj.name}/src",
		"Hazel/src",
		"Hazel/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.Glad}"
	}

	postbuildcommands 
	{
		'{COPY} "../Hazel/vendor/NvidiaAftermath/lib/x64/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"'
	}
	
	filter "system:windows"
		systemversion "latest"
				
		defines 
		{ 
			"HZ_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug"
		defines "HZ_DEBUG"
		symbols "on"

		links
		{
			"Hazel/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Hazel/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/VulkanSDK/Bin/shaderc_sharedd.dll" "%{cfg.targetdir}"'
		}
				
	filter "configurations:Release"
		defines "HZ_RELEASE"
		optimize "on"

		links
		{
			"Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'

		}

	filter "configurations:Dist"
		defines "HZ_DIST"
		optimize "on"

		links
		{
			"Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Hazel/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}
		
--[[project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	links 
	{ 
		"Hazel"
	}
	
	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp" 
	}
	
	includedirs 
	{
		"%{prj.name}/src",
		"Hazel/src",
		"Hazel/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.glm}"
	}
	
	filter "system:windows"
		systemversion "latest"
				
		defines 
		{ 
			"HZ_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug"
		defines "HZ_DEBUG"
		symbols "on"

		links
		{
			"Hazel/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}
				
	filter "configurations:Release"
		defines "HZ_RELEASE"
		optimize "on"

		links
		{
			"Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

	filter "configurations:Dist"
		defines "HZ_DIST"
		optimize "on"

		links
		{
			"Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}]]--
group ""

workspace "Sandbox"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

project "Hazel-ScriptCore"
	location "Hazel-ScriptCore"
	kind "SharedLib"
	language "C#"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.cs", 
	}

project "ExampleApp"
	location "ExampleApp"
	kind "SharedLib"
	language "C#"

	targetdir ("Hazelnut/assets/scripts")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files 
	{
		"%{prj.name}/src/**.cs", 
	}

	links
	{
		"Hazel-ScriptCore"
	}
group ""
