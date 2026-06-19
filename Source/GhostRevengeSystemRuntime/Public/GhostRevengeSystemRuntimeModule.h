// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

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
