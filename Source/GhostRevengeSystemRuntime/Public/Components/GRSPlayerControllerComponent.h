// Copyright (c) Yevhenii Selivanov

#pragma once

#include "AbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "CoreMinimal.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GRSPlayerControllerComponent.generated.h"

/**
 *  Attached to the BmrPlayerController to handle player input when a ghost character is possessed
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGRSPlayerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/

public:
	/** Sets default values for this component's properties */
	UGRSPlayerControllerComponent();

	/** Returns Player Controller of this component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]")
	ABmrPlayerController* GetPlayerController() const;
	ABmrPlayerController& GetPlayerControllerChecked() const;

	/** Returns current possessed pawn */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]")
	APawn* GetCurrentPawn() const;
	APawn& GetCurrentPawnChecked() const;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component */
	virtual void OnUnregister() override;

	/** Listen game states to reset player controller state */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);

public:
	/** Unpossess current pawn from ghost to BmwPlayerPawn */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void UnpossessGhostPawn();

	/** Disables current enhanced input and input bindings */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void DisableGhostInputs();

	/*********************************************************************************************
	 * Player Character
	 **********************************************************************************************/
protected:
	/** Reference to a main player character (BmrPawn) that was eliminated (original player, not ghost)  */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Bmr Player Character"))
	APawn* MainPlayerPawn = nullptr;

public:
	
	/** Returns main player (BmrPawn) character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE APawn* GetMainPlayerPawn() const { return MainPlayerPawn; } 
	
	/** Store reference for possessed player pawn. Used to Unpossess controller back to the pawn
	 * @param PlayerPawn is a main player pawn that ghost possesses.
	 */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetPossessedPlayerPawn(APawn* PlayerPawn);

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/
public:
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "[GhostRevengeSystem]")
	float CurrentHoldTimeInternal = 0.0f;

	/** Enables or disable input context (enhanced input) depends on possession state. Called when possessed pawn changed */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** Enables or disables the input context.
	 * * @param bEnable - true to enable, false to disable */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetManagedInputContextEnabled(AController* PlayerController, bool bEnable);

	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void MovePlayer(const FInputActionValue& ActionValue);

	/** Hold button to increase trajectory on max level achieved throw projectile */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ChargeBomb(const FInputActionValue& ActionValue);

	/** Add and update visual representation of charging (aiming) progress as trajectory */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ShowVisualTrajectory();

	/** Configure PredictProjectilePath settings and get result */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void PredictProjectilePath(FPredictProjectilePathResult& PredictResult);

	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ThrowProjectile();
};