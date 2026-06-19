// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// PoolManager
#include "Data/PoolObjectHandle.h"

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsCollisionComponent.generated.h"

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
	 * Main functionality
	 **********************************************************************************************/
protected:
	/** Array of pool actors handlers of collisions that should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Pool Collisions Actors Handlers"))
	TArray<FPoolObjectHandle> CollisionPoolActorHandlersInternal;

	/** Is called when local player character is ready to guarantee that they player controller is initialized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	// @PR JanSeliv [Coding Standards] - OnSomething callback must be BlueprintNativeEvent, applies across file (also OnTakeCollisionActorsFromPoolCompleted), match OnLocalPawnReady
	/** The spawner is considered as loaded only when the subsystem is loaded */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	// @PR JanSeliv [Coding Standards] - protected BP-exposed func needs meta BlueprintProtected to mirror C++ access, applies across file (also OnTakeCollisionActorsFromPoolCompleted)
	/** Spawn a collision box the side of the map */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SpawnMapCollisionOnSide();

	// @PR JanSeliv [Coding Standards] - signature-only FPoolObjectData neither included nor forward-declared, PoolObjectHandle.h does not pull it, add elaborated specifier `const TArray<struct FPoolObjectData>&` like FGameplayEventData above
	/** Grabs a side collision asset from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);
};