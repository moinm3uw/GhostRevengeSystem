// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "PoolManagerTypes.h"

#include "GRSGhostCharacterManagerComponent.generated.h"

enum class EBmrCurrentGameState : uint8;

/**
 * Actor component attached to game state to spawn ghost characters
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGRSGhostCharacterManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerCharacterPreRemovedFromLevel, UBmrMapComponent*, MapComponent, UObject*, DestroyCauser);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActivateGhostCharacter, AGRSPlayerCharacter*, GhostCharacter, const ABmrPawn*, PlayerCharacter);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveGhostCharacterFromMap, AGRSPlayerCharacter*, GhostCharacter);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshGhostCharacters);

	/** Called right before player character is going to be removed from the Map, on both server and clients */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnPlayerCharacterPreRemovedFromLevel OnPlayerCharacterPreRemovedFromLevel;

	/** Called to activate a ghost character from a player character reference */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnActivateGhostCharacter OnActivateGhostCharacter;

	/** Called to remove a ghost character from map */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnRemoveGhostCharacterFromMap OnRemoveGhostCharacterFromMap;

	/** Called to refresh ghost characters  */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnRefreshGhostCharacters OnRefreshGhostCharacters;

	// Sets default values for this component's properties
	UGRSGhostCharacterManagerComponent();

protected:
	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Actors Handlers"))
	TArray<FPoolObjectHandle> PoolActorHandlersInternal;

	/** Contains list of player characters that were eliminated at least once with reference to assigned ghost if still a ghost
	 * If a GhostPlayerCharacter reference is empty it means PlayerCharacter was revived once and character can't be a ghost anymore */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Dead Player Characters"))
	TMap<class ABmrPawn*, class AGRSPlayerCharacter*> DeadPlayerCharacters;

	/** Contains list of all map components events bounded to. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Bound MapComponents"))
	TArray<TWeakObjectPtr<class UBmrMapComponent>> BoundMapComponents;

	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/** The component is considered as loaded only when the subsystem is loaded */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnInitialize();

	/** Refresh the ghost characters visual */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RefreshGhostCharacters() const;

	/** Remove ghost characters from the map */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveGhostCharacters();

	/** Listen game states to remove ghost character from level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(EBmrCurrentGameState CurrentGameState);

	/** Subscribes to PlayerCharacters death events in order to see if a player died */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void RegisterForPlayerDeath();

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void PlayerCharacterOnPreRemovedFromLevel(class UBmrMapComponent* MapComponent, class UObject* DestroyCauser);

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
