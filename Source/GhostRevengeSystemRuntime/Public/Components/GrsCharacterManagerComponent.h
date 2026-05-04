// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"

#include "GrsCharacterManagerComponent.generated.h"

enum class EBmrCurrentGameState : uint8;

/**
 * Actor component attached to game state to spawn ghost characters
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

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/

protected:
	/** Called when the GRS data asset is loaded and available */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnDataAssetLoaded(const class UGRSDataAsset* DataAsset);
};
