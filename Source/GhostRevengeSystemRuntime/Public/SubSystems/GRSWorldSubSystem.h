// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "PoolManagerTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "GRSWorldSubSystem.generated.h"

/**
 * Implements the world subsystem to access different components in the module 
 */
UCLASS(BlueprintType, Blueprintable, Config = "GhostRevengeSystem", DefaultConfig)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSWorldSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGRSOnInitialize);

	/** Returns this Subsystem, is checked and will crash if it can't be obtained.*/
	static UGRSWorldSubSystem& Get();
	static UGRSWorldSubSystem& Get(const UObject& WorldContextObject);

	/* Delegate to inform that module is loaded. To have better loading control of the MGF  */
	UPROPERTY(BlueprintAssignable, Transient, Category = "C++")
	FGRSOnInitialize OnInitialize;

	/** Returns the data asset that contains all the assets of Ghost Revenge System game feature.
	 * @see UGRSWorldSubsystem::GRSDataAssetInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const class UGRSDataAsset* GetGRSDataAsset() const;

	/** Register main player character from ghost revenge system spot component */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RegisterMainCharacter(class APlayerCharacter* PlayerCharacter);

	/** When a main player character was removed from level spawn a new ghost character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void MainCharacterRemovedFromLevel(UMapComponent* MapComponent);

	/** Get current player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE class AGRSPlayerCharacter* GetGhostPlayerCharacter() const { return GhostPlayerCharacterInternal; }

	/** Get current player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE FVector GetMainPlayerCharacterSpawnLocation() const { return MainPlayerCharacterSpawnLocationInternal; }

	/** Returns main player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	class APlayerCharacter* GetMainPlayerCharacter();

protected:
	/** Contains all the assets and tweaks of Ghost Revenge System game feature.
	 * Note: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.
	 * Property is put to subsystem because its instance is created before any other object.
	 * It can't be put to DevelopSettings class because it does work properly for MGF-modules. */
	UPROPERTY(Config, VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Data Asset"))
	TSoftObjectPtr<const class UGRSDataAsset> DataAssetInternal;

	/** AGRSPlayerCharacter, set once game state changes into in-game */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Current Player Character"))
	TObjectPtr<class AGRSPlayerCharacter> GhostPlayerCharacterInternal;

	/** Main player character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TObjectPtr<class APlayerCharacter> MainPlayerCharacterInternal;

	/** Main player character spawn location on added to level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	FVector MainPlayerCharacterSpawnLocationInternal;

	/** Array of main players */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TArray<TObjectPtr<class APlayerCharacter>> MainPlayerCharacterArrayInternal;

	/** Array of pool actors handlers of characters which should be released */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Pool Actors Handlers"))
	TArray<FPoolObjectHandle> PoolActorHandlersInternal;

	/** Begin play of the subsystem */
	void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Add ghost character to the current active game (on level map) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "C++")
	void AddGhostCharacter();

	/** Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects);

	/** Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveGhostCharacterFromMap();

	/** Called when the ghost player kills another player and will be swaped with him */
	UFUNCTION(BlueprintCallable, Category= "C++")
	void OnGhostEliminatesPlayer();
};
