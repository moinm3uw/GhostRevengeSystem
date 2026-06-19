// Copyright (c)  Valerii Rotermel & Yevhenii Selivanov

#pragma once

// Bmr
#include "Controllers/BmrPlayerController.h"

// UE
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GrsPlayerControllerComponent.generated.h"

class ABmrPlayerController;
class ABmrPawn;
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

	// @PR JanSeliv [Coding Standards] - EBmrEndGameState used by-value but not forward-declared, add enum class EBmrEndGameState : uint8; at top like neighbor headers
	/** Called when player's match result was changed (Win, lose, draw or none applied). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EBmrEndGameState EndGameState);

	/** Is increased when this player kills an opponent */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnOpponentsKilledNumChanged(int32 OpponentsKilledNum);

public:
	// @PR JanSeliv [Coding Standards] - BlueprintProtected meta on public method, move under protected to mirror access, applies across file
	// @PR JanSeliv [Coding Standards] - AutoCreateRefTerm names ActionValue but no such param, drop stale meta, applies across file
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
	// @PR JanSeliv [Coding Standards] - wrap UObject member in TObjectPtr<ABmrPawn>, raw pointer not allowed for .h UObject member
	/** Reference to a main player character (BmrPawn) that was eliminated (original player, not ghost)  */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Bmr Player Character"))
	ABmrPawn* MainBmrPlayerPawn = nullptr;

public:
	// @PR JanSeliv [Coding Standards] - const getter missing BlueprintPure, add it like GetPlayerController and GetCurrentPawn above
	/** Returns main player (BmrPawn) character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE ABmrPawn* GetMainPlayerPawn() const { return MainBmrPlayerPawn; }

	/*********************************************************************************************
	 * Main functionality
	 **********************************************************************************************/
public:
	// @PR JanSeliv [Coding Standards] - Internal-suffixed transient runtime state must be protected not public, add BlueprintProtected to mirror access
	// @PR JanSeliv [Coding Standards] - UPROPERTY missing BP-expose specifier, add BlueprintReadWrite like every other Transient member in module, BlueprintProtected meta inert without it
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "[GhostRevengeSystem]")
	float CurrentHoldTimeInternal = 0.0f;

	/** Enables or disable input context (enhanced input) depends on possession state. Called when possessed pawn changed */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** Enables or disables the input context.
	 * * @param bEnable - true to enable, false to disable */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetManagedInputContextEnabled(AController* PlayerController, bool bEnable);

	// @PR JanSeliv [Coding Standards] - signature-only FInputActionValue lacks elaborated specifier, use const struct FInputActionValue& like FGameplayEventData above so no heavy include needed, applies across file (ChargeBomb)
	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void MovePlayer(const FInputActionValue& ActionValue);

	/** Hold button to increase trajectory on max level achieved throw projectile */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ChargeBomb(const FInputActionValue& ActionValue);

	/** Add and update visual representation of charging (aiming) progress as trajectory */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ShowVisualTrajectory();

	// @PR JanSeliv [Coding Standards] - non-const ref out-param needs Out-prefix or Ref-postfix name (OutResult), applies across file
	/** Add spline points to the aiming spline component */
	// @PR JanSeliv [Coding Standards] - "Aimning" typo in Category, rename to "Aiming", applies across file (5 Category strings)
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aimning", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aimning", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void AddSplineMesh(FPredictProjectilePathResult& Result);

	/** Configure PredictProjectilePath settings and get result */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aimning", meta = (BlueprintProtected))
	void PredictProjectilePath(FPredictProjectilePathResult& PredictResult);

	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aimning", meta = (BlueprintProtected))
	void ThrowProjectile();

	// @PR JanSeliv [Coding Standards] - pass FBmrCell struct by const ref, by-value copies whole struct
	/** Spawn bomb at aiming mesh location */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aimning", meta = (BlueprintProtected))
	void SpawnBomb(FBmrCell TargetCell);
};