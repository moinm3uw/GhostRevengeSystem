// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// PoolManager
#include "Data/PoolObjectHandle.h"

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsCollisionComponent.generated.h"

enum class EGRSCharacterSide : uint8;

/**
 *  Attached to the BmrGameState to spawn collision on the sides of the map.
 *  The component on start up register in subsystem and waits for overall loading of GFP. After OnInitialize event spawn collision through pool manager.
 *  Owns the whole lifecycle of those side collisions: takes them from the pool, places them by side and returns them back to the pool on cleanup.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsCollisionComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/

public:
	/** Sets default values for this component's properties */
	UGrsCollisionComponent();

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/*********************************************************************************************
	 * Side Collisions actors
	 **********************************************************************************************/

public:
	/** Returns TRUE if collision are spawned */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	bool IsCollisionsSpawned() const;

	/** Returns left side spawned collision or nullptr */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE AActor* GetLeftCollisionActor() const { return LeftSideCollisionInternal; }

	/** Returns right side spawned collision or nullptr */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE AActor* GetRightCollisionActor() const { return RightSideCollisionInternal; }

	/** Returns spawned collisions back to the pool they were taken from and clears cached references */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearCollisions();

protected:
	/** Array of pool actors handlers of collisions that should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Pool Collisions Actors Handlers"))
	TArray<FPoolObjectHandle> CollisionPoolActorHandlersInternal;

	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<AActor> LeftSideCollisionInternal = nullptr;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<AActor> RightSideCollisionInternal = nullptr;

	/** Caches the spawned collision actor as the one that bounds given side of the map
	 * @param Side - Side of the map the collision actor was placed on
	 * @param CollisionActor - Spawned collision actor to cache */
	void SetCollisionActorBySide(EGRSCharacterSide Side, AActor* CollisionActor);

	/** Returns the world location where the collision actor has to be placed to bound given side of the map */
	static FVector GetCollisionLocationBySide(EGRSCharacterSide Side);

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/
protected:
	/** Is called when local player character is ready to guarantee that they player controller is initialized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	/** The spawner is considered as loaded only when the subsystem is loaded */
	UFUNCTION(BlueprintNativeEvent, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	/** Spawn a collision box the side of the map */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void SpawnMapCollisionOnSide();

	/** Grabs a side collision asset from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "[GhostRevengeSystem]")
	void OnTakeCollisionActorsFromPoolCompleted(const TArray<struct FPoolObjectData>& CreatedObjects);
};
