// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "SubSystems/GRSWorldSubSystem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UGRSDataAsset;
#ifdef GHOSTREVENGESYSTEMRUNTIME_GRSWorldSubSystem_generated_h
#error "GRSWorldSubSystem.generated.h already included, missing '#pragma once' in GRSWorldSubSystem.h"
#endif
#define GHOSTREVENGESYSTEMRUNTIME_GRSWorldSubSystem_generated_h

#define FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_18_DELEGATE \
static void FGRSOnInitialize_DelegateWrapper(const FMulticastScriptDelegate& GRSOnInitialize);


#define FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetGRSDataAsset);


#define FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGRSWorldSubSystem(); \
	friend struct Z_Construct_UClass_UGRSWorldSubSystem_Statics; \
public: \
	DECLARE_CLASS(UGRSWorldSubSystem, UWorldSubsystem, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/GhostRevengeSystemRuntime"), NO_API) \
	DECLARE_SERIALIZER(UGRSWorldSubSystem) \
	static const TCHAR* StaticConfigName() {return TEXT("GhostRevengeSystem");} \



#define FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UGRSWorldSubSystem(); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGRSWorldSubSystem(UGRSWorldSubSystem&&); \
	UGRSWorldSubSystem(const UGRSWorldSubSystem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGRSWorldSubSystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGRSWorldSubSystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UGRSWorldSubSystem) \
	NO_API virtual ~UGRSWorldSubSystem();


#define FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_12_PROLOG
#define FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_INCLASS_NO_PURE_DECLS \
	FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GHOSTREVENGESYSTEMRUNTIME_API UClass* StaticClass<class UGRSWorldSubSystem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_UnrealProjects_bmb_Bomber_Plugins_GameFeatures_GhostRevengeSystem_Source_GhostRevengeSystemRuntime_Public_SubSystems_GRSWorldSubSystem_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
