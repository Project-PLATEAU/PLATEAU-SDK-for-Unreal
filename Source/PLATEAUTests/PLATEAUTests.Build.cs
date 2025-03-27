// Copyright © 2023 Ministry of Land, Infrastructure and Transport

using UnrealBuildTool;
using System;
using System.IO;


public class PLATEAUTests : ModuleRules {
	public PLATEAUTests(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnableExceptions = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
                "Landscape",
                "MeshDescription",
                "StaticMeshDescription",
                "MeshDescriptionOperations",
            }
		);

		PrivateDependencyModuleNames.AddRange(
			new[] {
				"PLATEAUEditor",
				"PLATEAURuntime",
				"PLATEAUEditorBPLibraries",
                "PLATEAURuntimeBPLibraries",
                "UnrealEd",
            });

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
			});

		IncludeLibPlateau();
	}

	// 注意 : 他の PLATEAU*.Build.cs にも同じものを書いてください
	public void IncludeLibPlateau() {
		bEnableExceptions = true;

		PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "../ThirdParty/include"));

		PublicDefinitions.Add("CITYGML_STATIC_DEFINE");

		string libPlateauPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib");

		if (Target.Platform == UnrealTargetPlatform.Win64) {
			libPlateauPath = libPlateauPath + "/windows/plateau_combined.lib";
			PublicAdditionalLibraries.Add(libPlateauPath);
			PublicAdditionalLibraries.Add("glu32.lib");
			PublicAdditionalLibraries.Add("opengl32.lib");
		} else if (Target.Platform == UnrealTargetPlatform.Mac) {
			
			PublicAdditionalLibraries.Add(libPlateauPath + "/macos/arm64/libplateau_combined.a");
			PublicAdditionalLibraries.Add(libPlateauPath + "/macos/x86_64/libplateau_combined.a");
			
			PublicAdditionalLibraries.Add(
				"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libiconv.tbd");
			PublicAdditionalLibraries.Add("/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/liblzma.tbd");
			PublicAdditionalLibraries.Add(
				"/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL.tbd");
		} else if (Target.Platform == UnrealTargetPlatform.Linux) {
			libPlateauPath = libPlateauPath + "/linux/libplateau.a";
			PublicAdditionalLibraries.Add(libPlateauPath);
		} else {
			throw new Exception("Unknown OS.");
		}

		//using c++17
		CppStandard = CppStandardVersion.Default;
	}
}
