// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
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
	// @PR JanSeliv [Coding Standards] - AGrsPawn already forward declared above, drop redundant inline class keyword, use plain AGrsPawn*, applies across file
	/** Set pawn location to available side (left or right) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	static void SetPawnToAvailableSide(class AGrsPawn* GrsPawn);

	// @PR JanSeliv [Coding Standards] - b prefix reserved for bool vars not funcs, rename bIsReady to IsReady per Is-func convention
	// @PR JanSeliv [Coding Standards] - GrsPawn only read, mark const AGrsPawn* like GetPlayerStateForPlayerID below, applies to GetOwningBmrPawn too
	// @PR JanSeliv [Coding Standards] - pure getter pairs BlueprintCallable with BlueprintPure per project convention, applies to GetPlayerStateForPlayerID and GetOwningBmrPawn
	/** Checks if Pawn is replicated fully (player state and controller present */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static bool bIsReady(class AGrsPawn* GrsPawn);

	/** Obtains player state from the provided playerID */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static class APlayerState* GetPlayerStateForPlayerID(const class AGrsPawn* GrsPawn);

	/** Obtains bmr pawn from the provided GrsPawn */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static class ABmrPawn* GetOwningBmrPawn(class AGrsPawn* GrsPawn);
};
