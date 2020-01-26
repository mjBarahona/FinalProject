workspace "FinalProject"
    architecture "x64"
    startproject "Demo"

    configurations
    {
        "Debug",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

project "ModelCompressor"
    location "ModelCompressor"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "On"
    systemversion "latest"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("build/" .. outputdir .. "/%{prj.name}")

    pchheader "prech.h"
    pchsource "ModelCompressor/src/prech.cpp"

    warnings "Extra"

    files
    {
        "%{prj.name}/include/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs
    {
        "%{prj.name}/include",
        "%{prj.name}/include/ModelCompressor",
        "%{prj.name}/deps",
    }

    --links
    --{
        
    --}

    filter "configurations:Debug"
        defines
        {
            "PE_DEBUG"
        }
        runtime "Debug"
        symbols "On"
        buildoptions "/MTd"

    filter "configurations:Release"
        defines "PE_RELEASE"
        runtime "Release"
        optimize "Speed"
        buildoptions "/MT"

project "Demo"
    location "Demo"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "On"
    systemversion "latest"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("build/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/include/**.h",
        "%{prj.name}/src/**.cpp",
    }

    includedirs
    {
        "ModelCompressor/include",
        "ModelCompressor/deps",
        "%{prj.name}/include",
    }

    links
    {
        "ModelCompressor"
    }

    filter "configurations:Debug"
        defines
        {
            "PE_DEBUG"
        }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "PE_RELEASE"
        runtime "Release"
        optimize "Speed"
