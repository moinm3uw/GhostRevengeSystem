// Copyright (c)  Valerii Rotermel & Yevhenii Selivanov

#pragma once

// Bmr
#include "Controllers/BmrPlayerController.h"

// UE
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GrsPlayerControllerComponent.generated.h"

enum class EBmrEndGameState : uint8;
class ABmrPlayerController;
class ABmrPawn;
struct FInputActionValue;

/**
 *  Attached to the BmrPlayerController to handle player input when a ghost character is possessed.
 *  Holds logic that listens input for charge to aim, throw projectile and spawn bomb.
 *
 *  Once BmrPlayerController (main player controller) possess a Ghost pawn (GrsPawn) component enables Grs inputs (enhanced input).
 *  For the game state change, ghost eliminates player, unload possesses back to BmrPawn and disables inputs
 *
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGrsPlayerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/

public:
	/** Sets default values for this component's properties */
	UGrsPlayerControllerComponent();

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

	/** Is increased when this player kills an opponent */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnOpponentsKilledNumChanged(int32 OpponentsKilledNum);

public:
	/** Unpossess current pawn from ghost to BmwPlayerPawn */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
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
	TObjectPtr<ABmrPawn> MainBmrPlayerPawn = nullptr;

public:
	/** Returns main player (BmrPawn) character */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE ABmrPawn* GetMainPlayerPawn() const { return MainBmrPlayerPawn; }

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/
protected:
	/** Used to track maximum charge time allowed for functionalities like - release on max charge */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	float CurrentHoldTime = 0.0f;

public:
	/** Enables or disable input context (enhanced input) depends on possession state. Called when possessed pawn changed */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** Enables or disables the input context.
	 * @param PlayerController - controller to which enable inputs
	 * @param bEnable - true to enable, false to disable */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetManagedInputContextEnabled(class AController* PlayerController, bool bEnable);

	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void MovePlayer(const FInputActionValue& ActionValue);

	/** Hold button to increase trajectory on max level achieved throw projectile */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ChargeBomb(const FInputActionValue& ActionValue);

	/** Add and update visual representation of charging (aiming) progress as trajectory */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ShowVisualTrajectory();

	/** Add spline points to the aiming spline component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aiming")
	void AddSplinePoints(FPredictProjectilePathResult& OutResult);

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aiming")
	void AddSplineMesh(FPredictProjectilePathResult& OutResult);

	/** Configure PredictProjectilePath settings and get result */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aiming")
	void PredictProjectilePath(FPredictProjectilePathResult& PredictResult);

	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aiming")
	void ThrowProjectile();

	/** Spawn bomb at aiming mesh location */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aiming")
	void SpawnBomb(const struct FBmrCell& TargetCell);
};