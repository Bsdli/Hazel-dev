project "Glad"
    kind "StaticLib"
    language "C"
    staticruntime "off"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
        "include/glad/glad.h",
        "src/glad.c"
    }

    includedirs
	{
        "include"
    }
    
	filter "system:windows"
        systemversion "latest"
