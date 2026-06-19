// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// PoolManager
#include "Data/PoolObjectHandle.h"

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsPawnComponent.generated.h"

/**
 * Component attached to main BmrPawn to spawn ghost player as GFP ready.
 * Is part of overall GFP loading. If component will not be registered module will not be considered as loaded.
 * On owning BmrPawn readiness events listens overall GhostRevengeSystem GFP load with primarily goal to spawn, init GrsPawns and place in world.
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

	// @PR JanSeliv [Coding Standards] - ABmrPawn undeclared in this header, add forward decl `class ABmrPawn;` at top, signature-only return type, never rely on transitive/unity include
	// @PR JanSeliv [Coding Standards] - redundant specifier pair, BlueprintPure already implies callable, drop BlueprintCallable, use BlueprintPure alone like neighbor GetGhostPlayerCharacter
	/** Returns BmrPawn of this component */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]")
	ABmrPawn* GetBmrPawn() const;
	ABmrPawn& GetBmrPawnChecked() const;

	/*********************************************************************************************
	 * Main functionality (core loop)
	 **********************************************************************************************/
protected:
	// @PR JanSeliv [Coding Standards] - protected member needs Internal suffix per module convention, rename to GrsPawnPoolManagerHandlersInternal like neighbor CollisionPoolActorHandlersInternal
	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "GrsPawn Pool Manager Handlers"))
	TArray<FPoolObjectHandle> GrsPawnPoolManagerHandlers;

// @PR JanSeliv [Coding Standards] - redundant `protected:`, same section already protected from line above, no section banner between, remove duplicate specifier
protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component */
	virtual void OnUnregister() override;

	// @PR JanSeliv [Coding Standards] - message listener handler must be On-prefixed per module convention, rename Player_PawnReady to OnPawnReady, neighbor GRSWorldSubSystem binds Player_LocalPawnReady to OnLocalPawnReady
	/** Event that fires when any pawn is spawned, possessed, and replicated, obtain pawn from Payload.Instigator */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void Player_PawnReady(const struct FGameplayEventData& Payload);

	// @PR JanSeliv [Coding Standards] - On-callback must be BlueprintNativeEvent, applies across file to OnTakeGrsPawnsFromPoolCompleted
	/** A pawn could be loaded/replicated faster than GFP is fully loaded therefore waiting for whole module to be initialized is required */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	// @PR JanSeliv [Coding Standards] - protected UFUNCTION missing meta = (BlueprintProtected), BP can call it unprotected, applies across file to OnTakeGrsPawnsFromPoolCompleted
	/** Spawn ghost character when a module is initialized */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void AddGhostCharacter();

	// @PR JanSeliv [Coding Standards] - signature-only FPoolObjectData neither included nor forward-declared, PoolObjectHandle.h does not pull it, add elaborated specifier `const TArray<struct FPoolObjectData>&` like FGameplayEventData params above
	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedGhostPawns - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void OnTakeGrsPawnsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedGhostPawns);
};