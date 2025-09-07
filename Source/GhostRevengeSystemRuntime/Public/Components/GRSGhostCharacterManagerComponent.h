// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoolManagerTypes.h"
#include "GRSGhostCharacterManagerComponent.generated.h"

/**
 * Actor component attached to game state to spawn ghost characters
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGRSGhostCharacterManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGRSGhostCharacterManagerComponent();

protected:
	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Actors Handlers"))
	TArray<FPoolObjectHandle> PoolActorHandlersInternal;

	// Called when the game starts
	virtual void BeginPlay() override;

	/** Listen game states to remove ghost character from level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveGhostCharacterFromMap();

	/** Whenever a main player character is removed from level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnMainCharacterRemovedFromLevel();

	/** Add ghost character to the current active game (on level map) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void AddGhostCharacter();

	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Called when the ghost player kills another player and will be swaped with him */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnGhostEliminatesPlayer();

public:
};
