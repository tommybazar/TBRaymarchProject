// Created by Tommy Bazar. No rights reserved :)
// Special credits go to : Temaran (compute shader tutorial), TheHugeManatee (original concept, supervision)
// and Ryan Brucks (original raymarching code).


using UnrealBuildTool;

public class VolumeTextureToolkit : ModuleRules
{
    public VolumeTextureToolkit(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "VolumeTextureToolkit",
                "VolumeTextureToolkit/Public/",
                "VolumeTextureToolkit/Public/MHD"
                }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "RenderCore",
                "RHI",
                "AssetRegistry",
// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
