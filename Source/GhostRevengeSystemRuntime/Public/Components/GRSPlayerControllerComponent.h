// Copyright (c) Yevhenii Selivanov

#pragma once

#include "AbilitySystemComponent.h"
#include "Components/ActorComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "CoreMinimal.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GRSPlayerControllerComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGRSPlayerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

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

public:
	UPROPERTY()
	float CurrentHoldTimeInternal = 0.0f;

	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void MovePlayer(const FInputActionValue& ActionValue);

	/** Hold button to increase trajectory on max level achieved throw projectile */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ChargeBomb(const FInputActionValue& ActionValue);

	/** Add and update visual representation of charging (aiming) progress as trajectory */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ShowVisualTrajectory();

	/** Configure PredictProjectilePath settings and get result */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void PredictProjectilePath(FPredictProjectilePathResult& PredictResult);

	/** Called when possessed pawn changed */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** Enables or disables the input context.
	 * * @param bEnable - true to enable, false to disable */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetManagedInputContextEnabled(AController* PlayerController, bool bEnable);

	/*********************************************************************************************
	 * Bomb
	 **********************************************************************************************/
public:
	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void ThrowProjectile();

	/*********************************************************************************************
	 * End of ghost character
	 **********************************************************************************************/
public:
	/** Clean up the character for the MGF unload */
	void PerformCleanUp();
};