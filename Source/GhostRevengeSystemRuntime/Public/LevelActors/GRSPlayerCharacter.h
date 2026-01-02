// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "Actors/BmrPawn.h"
#include "Components/BmrMapComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Net/UnrealNetwork.h"

#include "GRSPlayerCharacter.generated.h"

/**
 * Represents the side of ghost character
 */
UENUM(BlueprintType, DisplayName = "Ghost Character Side")
enum class EGRSCharacterSide : uint8
{
	///< Is not defined
	None,
	///< Star is locked
	Left,
	///< Star is unlocked
	Right,
};

/**
 * Ghost Players (only for players, no AI) whose goal is to perform revenge as ghost (spawned on side of map).
 * Copy the died player mesh and skin.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGRSPlayerCharacter : public ACharacter,
                                                          public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/*********************************************************************************************
	 * Delegates & GAS
	 **********************************************************************************************/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGhostAddedToLevel);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGhostPossesController_Client);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGhostPossesController_Server);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGhostRemovedFromLevel, AController*, CurrentPlayerController, AGRSPlayerCharacter*, GhostPlayerCharacter);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGhostEliminatesPlayer, FVector, AtLocation, AGRSPlayerCharacter*, GhostCharacter);

	/** Is called when a ghost character added to level without possession */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnGhostAddedToLevel OnGhostAddedToLevel;

	/** Is called when a ghost character is added to level and possessed a controller on client */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnGhostPossesController_Client OnGhostPossesController_Client;

	/** Is called when a ghost character is added to level and possessed a controller on server*/
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnGhostPossesController_Server OnGhostPossesController_Server;

	/** Is called when a ghost character removed from level */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnGhostRemovedFromLevel OnGhostRemovedFromLevel;

	/** Is called when a ghost characters kills another player */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnGhostEliminatesPlayer OnGhostEliminatesPlayer;

	/** Cached handle of current applied effect */
	FActiveGameplayEffectHandle GASEffectHandle;

	/*********************************************************************************************
	 * Utils
	 **********************************************************************************************/

	/** Get the character side */
	UFUNCTION(BlueprintCallable, Category = "C++")
	EGRSCharacterSide GetCharacterSide() const;

	/** Returns the Skeletal Mesh of ghost revenge character. */
	UBmrSkeletalMeshComponent& GetMeshChecked() const;

	/** Set visibility of the player character */
	void SetVisibility(bool Visibility);

	/** Returns own character ID, e.g: 0, 1, 2, 3 */
	UFUNCTION(BlueprintPure, Category = "C++")
	int32 GetPlayerId() const;

	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*********************************************************************************************
	 * Initialization
	 **********************************************************************************************/

	/** Sets default values for this character's properties */
	AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	/** Mesh of component. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Mesh Component"))
	TObjectPtr<class UMeshComponent> MeshComponentInternal = nullptr;

	/** Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetDefaultParams();

	/** Initialize skeletal mesh of the character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitializeSkeletalMesh();

	/** Configure the movement component of the character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void MovementComponentConfiguration();

	/** Set up the capsule component of the character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetupCapsuleComponent();

	/*********************************************************************************************
	 * Nickname component
	 **********************************************************************************************/
public:
	/** Returns the 3D widget component that displays the player name above the character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE class UBmrPlayerNameWidgetComponent* GetPlayerName3DWidgetComponent() const { return PlayerName3DWidgetComponentInternal; }

	/** Set/Update player name */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetPlayerName(const ABmrPawn* MainCharacter) const;

protected:
	/** 3D widget component that displays the player name above the character. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<class UBmrPlayerNameWidgetComponent> PlayerName3DWidgetComponentInternal = nullptr;

	/*********************************************************************************************
	 * Lifecycle and Main functionality
	 **********************************************************************************************/

public:
	friend class UBmrCheatManager;

	/** Called when the game starts or when spawned (on spawned on the level) */
	virtual void BeginPlay() override;

	/** Set character visual once added to the level from a refence character (visuals, animations) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetCharacterVisual(const ABmrPawn* PlayerCharacter);

	/** Set and apply default skeletal mesh for this player.
	 * @param bForcePlayerSkin If true, will force the bot to change own skin to look like a player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetDefaultPlayerMeshData(bool bForcePlayerSkin = false);

	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void MovePlayer(const FInputActionValue& ActionValue);

protected:
	/** Refresh ghost players required elements. Happens only when game is starting or active because requires to have all players (humans) to be connected */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnRefreshGhostCharacters();

	/** Server-only logic Perform ghost character activation (possessing controller) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void ActivateGhostCharacter(AGRSPlayerCharacter* GhostCharacter, const ABmrPawn* PlayerCharacter);

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPreRemovedFromLevel(class UBmrMapComponent* MapComponent, class UObject* DestroyCauser);

	/** Remove ghost character from the level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnRemoveGhostCharacterFromMap(AGRSPlayerCharacter* GhostCharacter);

	/** Possess a player controller */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void TryPossessController(AController* PlayerController);

	/*********************************************************************************************
	 * Hold To Charge and aim
	 **********************************************************************************************/
protected:
	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	class USplineComponent* ProjectileSplineComponentInternal;

	UPROPERTY()
	TObjectPtr<class AGRSBombProjectile> BombProjectileInternal = nullptr;

	UPROPERTY()
	float CurrentHoldTimeInternal = 0.0f;

	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TArray<class USplineMeshComponent*> SplineMeshArrayInternal;

	/** Aiming sphere used when a player aiming*/
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* AimingSphereComponent;

public:
	/** Hold button to increase trajectory on max level achieved throw projectile */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ChargeBomb(const FInputActionValue& ActionValue);

	/** Add and update visual representation of charging (aiming) progress as trajectory */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ShowVisualTrajectory();

	/** Throw projectile event, binded to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void ThrowProjectile();

	/** Hide spline elements (trajectory) */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ClearTrajectorySplines();

	/** Configure PredictProjectilePath settings and get result */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void PredictProjectilePath(FPredictProjectilePathResult& PredictResult);

	/** Add a mesh to the last element of the predicцt Projectile path results */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddMeshToEndProjectilePath(FPredictProjectilePathResult& Result);

	/** Add spline points to the spline component */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddSplineMesh(FPredictProjectilePathResult& Result);

	/** Called when a controller has been replicated to the client. Used to enable input context only */
	virtual void OnRep_Controller() override;

	/** Called when a character has been possessed by a new controller */
	virtual void PossessedBy(AController* NewController) override;

	/** Called when our Controller no longer possesses us. Only called on the server (or in standalone). */
	virtual void UnPossessed() override;

	/** Event called after a pawn's controller has changed, on the server and owning client. This will happen at the same time as the delegate on GameInstance.
	 * Used to disable input context on the clients only */
	UFUNCTION()
	void OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** Enables or disables the input context.
	 * * @param bEnable - true to enable, false to disable */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetManagedInputContextEnabled(AController* PlayerController, bool bEnable);

	/*********************************************************************************************
	 * Bomb
	 **********************************************************************************************/
protected:
	/** Spawn bomb on aiming sphere position. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SpawnBomb(FBmrCell TargetCell);

	/*********************************************************************************************
	 * End of ghost character
	 **********************************************************************************************/
public:
	/** Clean up the character for the MGF unload */
	void PerformCleanUp();
};
