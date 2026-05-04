// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "AbilitySystemInterface.h"
#include "Actors/BmrPawn.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GrsPawnSubobjects/GrsPawnAimingComponent.h"
#include "GrsPawnSubobjects/GrsPawnArrowStartWidgetComponent.h"
#include "GrsPawnSubobjects/GrsPawnPlayerNickNameWidgetComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Net/UnrealNetwork.h"

#include "GrsPawn.generated.h"

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
class GHOSTREVENGESYSTEMRUNTIME_API AGrsPawn : public ACharacter
    , public IAbilitySystemInterface
{
	GENERATED_BODY()

protected:
	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Obtains players state from the cached and replicated PlayerID  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	class UGrsPlayerStateComponent* GetGrsPlayerStateComponent() const;
	class UGrsPlayerStateComponent& GetGrsPlayerStateComponentChecked() const;

public:
	/** Sets default values for this character's properties */
	AGrsPawn(const FObjectInitializer& ObjectInitializer);

protected:
	/** 3D widget component that displays the player name above the character */
	FGrsPawnPlayerNickNameWidgetComponent PlayerNickName3DWidgetComponent;

	/** 3D Static mesh component that displays the arrow above the local player during match start. */
	FGrsPawnArrowStartWidgetComponent ArrowStartWidgetComponent;

	/** A GrsPawnComponent that spawned this pawn */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Owning Grs Pawn Component"))
	class UGrsPawnComponent* OwningPawnComponent = nullptr;

	/*********************************************************************************************
	 * Player Character
	 **********************************************************************************************/
protected:
	/** Player id of related BmrPlayerCharacter */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_PlayerID", Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Id of Bmr Player Character"))
	int32 PlayerID = 0;

public:
	/** Called on client when player ID is changed. */
	UFUNCTION()
	void OnRep_PlayerID();

	/**Returns current replicated player ID */
	UFUNCTION()
	FORCEINLINE int32 GetPlayerID() const { return PlayerID; }

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

protected:
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

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPreRemovedFromLevel(class UBmrMapComponent* PlayerMapComponent, class UObject* DestroyCauser);

	/** Activates ghost with required initiation  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryActivateGhostCharacter(AGrsPawn* GhostCharacter, ABmrPawn* FromPlayerCharacter);

	/** Possess a player controller */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	void TryPossessController(AController* PlayerController);

	/** Overridable function called whenever this actor is being removed from a level. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** APawn Interface when this pawn was unpossessed */
	virtual void UnPossessed() override;

	/** APawn Interface when this pawn was possessed by a new controller */
	virtual void PossessedBy(AController* NewController) override;

	/** APawn Interface when this pawn was replicated by a new controller */
	virtual void OnRep_Controller() override;

	/** APawn Interface when this pawn was replicated by a new player state */
	virtual void OnRep_PlayerState() override;

	/** Refresh and enable this pawn */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]")
	void RefreshPawn();

	/** Remove ghost character from the level when clean up or ghost kills a player */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RemoveGhostCharacterFromMap();

	/** Clean up the character for the MGF unload */
	void PerformCleanUp();

	/*********************************************************************************************
	 * Aiming functionality
	 **********************************************************************************************/
protected:
	/**  Ghost player Aiming visualization and bomb spawn functionality component */
	FGrsPawnAimingComponent AimingComponent;

public:
	/** Add a mesh to the last element of the predict Projectile path results */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void AddMeshToEndProjectilePath(FVector Location);

	/** Add spline points to the spline component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void AddSplineMesh(FPredictProjectilePathResult& Result);

	/** Throw projectile event, bound to onetime button press */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ThrowProjectile();
};
