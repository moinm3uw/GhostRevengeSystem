// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GhostRevengeSystemRuntime : ModuleRules
{
	public GhostRevengeSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				// Bomber
				"Bomber"
			}
		);


		PrivateDependencyModuleNames.AddRange(new string[]
			{
				"CoreUObject", "Engine", "Slate", "SlateCore", // core
				"GameplayTags", // UE_DEFINE_GAMEPLAY_STATIC
				// Bomber
				"MyUtils",
			}
		);
	}
}