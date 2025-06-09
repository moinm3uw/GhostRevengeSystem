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

	/** Returns the Player Controller component of the ghost character */
	UFUNCTION(BlueprintPure, Category = "C++", DisplayName = "Get Ghost Character Player Controller Component", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class AGRSPlayerCharacter* GetPlayerControllerComponent(const UObject* OptionalWorldContext = nullptr);

	/* Delegate to inform that module is loaded. To have better loading control of the MGF  */
	UPROPERTY(BlueprintAssignable, Transient, Category = "C++")
	FGRSOnInitialize OnInitialize;

	/** Returns the data asset that contains all the assets of Ghost Revenge System game feature.
	 * @see UGRSWorldSubsystem::GRSDataAssetInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const class UGRSDataAsset* GetGRSDataAsset() const;

	/** Returns available FName for spot component */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FName GetSpotName();

	/** Register the ghost revenge system spot component */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RegisterSpotComponent(class UGhostRevengeSystemSpotComponent* SpotComponent);

	/** Get current spot component */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE class UGhostRevengeSystemSpotComponent* GetSpotComponent() const { return SpotComponentInternal; }

protected:
	/** Contains all the assets and tweaks of Ghost Revenge System game feature.
	 * Note: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.
	 * Property is put to subsystem because its instance is created before any other object.
	 * It can't be put to DevelopSettings class because it does work properly for MGF-modules. */
	UPROPERTY(Config, VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Data Asset"))
	TSoftObjectPtr<const class UGRSDataAsset> DataAssetInternal;

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Stores list of FNames (enums converted to FName) of spots */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Spot Map"))
	TMap<FName/*SpotName*/, TObjectPtr<class UGRSComponent>> SpotComponentsMapInternal;

	/** Spot Component for the current character */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Spot Component"))
	TObjectPtr<class UGhostRevengeSystemSpotComponent> SpotComponentInternal;

	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
