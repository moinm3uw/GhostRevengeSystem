// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

using UnrealBuildTool;

public class GhostRevengeSystemRuntime : ModuleRules
{
	public GhostRevengeSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Latest; 
		bEnableNonInlinedGenCppWarnings = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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