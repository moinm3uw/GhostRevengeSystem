// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"

#include "GrsCharacterManagerComponent.generated.h"

enum class EBmrCurrentGameState : uint8;

/**
 * Actor component attached to game state to load data asset file from disk.
 * Is part of overall GFP loading. If component will not be registered module will not be considered as loaded.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsCharacterManagerComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/
public:
	// Sets default values for this component's properties
	UGrsCharacterManagerComponent();

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/

protected:
	/** Called when the GRS data asset is loaded and available */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnDataAssetLoaded(const class UGRSDataAsset* DataAsset);
};