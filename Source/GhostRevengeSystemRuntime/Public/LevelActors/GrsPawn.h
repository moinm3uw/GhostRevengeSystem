// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GrsPawn.generated.h"

class UGrsPlayerStateComponent;
class UBmrPlayerNameWidgetComponent;
class UBmrPlayerArrowStartComponent;
class UStaticMeshComponent;
class USplineMeshComponent;

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
 * Ghost Pawns is a 2nd chance for eliminated player to come back into the game.
 * They are spawned on the side of the map and can spawn bomb to the main level. By eliminating a player/bot they will be revived back to level.
 * Ghosts can be only players, no AI\bots.
 * Holds a replicated PlayerID so that they can copy eliminated player visuals ( mesh,  applied, skin, animation, nickname etc).
 *
 * As GrsPawn is spawned it's being initialized with replicated PlayerID and with initial character location.
 * Initialization with PlayerID means that the pawn is ready for further initiation.
 * Once pawn is ready subscribes to GhostRevengeSystem readiness event to wait whole GFP to be loaded as it is relying on data from others parts.
 *
 * On GFP readiness event is triggered, Pawns starts listening when a corresponding BmrPawn (by playerID) was removed from level and activates ghosts - possess, show visual representation, set location etc).
 * Pawn automatically hides itself (visually) from level when Unpossess, EndPlay, Destroy happened.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGrsPawn : public ACharacter
    , public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	/** Obtains players state from the cached and replicated PlayerID  */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	UGrsPlayerStateComponent* GetGrsPlayerStateComponent() const;
	UGrsPlayerStateComponent& GetGrsPlayerStateComponentChecked() const;

	/** Returns the Ability System Component from the Player State.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Sets default values for this character's properties */
	AGrsPawn(const FObjectInitializer& ObjectInitializer);

protected:
	/** 3D widget component that displays the player name above the character */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<UBmrPlayerNameWidgetComponent> PlayerNickName3DWidgetComponent = nullptr;

	/** Initialize player name widget (on top of character) */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void InitializePlayerNameWidget();

public:
	/** Returns the 3D widget component that displays the player name above the character. */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UBmrPlayerNameWidgetComponent* GetPlayerNickName3DWidgetComponent() const { return PlayerNickName3DWidgetComponent; }

protected:
	/** 3D Static mesh component that displays the arrow above the local player during match start. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<UBmrPlayerArrowStartComponent> PlayerArrowStartComponent = nullptr;

public:
	/** Returns static mesh component that displays the arrow above the local player during match start. */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UBmrPlayerArrowStartComponent* GetPlayerArrowStartWidgetComponent() const { return PlayerArrowStartComponent; }

	/*********************************************************************************************
	 * Player Character
	 **********************************************************************************************/
protected:
	/** Player id of related BmrPlayerCharacter */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_PlayerID", Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Id of Bmr Player Character"))
	int32 PlayerID = 0;

public:
	/** Called on client when player ID is changed. */
	UFUNCTION()
	void OnRep_PlayerID();

	/**Returns current replicated player ID */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE int32 GetPlayerID() const { return PlayerID; }

	/*********************************************************************************************
	 * Main functionality (core loop)
	 **********************************************************************************************/

	/** Basic initialization of the Pawn */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void InitPawn(int32 NewPlayerId);

protected:
	/** Returns properties that are replicated for the lifetime of the actor channel. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The player character could be replicated faster than GFP is loaded on client so the only we have to wait/check for subsystem to initialize as it is central loading point */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnInitialize(const struct FGameplayEventData& Payload);

	/** Listen game states to remove ghost character from level */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnPreRemovedFromLevel(class UBmrMapComponent* PlayerMapComponent, class UObject* DestroyCauser);

	/** Activates ghost with required initiation  */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryActivateGhostCharacter(AGrsPawn* GhostCharacter, class ABmrPawn* FromPlayerCharacter);

	/** Possess a player controller */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryPossessController(AController* PlayerController);

	/** APawn Interface when this pawn was possessed by a new controller */
	virtual void PossessedBy(AController* NewController) override;

	/** APawn Interface when this pawn was replicated by a new controller */
	virtual void OnRep_Controller() override;

	/** APawn Interface when this pawn was replicated by a new player state */
	virtual void OnRep_PlayerState() override;

	/** Overridable function called whenever this actor is being removed from a level. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** APawn Interface when this pawn was unpossessed */
	virtual void UnPossessed() override;

	/** Refresh and enable this pawn */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RefreshPawn();

	/** Remove ghost character from the level when clean up or ghost kills a player */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void HideGhostCharacterFromMap();

	/** Clean up the character for the GFP unload */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void PerformCleanUp();

	/*********************************************************************************************
	 * Aiming functionality
	 **********************************************************************************************/
protected:
	/** Mesh of component. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem] | Aiming", meta = (BlueprintProtected))
	TObjectPtr<class UMeshComponent> AimingMeshComponent = nullptr;

	/** Spline component used to visually display a projectile trajectory path */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Aiming", meta = (BlueprintProtected))
	TObjectPtr<class USplineComponent> AimingSplineComponent = nullptr;

	/** Spline component used to build a projectile trajectory path */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Aiming", meta = (BlueprintProtected))
	TArray<TObjectPtr<USplineMeshComponent>> AimingSplineMeshArray;

	/** Aiming sphere used when a player aiming */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Aiming", meta = (BlueprintProtected))
	TObjectPtr<UStaticMeshComponent> AimingSphereComponent = nullptr;

	/** Initiate and activate aiming point */
	UFUNCTION(Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void InitAimingSphere();

public:
	/** Obtain aiming static mesh (currently it's sphere component */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem] | Aiming")
	FORCEINLINE USplineComponent* GetAimingSplineComponent() const { return AimingSplineComponent; }

	/** Add a new spline mesh component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem] | Aiming")
	void AddAimingSplineMeshComponent(USplineMeshComponent* SplineMeshComponent);

	/** Obtain aiming static mesh (currently it's sphere component */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem] | Aiming")
	FORCEINLINE UStaticMeshComponent* GetAimingSphereComponent() const { return AimingSphereComponent; }

	/** Hide spline elements (trajectory) */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearTrajectorySplines();
};
