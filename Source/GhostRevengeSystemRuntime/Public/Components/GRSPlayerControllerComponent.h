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
	APawn* GetCurrentGhostCharacter() const;
	APawn& GetCurrentPawnChecked() const;

protected:
	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Clears all transient data created by this component */
	virtual void OnUnregister() override;

public:
	/** Clean up the character for the MGF unload */
	void PerformCleanUp();

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
	
	/** Set up input bindings in given contexts. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void BindInputActionsInContext(const UBmrInputMappingContext* InInputContext);
	
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