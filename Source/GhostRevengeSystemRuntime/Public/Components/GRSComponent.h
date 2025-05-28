// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelActors/PlayerCharacter.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "GRSComponent.generated.h"

/**
 * Ghost Revenge System module, where the Owner is Player Controller actor.
 * Is responsible to spawn Ghost character after player first death and handle inputs of the spawned pawn
 */
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

	/** Returns Player Controller of this component. */
	UFUNCTION(BlueprintPure, Category = "C++")
	class AMyPlayerController* GetPlayerController() const;
	class AMyPlayerController& GetPlayerControllerChecked() const;

	/** */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE AGRSPlayerCharacter* GetGhostPlayerCharacter() { return GhostPlayerCharacter; }

	/*********************************************************************************************
	 * Protected properties
	 **********************************************************************************************/
protected:
	/*********************************************************************************************
	 * Protected functions
	 **********************************************************************************************/
protected:
	/** Called when the owning Actor begins play or when the component is created if the Actor has already begun play. */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component. */
	virtual void OnUnregister() override;

	/** Called when the ghost revenge system is ready loaded (when game transitions to ingame state) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnInitialize();

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Subscribes to the end game state change notification on the player state. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalPlayerStateReady(class AMyPlayerState* PlayerState, int32 CharacterID);

	/** Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.)  */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EEndGameState EndGameState);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Enables or disables the input context. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetManagedInputContextEnabled();

	/** AGRSPlayerCharacter, set once game state changes into in-game */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Current Player Character"))
	TObjectPtr<AGRSPlayerCharacter> GhostPlayerCharacter;
	
};
