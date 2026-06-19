// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
// @PR JanSeliv [Coding Standards] - CoreMinimal.h leads UE group, put before Components/ActorComponent.h like neighbor GrsBombProjectile.h
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
// @PR JanSeliv [Coding Standards] - unused header include, no replicated UPROPERTY nor GetLifetimeReplicatedProps override here, remove it
#include "Net/UnrealNetwork.h"

#include "GrsCharacterManagerComponent.generated.h"

// @PR JanSeliv [Coding Standards] - unused forward declaration, EBmrCurrentGameState not referenced in header, remove it
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