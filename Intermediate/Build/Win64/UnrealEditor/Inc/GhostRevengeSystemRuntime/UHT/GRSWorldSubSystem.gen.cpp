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
GHOSTREVENGESYSTEMRUNTIME_API UClass* Z_Construct_UClass_UGRSDataAsset_NoRegister();
GHOSTREVENGESYSTEMRUNTIME_API UClass* Z_Construct_UClass_UGRSWorldSubSystem();
GHOSTREVENGESYSTEMRUNTIME_API UClass* Z_Construct_UClass_UGRSWorldSubSystem_NoRegister();
GHOSTREVENGESYSTEMRUNTIME_API UFunction* Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature();
UPackage* Z_Construct_UPackage__Script_GhostRevengeSystemRuntime();
// End Cross Module References

// Begin Delegate FGRSOnInitialize
struct Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/SubSystems/GRSWorldSubSystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGRSWorldSubSystem, nullptr, "GRSOnInitialize__DelegateSignature", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void UGRSWorldSubSystem::FGRSOnInitialize_DelegateWrapper(const FMulticastScriptDelegate& GRSOnInitialize)
{
	GRSOnInitialize.ProcessMulticastDelegate<UObject>(NULL);
}
// End Delegate FGRSOnInitialize

// Begin Class UGRSWorldSubSystem Function GetGRSDataAsset
struct Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics
{
	struct GRSWorldSubSystem_eventGetGRSDataAsset_Parms
	{
		const UGRSDataAsset* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "C++" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the data asset that contains all the assets of Ghost Revenge System game feature.\n\x09 * @see UGRSWorldSubsystem::GRSDataAssetInternal. */" },
#endif
		{ "ModuleRelativePath", "Public/SubSystems/GRSWorldSubSystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the data asset that contains all the assets of Ghost Revenge System game feature.\n@see UGRSWorldSubsystem::GRSDataAssetInternal." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GRSWorldSubSystem_eventGetGRSDataAsset_Parms, ReturnValue), Z_Construct_UClass_UGRSDataAsset_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ReturnValue_MetaData), NewProp_ReturnValue_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGRSWorldSubSystem, nullptr, "GetGRSDataAsset", nullptr, nullptr, Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::GRSWorldSubSystem_eventGetGRSDataAsset_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::GRSWorldSubSystem_eventGetGRSDataAsset_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGRSWorldSubSystem::execGetGRSDataAsset)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(const UGRSDataAsset**)Z_Param__Result=P_THIS->GetGRSDataAsset();
	P_NATIVE_END;
}
// End Class UGRSWorldSubSystem Function GetGRSDataAsset

// Begin Class UGRSWorldSubSystem
void UGRSWorldSubSystem::StaticRegisterNativesUGRSWorldSubSystem()
{
	UClass* Class = UGRSWorldSubSystem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetGRSDataAsset", &UGRSWorldSubSystem::execGetGRSDataAsset },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
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
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Implements the world subsystem to access different components in the module \n */" },
#endif
		{ "IncludePath", "SubSystems/GRSWorldSubSystem.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/SubSystems/GRSWorldSubSystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Implements the world subsystem to access different components in the module" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OnInitialize_MetaData[] = {
		{ "Category", "C++" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/* Delegate to inform that module is loaded. To have better loading control of the MGF  */" },
#endif
		{ "ModuleRelativePath", "Public/SubSystems/GRSWorldSubSystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Delegate to inform that module is loaded. To have better loading control of the MGF" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DataAssetInternal_MetaData[] = {
		{ "BlueprintProtected", "" },
		{ "Category", "C++" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Contains all the assets and tweaks of Ghost Revenge System game feature.\n\x09 * Note: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.\n\x09 * Property is put to subsystem because its instance is created before any other object.\n\x09 * It can't be put to DevelopSettings class because it does work properly for MGF-modules. */" },
#endif
		{ "DisplayName", "Ghost Revenge System Data Asset" },
		{ "ModuleRelativePath", "Public/SubSystems/GRSWorldSubSystem.h" },
		{ "NativeConstTemplateArg", "" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Contains all the assets and tweaks of Ghost Revenge System game feature.\nNote: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.\nProperty is put to subsystem because its instance is created before any other object.\nIt can't be put to DevelopSettings class because it does work properly for MGF-modules." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnInitialize;
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_DataAssetInternal;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UGRSWorldSubSystem_GetGRSDataAsset, "GetGRSDataAsset" }, // 1926697356
		{ &Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature, "GRSOnInitialize__DelegateSignature" }, // 3517658
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGRSWorldSubSystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UGRSWorldSubSystem_Statics::NewProp_OnInitialize = { "OnInitialize", nullptr, (EPropertyFlags)0x0010000010082000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGRSWorldSubSystem, OnInitialize), Z_Construct_UDelegateFunction_UGRSWorldSubSystem_GRSOnInitialize__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OnInitialize_MetaData), NewProp_OnInitialize_MetaData) }; // 3517658
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UGRSWorldSubSystem_Statics::NewProp_DataAssetInternal = { "DataAssetInternal", nullptr, (EPropertyFlags)0x0024080000024805, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGRSWorldSubSystem, DataAssetInternal), Z_Construct_UClass_UGRSDataAsset_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DataAssetInternal_MetaData), NewProp_DataAssetInternal_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGRSWorldSubSystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGRSWorldSubSystem_Statics::NewProp_OnInitialize,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGRSWorldSubSystem_Statics::NewProp_DataAssetInternal,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGRSWorldSubSystem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGRSWorldSubSystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UWorldSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_GhostRevengeSystemRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGRSWorldSubSystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGRSWorldSubSystem_Statics::ClassParams = {
	&UGRSWorldSubSystem::StaticClass,
	"GhostRevengeSystem",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UGRSWorldSubSystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UGRSWorldSubSystem_Statics::PropPointers),
	0,
	0x009000A6u,
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
		{ Z_Construct_UClass_UGRSWorldSubSystem, UGRSWorldSubSystem::StaticClass, TEXT("UGRSWorldSubSystem"), &Z_Registration_Info_UClass_UGRSWorldSubSystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGRSWorldSubSystem), 2975855674U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_22486358(TEXT("/Script/GhostRevengeSystemRuntime"),
	Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
