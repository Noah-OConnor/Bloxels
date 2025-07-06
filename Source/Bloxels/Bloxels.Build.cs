// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Bloxels : ModuleRules
{
	public Bloxels(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"ProceduralMeshComponent",
			"FastNoiseGenerator",
			"FastNoise",
			"Json",
			"JsonUtilities"
		});
	}
}
