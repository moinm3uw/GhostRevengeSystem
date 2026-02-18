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
	 * Delegates
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

	/*********************************************************************************************
	 * Initialization
	 **********************************************************************************************/

	/** Sets default values for this character's properties */
	AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer);

protected:
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
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetPlayerName(const ABmrPawn* MainCharacter);

protected:
	/** 3D widget component that displays the player name above the character. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<class UBmrPlayerNameWidgetComponent> PlayerName3DWidgetComponentInternal = nullptr;

	/*********************************************************************************************
	 * Main functionality (core loop)
	 **********************************************************************************************/

public:
	friend class UBmrCheatManager;

	/** Called when the game starts or when spawned (on spawned on the level) */
	virtual void BeginPlay() override;

	/** Called when actor is destroyed */
	virtual void Destroyed() override;

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

	/*********************************************************************************************
	 * GAS
	 **********************************************************************************************/

protected:
	/** Cached handle of current applied effect */
	FActiveGameplayEffectHandle GASEffectHandle;

public:
	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Returns handle of current applied ability effect  */
	UFUNCTION(BlueprintCallable, Category = "C++")
	FORCEINLINE FActiveGameplayEffectHandle GetGASEffectHandle() const { return GASEffectHandle; }

	/** To Remove current active applied gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RemoveActiveGameplayEffect();

	/** To apply explosion gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void ApplyExplosionGameplayEffect();

	/*********************************************************************************************
	 * Utils
	 **********************************************************************************************/

	/** Returns the Skeletal Mesh of ghost revenge character. */
	UBmrSkeletalMeshComponent& GetMeshChecked() const;

	/** Set visibility of the player character */
	void SetVisibility(bool Visibility);

	/** Returns own character ID, e.g: 0, 1, 2, 3 */
	UFUNCTION(BlueprintPure, Category = "C++")
	int32 GetPlayerId() const;

	/** Set character visual once added to the level from a refence character (visuals, animations) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetCharacterVisual(const ABmrPawn* PlayerCharacter);

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player
	 * @param bForcePlayerSkin If true, will force the bot to change own skin to look like a player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetPlayerMeshData(bool bForcePlayerSkin = false);

protected:
	/** Possess a player controller */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void TryPossessController(AController* PlayerController);

	/*********************************************************************************************
	 * Aiming
	 **********************************************************************************************/
protected:
	/** Mesh of component. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Mesh Component"))
	TObjectPtr<class UMeshComponent> MeshComponentInternal = nullptr;

	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	class USplineComponent* ProjectileSplineComponentInternal;

	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TArray<class USplineMeshComponent*> SplineMeshArrayInternal;

	/** Aiming sphere used when a player aiming*/
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* AimingSphereComponent;

	/** Projectile to be spawned once bomb is ready to be thrown */
	UPROPERTY()
	TObjectPtr<class AGRSBombProjectile> BombProjectileInternal = nullptr;

public:
	/** Add a mesh to the last element of the predict Projectile path results */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddMeshToEndProjectilePath(FVector Location);

	/** Add spline points to the spline component */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Hide spline elements (trajectory) */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ClearTrajectorySplines();

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddSplineMesh(FPredictProjectilePathResult& Result);

	/*********************************************************************************************
	 * Bomb
	 **********************************************************************************************/
public:
	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void ThrowProjectile();

	/** Spawn bomb on aiming sphere position. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SpawnBomb(FBmrCell TargetCell);

public:
	/** Clean up the character for the MGF unload */
	void PerformCleanUp();
};
