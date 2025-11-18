// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "PoolManagerTypes.h"
#include "Structures/Cell.h"

#include "GhostRevengeCollisionComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeCollisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties */
	UGhostRevengeCollisionComponent();

protected:
	/** Array of pool actors handlers of collisions that should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Collisions Actors Handlers"))
	TArray<FPoolObjectHandle> CollisionPoolActorHandlersInternal;

	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Is called when local player character is ready to guarantee that they player controller is initialized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* Character, int32 CharacterID);

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/** The spawner is considered as loaded only when the subsystem is loaded */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnInitialize();

	/** Spawn a collision box the side of the map */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "C++")
	void SpawnMapCollisionOnSide();

	/** Grabs a side collision asset from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);
};
