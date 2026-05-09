// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// PoolManager
#include "Data/PoolObjectHandle.h"

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsPawnComponent.generated.h"

/**
 * Component attached to main BmrPawn to spawn ghost player as MGF ready.
 * Is part of overall MGF loading. If component will not be registered module will not be considered as loaded.
 * On owning BmrPawn readiness events listens overall GhostRevengeSystem MGF load with primarily goal to spawn, init GrsPawns and place in world.
 * Initialization sets replicated PlayerID for each spawned pawn.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsPawnComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Initialization
	 **********************************************************************************************/

public:
	// Sets default values for this component's properties
	UGrsPawnComponent();

	/** Returns BmrPawn of this component */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]")
	ABmrPawn* GetBmrPawn() const;
	ABmrPawn& GetBmrPawnChecked() const;

	/*********************************************************************************************
	 * Main functionality (core loop)
	 **********************************************************************************************/
protected:
	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "GrsPawn Pool Manager Handlers"))
	TArray<FPoolObjectHandle> GrsPawnPoolManagerHandlers;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component */
	virtual void OnUnregister() override;

	/** Event that fires when any pawn is spawned, possessed, and replicated, obtain pawn from Payload.Instigator */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void Player_PawnReady(const struct FGameplayEventData& Payload);

	/** A pawn could be loaded/replicated faster than MGF(GFP) is fully loaded therefore waiting for whole module to be initialized is required */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	/** Spawn ghost character when a module is initialized */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void AddGhostCharacter();

	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedGhostPawns - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void OnTakeGrsPawnsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedGhostPawns);
};