// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

using UnrealBuildTool;

public class GhostRevengeSystemRuntime : ModuleRules
{
	public GhostRevengeSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs; //@Todo: DEBUG.  Instead of current UseExplicitOrSharedPCHs
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Error;
        OptimizeCode = CodeOptimization.Never;  // debug
        bUseUnity = false; //@Todo: DEBUG

		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core", "UMG", "EnhancedInput", "GameplayAbilities" // Created UBmrInputAction, UBmrInputMappingContext
				// Bomber
				,
				"Bomber"
			}
		);


		PrivateDependencyModuleNames.AddRange(new string[]
			{
				"CoreUObject", "Engine", "Slate", "SlateCore" // core
				,
				"GameplayTags" // UE_DEFINE_GAMEPLAY_STATIC
				,
				"GameplayAbilities" // Tags
				// Bomber
				,
				"MyUtils", "PoolManager", "FunctionPicker" // spawn ghost character
				,
				"GameplayMessageRuntime" // BmrGameplayMessageSubsystem
			}
		);
	}
}