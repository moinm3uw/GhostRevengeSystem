// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/** Define Grs log category. */
DECLARE_LOG_CATEGORY_EXTERN(LogGrs, Log, All);

class FGhostRevengeSystemRuntimeModule : public IModuleInterface
{
public:
	//~IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface
};
