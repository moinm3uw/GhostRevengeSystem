// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Data/PoolObjectHandle.h"
#include "Net/UnrealNetwork.h"

#include "GhostRevengeCollisionComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeCollisionComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/

public:
	/** Sets default values for this component's properties */
	UGhostRevengeCollisionComponent();

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/
public:
protected:
	/** Array of pool actors handlers of collisions that should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Pool Collisions Actors Handlers"))
	TArray<FPoolObjectHandle> CollisionPoolActorHandlersInternal;

	/** Is called when local player character is ready to guarantee that they player controller is initialized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	/** The spawner is considered as loaded only when the subsystem is loaded */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize();

	/** Spawn a collision box the side of the map */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SpawnMapCollisionOnSide();

	/** Grabs a side collision asset from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);
};
