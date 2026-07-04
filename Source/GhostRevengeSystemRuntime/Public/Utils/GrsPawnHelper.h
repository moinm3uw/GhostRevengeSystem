// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

#pragma once

// UE
#include "UObject/Object.h"

#include "GrsPawnHelper.generated.h"

class AGrsPawn;

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
	static void SetPawnToAvailableSide(AGrsPawn* GrsPawn);

	/** Checks if Pawn is replicated fully (player state and controller present */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	static bool IsReady(const AGrsPawn* GrsPawn);

	/** Obtains player state from the provided playerID */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	static class APlayerState* GetPlayerStateForPlayerID(const AGrsPawn* GrsPawn);

	/** Obtains bmr pawn from the provided GrsPawn */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	static class ABmrPawn* GetOwningBmrPawn(AGrsPawn* GrsPawn);
};
