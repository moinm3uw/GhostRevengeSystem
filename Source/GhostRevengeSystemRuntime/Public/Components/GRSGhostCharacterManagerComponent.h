// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "PoolManagerTypes.h"

#include "GRSGhostCharacterManagerComponent.generated.h"

/**
 * Actor component attached to game state to spawn ghost characters
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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

	/** Contains list of player characters that were eliminated at least once with reference to assigned ghost if still a ghost
	 * If a GhostPlayerCharacter reference is empty it means PlayerCharacter was revived once and character can't be a ghost anymore */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Dead Player Characters"))
	TMap<class APlayerCharacter*, class AGRSPlayerCharacter*> DeadPlayerCharacters;

	// Called when the game starts
	virtual void BeginPlay() override;

	/** The component is considered as loaded only when the subsystem is loaded */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnInitialize();

	/** Listen game states to remove ghost character from level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Subscribes to PlayerCharacters death events in order to see if a player died */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void RegisterForPlayerDeath();

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void PlayerCharacterOnPreRemovedFromLevel(class UMapComponent* MapComponent, class UObject* DestroyCauser);

	/** Add ghost character to the current active game (on level map) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void AddGhostCharacter();

	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Called when the ghost player kills another player and will be swaped with him */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void OnGhostEliminatesPlayer(FVector AtLocation, class AGRSPlayerCharacter* GhostCharacter);

	/** Called when the ghost character should be removed from level to unpossess controller */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void OnGhostRemovedFromLevel(class AController* CurrentController, class AGRSPlayerCharacter* GhostCharacter);

	/** Unpossess ghost and spawn&possess a regular player character to the level at location */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RevivePlayerCharacter(class AController* PlayerController, AGRSPlayerCharacter* GhostCharacter);

public:
};
