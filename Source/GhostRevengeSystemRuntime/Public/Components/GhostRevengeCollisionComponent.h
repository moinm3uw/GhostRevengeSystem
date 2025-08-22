// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structures/Cell.h"
#include "PoolManagerTypes.h"
#include "GhostRevengeCollisionComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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

	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Replicated, Category = "C++", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<class AActor> LeftSideCollisionInternal;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Replicated, Category = "C++", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AActor> RightSideCollisionInternal;

	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Returns properties that are replicated for the lifetime of the actor channel. */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called when game state is changed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Spawn a collision box the side of the map */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "C++")
	void SpawnMapCollisionOnSide();

	/** Grabs a side collision asset from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Remove a collision box the sides of the map */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveMapCollisionOnSide();
};
