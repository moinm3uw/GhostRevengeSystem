// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
// @PR JanSeliv [Coding Standards] - cpp uses IMPLEMENT_MODULE but relies on transitive include for it, add own `#include "Modules/ModuleManager.h"` like neighbor ProgressionSystemRuntimeModule.cpp
#include "GhostRevengeSystemRuntimeModule.h"

// @PR JanSeliv [Coding Standards] - unused include, GRSWorldSubSystem not referenced in cpp, remove it
#include "SubSystems/GRSWorldSubSystem.h"

// @PR JanSeliv [Coding Standards] - no LOCTEXT used in cpp, remove dead LOCTEXT_NAMESPACE define and matching undef below
#define LOCTEXT_NAMESPACE "FGhostRevengeSystemRuntimeModule"

DEFINE_LOG_CATEGORY(LogGrs);

void FGhostRevengeSystemRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory;
	// the exact timing is specified in the .uplugin file per-module
}

void FGhostRevengeSystemRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.
	// For modules that support dynamic reloading, we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGhostRevengeSystemRuntimeModule, GhostRevengeSystemRuntime)
