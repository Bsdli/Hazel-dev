workspace "Hazel"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

	startproject "Hazelnut"
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Hazel/vendor/GLFW/include"
IncludeDir["Glad"] = "Hazel/vendor/Glad/include"
IncludeDir["ImGui"] = "Hazel/vendor/ImGui"
IncludeDir["glm"] = "Hazel/vendor/glm"

include "Hazel/vendor/GLFW"
include "Hazel/vendor/Glad"
include "Hazel/vendor/ImGui"

project "Hazel"
	location "Hazel"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "hzpch.h"
	pchsource "Hazel/src/hzpch.cpp"

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
		"%{prj.name}/vendor",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{prj.name}/vendor/assimp/include",
		"%{prj.name}/vendor/stb/include"
	}
	
	links 
	{ 
		"GLFW",
		"Glad",
		"ImGui",
		"opengl32.lib"
	}
	
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
				
	filter "configurations:Release"
		defines "HZ_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "HZ_DIST"
		optimize "On"

project "Hazelnut"
	location "Hazelnut"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
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
		"%{IncludeDir.glm}"
	}

	postbuildcommands 
	{
		'{COPY} "../Hazelnut/assets" "%{cfg.targetdir}/assets"'
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
			'{COPY} "../Hazel/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"'
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
			'{COPY} "../Hazel/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"'
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
			'{COPY} "../Hazel/vendor/assimp/bin/Release/assimp-vc141-mtd.dll" "%{cfg.targetdir}"'
		}
		
		-- project "Sandbox"
		-- location "Sandbox"
		-- kind "ConsoleApp"
		-- language "C++"
		-- cppdialect "C++17"
		-- staticruntime "on"
		
		-- targetdir ("bin/" .. outputdir .. "/%{prj.name}")
		-- objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
		-- links 
		-- { 
		-- 	"Hazel"
		-- }
		
		-- files 
		-- { 
		-- 	"%{prj.name}/src/**.h", 
		-- 	"%{prj.name}/src/**.c", 
		-- 	"%{prj.name}/src/**.hpp", 
		-- 	"%{prj.name}/src/**.cpp" 
		-- }
		
		-- includedirs 
		-- {
		-- 	"%{prj.name}/src",
		-- 	"Hazel/src",
		-- 	"Hazel/vendor",
		-- 	"%{IncludeDir.glm}"
		-- }
		
		-- filter "system:windows"
		-- 	systemversion "latest"
					
		-- 	defines 
		-- 	{ 
		-- 		"HZ_PLATFORM_WINDOWS"
		-- 	}
		
		-- filter "configurations:Debug"
		-- 	defines "HZ_DEBUG"
		-- 	symbols "on"
	
		-- 	links
		-- 	{
		-- 		"Hazel/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		-- 	}
					
		-- filter "configurations:Release"
		-- 	defines "HZ_RELEASE"
		-- 	optimize "on"
	
		-- 	links
		-- 	{
		-- 		"Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		-- 	}
	
		-- filter "configurations:Dist"
		-- 	defines "HZ_DIST"
		-- 	optimize "on"
	
		-- 	links
		-- 	{
		-- 		"Hazel/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		-- 	}