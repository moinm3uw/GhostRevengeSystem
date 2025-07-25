// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GRSComponent.generated.h"


UCLASS(BlueprintType, Blueprintable, DisplayName = "GRS Component", ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGRSComponent : public UActorComponent
{
	GENERATED_BODY()
	/*********************************************************************************************
	 * Public functions
	 **********************************************************************************************/

public:
	// Sets default values for this component's properties
	UGRSComponent();

	/*********************************************************************************************
	 * Protected properties
	 **********************************************************************************************/
protected:

	UPROPERTY()
	AMyPlayerState *PlayerStateInternal;
	/*********************************************************************************************
	 * Protected functions
	 **********************************************************************************************/
protected:
	/** Called when a component is registered, after Scene is set, but before CreateRenderState_Concurrent or OnCreatePhysicsState are called. */
	virtual void OnRegister() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;
	
	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Subscribes to the end game state change notification on the player state. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalPlayerStateReady(class AMyPlayerState* PlayerState, int32 CharacterID);

	/** Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.)  */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EEndGameState EndGameState);
	
};
