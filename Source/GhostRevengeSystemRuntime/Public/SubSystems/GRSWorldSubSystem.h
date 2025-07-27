// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
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

	/** Register the ghost revenge system spot component */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RegisterSpotComponent(class UGhostRevengeSystemSpotComponent* SpotComponent);

	/** Get current spot component */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE class UGhostRevengeSystemSpotComponent* GetSpotComponent() const { return SpotComponentInternal; }

	/** Register the ghost revenge player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RegisterGhostPlayerCharacter(class AGRSPlayerCharacter* GhostPlayer);

	/** Get current player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE class AGRSPlayerCharacter* GetGhostPlayerCharacter() const { return GhostPlayerCharacterInternal; }

	/** Register main player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RegisterMainPlayerCharacter(class APlayerCharacter* MainPlayerCharacter);
	
	/** Returns main player character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	class APlayerCharacter* GetMainPlayerCharacter();

	/** Returns main player character spawn location */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE FVector GetMainPlayerCharacterSpawnLocation() const { return MainPlayerCharacterSpawnLocationInternal; }

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

	/** Spot Component for the current character */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Spot Component"))
	TObjectPtr<class UGhostRevengeSystemSpotComponent> SpotComponentInternal;

	/** Main player character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TArray<TObjectPtr<class APlayerCharacter>> MainPlayerCharacterArrayInternal;

	/** Main player character spawn location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	FVector MainPlayerCharacterSpawnLocationInternal;

	/** Begin play of the subsystem */
	void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);
};
