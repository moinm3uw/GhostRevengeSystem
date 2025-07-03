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
	APlayerController& GetPlayerControllerChecked() const;

	UFUNCTION(BlueprintCallable, Category= "C++")
	void SpawnGhost(AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Add ghost character to the current active game (on level map) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void AddGhostCharacter();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	class ACharacter* PreviousPlayerCharacterInternal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	class AMyPlayerController* PreviousPlayerControllerInternal;

	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Actors Handlers"))
	TArray<FPoolObjectHandle> PoolActorHandlersInternal;

	/** Array of pool actors handlers of collisions that should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Collisions Actors Handlers"))
	TArray<FPoolObjectHandle> CollisionPoolActorHandlersInternal;

	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<class AActor> LeftSideCollisionInternal;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AActor> RightSideCollisionInternal;

	UPROPERTY()
	int32 PlayerIdInternal;

	UPROPERTY()
	FCell ActorSpawnCellLocationInternal;

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

	/** Spawn a collision box the side of the map */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SpawnMapCollisionOnSide();

	/** Remove a collision box the sides of the map */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveMapCollisionOnSide();

	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Grabs a side collision asset from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Enables or disables the input context.
	 * * @param bEnable - true to enable, false to disable */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetManagedInputContextEnabled(bool bEnable);

	/** Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveGhostCharacterFromMap();
};
