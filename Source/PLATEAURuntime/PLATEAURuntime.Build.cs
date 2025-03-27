// Copyright 2023 Ministry of Land, Infrastructure and Transport

using UnrealBuildTool;
using System;
using System.IO;

public class PLATEAURuntime : ModuleRules
{
    public PLATEAURuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        bEnableExceptions = true;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Default;

        PublicIncludePaths.AddRange(
            new string[]
            {
            }
        );


        PrivateIncludePaths.AddRange(
            new string[]
            {
                // ... add other private include paths required here ...
            }
        );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities" , "Landscape", "MeshDescription"
            }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "MeshDescription",
                "StaticMeshDescription",
                "RHI",
                "ImageWrapper",
                "RenderCore",
                "OpenGL",
                "Projects",
                "Json",
                "JsonUtilities"
                // ... add private dependencies that you statically link with here ...	
            }
        );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        IncludeLibPlateau();
    }

    // 注意 : 他の PLATEAU*.Build.cs にも同じものを書いてください
    public void IncludeLibPlateau()
    {

        bEnableExceptions = true;
        
        PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/include"));

        PublicDefinitions.Add("CITYGML_STATIC_DEFINE");

        string libPlateauPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            libPlateauPath = libPlateauPath + "/windows/plateau_combined.lib";
            PublicAdditionalLibraries.Add(libPlateauPath);
            PublicAdditionalLibraries.Add("glu32.lib");
            PublicAdditionalLibraries.Add("opengl32.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add(libPlateauPath + "/macos/arm64/libplateau_combined.a");
            PublicAdditionalLibraries.Add(libPlateauPath + "/macos/x86_64/libplateau_combined.a");
            
            PublicAdditionalLibraries.Add("/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libiconv.tbd");
            PublicAdditionalLibraries.Add("/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/liblzma.tbd");
            PublicAdditionalLibraries.Add("/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL.tbd");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            libPlateauPath = libPlateauPath + "/linux/libplateau.a";
            PublicAdditionalLibraries.Add(libPlateauPath);
        }
        else
        {
            throw new Exception("Unknown OS.");
        }


        //using c++17
        CppStandard = CppStandardVersion.Default;
    }
}
