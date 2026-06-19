// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
// @PR JanSeliv [Coding Standards] - header only inherits IModuleInterface, include minimal `Modules/ModuleInterface.h` like neighbor ProgressionSystemRuntimeModule.h, ModuleManager.h belongs in cpp for IMPLEMENT_MODULE
#include "Modules/ModuleManager.h"

/** Define Grs log category. */
DECLARE_LOG_CATEGORY_EXTERN(LogGrs, Log, All);

class FGhostRevengeSystemRuntimeModule : public IModuleInterface
{
public:
	// @PR JanSeliv [Coding Standards] - GRSModuleName unused, no reference across module, remove dead member
	inline static const FName GRSModuleName = TEXT("GhostRevengeSystem");
	//~IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface
};
