// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GhostRevengeUtils.generated.h"

/**
 *
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the ghost character */
	UFUNCTION(BlueprintPure, Category = "C++", DisplayName = "Get Ghost Player Character", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class AGRSPlayerCharacter* GetGhostPlayerCharacter(const UObject* OptionalWorldContext = nullptr);
};
