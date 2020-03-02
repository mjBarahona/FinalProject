project "MiniZ"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "On"
    systemversion "latest"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("build/" .. outputdir .. "/%{prj.name}")
    architecture "x86_64"

    defines {"_WIN32" , "WIN32"}

    files
    {
        "**.h",
        "**.c",
        "**.cpp"
    }

    filter "configurations:Debug"
       
        runtime "Debug"
        symbols "On"
        buildoptions "/MTd"

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        buildoptions "/MT"