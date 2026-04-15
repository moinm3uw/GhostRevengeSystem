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
class GHOSTREVENGESYSTEMRUNTIME_API AGRSPlayerCharacter : public ACharacter
    , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/*********************************************************************************************
	 * Delegates
	 **********************************************************************************************/
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGhostAddedToLevel);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGhostPossesController_Client);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGhostPossesController_Server);

	/** Is called when a ghost character added to level without possession */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "[GhostRevengeSystem]")
	FOnGhostAddedToLevel OnGhostAddedToLevel;

	/** Is called when a ghost character is added to level and possessed a controller on client */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "[GhostRevengeSystem]")
	FOnGhostPossesController_Client OnGhostPossesController_Client;

	/** Is called when a ghost character is added to level and possessed a controller on server*/
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "[GhostRevengeSystem]")
	FOnGhostPossesController_Server OnGhostPossesController_Server;

	/*********************************************************************************************
	 * Initialization
	 **********************************************************************************************/

	/** Sets default values for this character's properties */
	AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	/** Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetDefaultParams();

	/** Initialize skeletal mesh of the character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void InitializeSkeletalMesh();

	/** Configure the movement component of the character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void MovementComponentConfiguration();

	/** Set up the capsule component of the character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetupCapsuleComponent();

	/*********************************************************************************************
	 * Nickname component
	 **********************************************************************************************/
public:
	/** Returns the 3D widget component that displays the player name above the character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE class UBmrPlayerNameWidgetComponent* GetPlayerName3DWidgetComponent() const { return PlayerName3DWidgetComponent; }

	/** Initialize player name widget (on top of character) */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void InitializePlayerNameWidget();

protected:
	/** 3D widget component that displays the player name above the character. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<class UBmrPlayerNameWidgetComponent> PlayerName3DWidgetComponent = nullptr;

	/*********************************************************************************************
	 * Arrow component
	 **********************************************************************************************/
public:
	/** Returns static mesh component that displays the arrow above the local player during match start. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE class UBmrPlayerArrowStartComponent* GetPlayerArrowStartWidgetComponent() const { return PlayerArrowStartComponent; }

protected:
	/** Static mesh component that displays the arrow above the local player during match start. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[Bomber]", meta = (BlueprintProtected))
	TObjectPtr<class UBmrPlayerArrowStartComponent> PlayerArrowStartComponent = nullptr;

	/** A GrsPawnComponent that spawned this pawn */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Owning Grs Pawn Component"))
	class UGrsPawnComponent* OwningPawnComponent = nullptr;

	/*********************************************************************************************
	 * Main functionality (core loop)
	 **********************************************************************************************/

public:
	friend class UBmrCheatManager;

	/** Basic initialization of the Pawn */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void InitPawn(int32 NewPlayerId);

	/** Register owning pawn component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterPawnComponent(class UGrsPawnComponent* NewPawnComponent);

	/** Remove ghost character from the level */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RemoveGhostCharacterFromMap();

protected:
	/** Called when the game starts or when spawned (on spawned on the level) */
	virtual void BeginPlay() override;

	/** Overridable function called whenever this actor is being removed from a level. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** APawn Interface when this pawn was possessed by a new controller */
	virtual void PossessedBy(AController* NewController) override;

	/** APawn Interface when this pawn was replicated by a new controller */
	virtual void OnRep_Controller() override;
	
	/** APawn Interface when this pawn was replicated by a new player state */
	virtual void OnRep_PlayerState() override;
	
	/** Refresh the pawn visuals  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void RefreshPawn();
	
	/** Checks if Pawn is replicated fully (player state and controller present */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	bool bIsReady();
	
	/** APawn Interface when this pawn was unpossessed */
	virtual void UnPossessed() override;

	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Returns properties that are replicated for the lifetime of the actor channel. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The player character could be replicated faster than MGF(GFP) is loaded on client so the only we have to wait/check for subsystem to initialize as it is central loading point */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);
	
	/** Is increased when this player kills an opponent */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnOpponentsKilledNumChanged(int32 OpponentsKilledNum);

	/** Listen game states to remove ghost character from level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);

	/** Activates ghost with required initiation  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryActivateGhostCharacter(AGRSPlayerCharacter* GhostCharacter, ABmrPawn* FromPlayerCharacter);

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPreRemovedFromLevel(class UBmrMapComponent* PlayerMapComponent, class UObject* DestroyCauser);

	/*********************************************************************************************
	 * Player Character
	 **********************************************************************************************/

	/** Player id of related BmrPlayerCharacter */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_PlayerID", Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Id of Bmr Player Character"))
	int32 PlayerID = 0;

public:
	/** Called on client when player ID is changed. */
	UFUNCTION()
	void OnRep_PlayerID();

	/**Returns current replicated player ID */
	UFUNCTION()
	FORCEINLINE int32 GetPlayerID() { return PlayerID; }

	/*********************************************************************************************
	 * Utils
	 **********************************************************************************************/

	/** Returns the Skeletal Mesh of ghost revenge character. */
	UBmrSkeletalMeshComponent& GetMeshChecked() const;

	/** Set visibility of the player character */
	void SetVisibility(bool Visibility);

	/** Set visibility of the arrow on top of player character */
	void SetArrowEnabled(bool bVisibility);

	/** Initialize character visual (animation, skins)  once added to the level by utilizing player id */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SetCharacterVisual();

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	void InitPlayerMesh();

protected:
	/** Possess a player controller */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	void TryPossessController(AController* PlayerController);

	/** Set side for this pawn (left or right) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	void SetPawnSide();

	/*********************************************************************************************
	 * Aiming
	 **********************************************************************************************/
protected:
	/** Mesh of component. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Mesh Component"))
	TObjectPtr<class UMeshComponent> MeshComponentInternal = nullptr;

	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]")
	class USplineComponent* ProjectileSplineComponentInternal;

	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]")
	TArray<class USplineMeshComponent*> SplineMeshArrayInternal;

	/** Aiming sphere used when a player aiming*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem")
	class UStaticMeshComponent* AimingSphereComponent;

public:
	/** Add a mesh to the last element of the predict Projectile path results */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void AddMeshToEndProjectilePath(FVector Location);

	/** Applies spawn bomb gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void ApplyExplosionGameplayEffect();

	/** Removes spawn bomb gameplay effect */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RemoveExplosionGameplayEffect();

	/** Add spline points to the spline component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Hide spline elements (trajectory) */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ClearTrajectorySplines();

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void AddSplineMesh(FPredictProjectilePathResult& Result);

	/*********************************************************************************************
	 * Bomb
	 **********************************************************************************************/
public:
	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ThrowProjectile();

	/** Spawn bomb on aiming sphere position. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void SpawnBomb(FBmrCell TargetCell);

public:
	/** Clean up the character for the MGF unload */
	void PerformCleanUp();
};
