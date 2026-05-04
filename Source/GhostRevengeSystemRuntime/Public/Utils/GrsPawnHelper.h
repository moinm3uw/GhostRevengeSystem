// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "GrsPawnHelper.generated.h"

class AGRSPlayerCharacter;

/**
 * This is helper designed only for the GrsPlayerCharacter with main intent to reduce the size of GrsPlayerCharacter
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsPawnHelper : public UObject
{
	GENERATED_BODY()

public:
	/** Set pawn location to available side (left or right) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	static void SetPawnToAvailableSide(class AGRSPlayerCharacter* GrsPawn);

	/** Checks if Pawn is replicated fully (player state and controller present */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static bool bIsReady(class AGRSPlayerCharacter* GrsPawn);
	
	/** Obtains player state from the provided playerID */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static class APlayerState* GetPlayerStateForPlayerID(const class AGRSPlayerCharacter* GrsPawn);
};
