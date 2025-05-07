// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "GhostRevengeSystemRuntime/Public/SubSystems/GRSWorldSubSystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGRSWorldSubSystem() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_UWorldSubsystem();
GHOSTREVENGESYSTEMRUNTIME_API UClass* Z_Construct_UClass_UGRSWorldSubSystem();
GHOSTREVENGESYSTEMRUNTIME_API UClass* Z_Construct_UClass_UGRSWorldSubSystem_NoRegister();
UPackage* Z_Construct_UPackage__Script_GhostRevengeSystemRuntime();
// End Cross Module References

// Begin Class UGRSWorldSubSystem
void UGRSWorldSubSystem::StaticRegisterNativesUGRSWorldSubSystem()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGRSWorldSubSystem);
UClass* Z_Construct_UClass_UGRSWorldSubSystem_NoRegister()
{
	return UGRSWorldSubSystem::StaticClass();
}
struct Z_Construct_UClass_UGRSWorldSubSystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Implements the world subsystem to access different components in the module \n */" },
#endif
		{ "IncludePath", "SubSystems/GRSWorldSubSystem.h" },
		{ "ModuleRelativePath", "Public/SubSystems/GRSWorldSubSystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Implements the world subsystem to access different components in the module" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGRSWorldSubSystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGRSWorldSubSystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UWorldSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_GhostRevengeSystemRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGRSWorldSubSystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGRSWorldSubSystem_Statics::ClassParams = {
	&UGRSWorldSubSystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGRSWorldSubSystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UGRSWorldSubSystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGRSWorldSubSystem()
{
	if (!Z_Registration_Info_UClass_UGRSWorldSubSystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGRSWorldSubSystem.OuterSingleton, Z_Construct_UClass_UGRSWorldSubSystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGRSWorldSubSystem.OuterSingleton;
}
template<> GHOSTREVENGESYSTEMRUNTIME_API UClass* StaticClass<UGRSWorldSubSystem>()
{
	return UGRSWorldSubSystem::StaticClass();
}
UGRSWorldSubSystem::UGRSWorldSubSystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGRSWorldSubSystem);
UGRSWorldSubSystem::~UGRSWorldSubSystem() {}
// End Class UGRSWorldSubSystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGRSWorldSubSystem, UGRSWorldSubSystem::StaticClass, TEXT("UGRSWorldSubSystem"), &Z_Registration_Info_UClass_UGRSWorldSubSystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGRSWorldSubSystem), 3372926242U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_1631871057(TEXT("/Script/GhostRevengeSystemRuntime"),
	Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
