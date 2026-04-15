// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** Define Grs log category. */
DECLARE_LOG_CATEGORY_EXTERN(LogGrs, Log, All);

class FGhostRevengeSystemRuntimeModule : public IModuleInterface
{
public:
	inline static const FName GRSModuleName = TEXT("GhostRevengeSystem");
	//~IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface
};
