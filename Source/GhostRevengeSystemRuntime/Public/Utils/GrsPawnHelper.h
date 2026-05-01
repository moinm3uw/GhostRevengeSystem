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
	/** GrsPawn checker  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void GrsPawnCheckf(class AGRSPlayerCharacter* GrsPawn);

	/** Initialize skeletal mesh of the character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void InitializeSkeletalMesh(class AGRSPlayerCharacter* GrsPawn);

	/** Configure the movement component of the character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void MovementComponentConfiguration(class AGRSPlayerCharacter* GrsPawn);

	/** Set up the capsule component of the character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void SetupCapsuleComponent(class AGRSPlayerCharacter* GrsPawn);

	/** Returns the Skeletal Mesh of ghost revenge character. */
	static class UBmrSkeletalMeshComponent* GetMeshChecked(class AGRSPlayerCharacter* GrsPawn);

	/** Set visibility of the player character */
	static void SetVisibility(class AGRSPlayerCharacter* GrsPawn, bool Visibility);

	/** Set visibility of the arrow on top of player character */
	static void SetArrowEnabled(class AGRSPlayerCharacter* GrsPawn, bool bVisibility);

	/** Initialize character visual (animation, skins)  once added to the level by utilizing player id */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void SetCharacterVisual(class AGRSPlayerCharacter* GrsPawn);

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	static void InitPlayerMesh(class AGRSPlayerCharacter* GrsPawn);

	/** Set side for this pawn (left or right) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	static void SetPawnSide(class AGRSPlayerCharacter* GrsPawn);

	/** Checks if Pawn is replicated fully (player state and controller present */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static bool bIsReady(class AGRSPlayerCharacter* GrsPawn);

	/** Refresh the pawn visuals  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void RefreshPawn(class AGRSPlayerCharacter* GrsPawn);

	/** Hide spline elements (trajectory) */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	static void ClearTrajectorySplines(class AGRSPlayerCharacter* GrsPawn);

	/** Initialize player name widget (on top of character) */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static void InitializePlayerNameWidget(class AGRSPlayerCharacter* GrsPawn);
};
