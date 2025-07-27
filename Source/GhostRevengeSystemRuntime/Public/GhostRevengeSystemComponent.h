// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoolManagerTypes.h"
#include "Structures/Cell.h"
#include "GhostRevengeSystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGhostRevengeSystemComponent();

	/** Returns Player Controller of this component. */
	UFUNCTION(BlueprintPure, Category = "C++")
	APlayerController* GetPlayerController() const;
	APlayerController* GetPlayerControllerChecked() const;

	/** Adds ghost character to the level */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void SpawnGhost(class AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Called when the ghost player killed */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnGhostPlayerKilled();

	/** Add ghost character to the current active game (on level map) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "C++")
	void AddGhostCharacter();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** Initial (main) player controller */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	class AMyPlayerController* PreviousPlayerControllerInternal;

	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Actors Handlers"))
	TArray<FPoolObjectHandle> PoolActorHandlersInternal;

	/** Called when this level actor is reconstructed or added on the Generated Map, on both server and clients. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnAddedToLevel(class UMapComponent* MapComponent);

	/** Called right before owner actor going to remove from the Generated Map. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostRemovedFromLevel(class UMapComponent* MapComponent, UObject* DestroyCauser);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.)  */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EEndGameState EndGameState);

	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveGhostCharacterFromMap();
};
