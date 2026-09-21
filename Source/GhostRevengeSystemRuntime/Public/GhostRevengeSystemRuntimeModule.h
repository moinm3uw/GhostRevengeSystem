// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/** Define Grs log category. */
DECLARE_LOG_CATEGORY_EXTERN(LogGrs, Log, All);

/** Max number of players in the match.
 * Used for the GFP to considered as ready only once a pawn component is registered for each of them.
 * Since now, it relies strongly on certain amount of players there is a @todo to obtain max player param from Bmr core */
extern const int32 GrsMaxPlayers;

class FGhostRevengeSystemRuntimeModule : public IModuleInterface
{
public:
	//~IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface
};
