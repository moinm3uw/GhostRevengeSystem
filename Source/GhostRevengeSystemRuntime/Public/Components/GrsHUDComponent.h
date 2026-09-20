// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "GrsHUDComponent.generated.h"

enum class EBmrEndGameState : uint8;

/**
 * Actor component attached to game state to take care of the Bmr HUD while this GFP is loaded.
 * Hides the end game result of the local player while they are eliminated, since they still play as a ghost and the match is not over for them.
 * Restores the HUD back on each new match and on GFP unload.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsHUDComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/
public:
	/** Sets default values for this component's properties */
	UGrsHUDComponent();

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/
protected:
	/** Is called when local player character is ready to guarantee that the player state is initialized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	/** Listen game states to restore the HUD back once the match is over */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);

	/** Listen end game states of the local player to hide the HUD while they play as a ghost */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EBmrEndGameState EndGameState);

	/** Changes the Bmr HUD visibility */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void ChangeHUDEndResultVisibility(bool bVisibility);

	/** Find and return a textblock element responsible for the end game result */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	class UTextBlock* GetTextBlockToHide(FName ResultTextBlockName);
};
