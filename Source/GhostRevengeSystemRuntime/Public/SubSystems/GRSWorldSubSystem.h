// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// GFPM
#include "Subsystems/GfpmWorldSubsystem.h"

#include "GRSWorldSubSystem.generated.h"

enum class EGRSCharacterSide : uint8;
enum class EBmrEndGameState : uint8;
class AActor;
class ABmrPawn;
class AGrsPawn;
class UGrsCharacterManagerComponent;
class UGrsPawnComponent;
class UGrsCollisionComponent;

/**
 * Implements the world subsystem to act as singleton with access to different components in the module.
 * Manages GFP overall loading status.
 * Manages also if a player character (BmrPawn) is revivable or not. A player character can be revived only once per game round, resets revived players when game starts (game state changes to InGame)
 * Manages available spot (left or right side) for GrsPawn on spawn. Only 1 grs allowed per side
 */
/* @PR JanSeliv [Architecture] - god-object subsystem fuses 5 unrelated jobs into one non-replicated singleton every component hard-depends on: GFP load orchestration, revive-once rules, ghost side allocation, side-collision lifecycle, Bmr HUD visibility.
 * Split per NMM: thin readiness broker, revive and side state on replicated PlayerState, side allocation own owner, collision lifecycle into GrsCollisionComponent, drop UI entirely */
UCLASS(BlueprintType, Blueprintable)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSWorldSubSystem : public UGfpmWorldSubsystem
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Subsystem's Lifecycle
	 **********************************************************************************************/

public:
	/** Returns this Subsystem, is checked and will crash if it can't be obtained.*/
	static UGRSWorldSubSystem& Get();

protected:
	/** Subscribes to local pawn ready event */
	virtual void OnGameFeatureInitialize_Implementation() override;

	/** Clears all transient data created by this subsystem */
	virtual void OnGameFeatureDeinitialize_Implementation() override;

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	/** Checks if all components present and invokes initialization */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryInit();

	/** Cleanup used on unloading module to remove properties that should not be available by other objects. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void PerformCleanUp();

public:
	/** Checks if the system is ready to load */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	bool IsReady() const;

	/*********************************************************************************************
	 * Side Collisions actors
	 **********************************************************************************************/
protected:
	/** Current Collision Manager Component used to identify if GFP is ready to be loaded */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Collision Manager Component"))
	TObjectPtr<UGrsCollisionComponent> CollisionManagerComponent;

	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<AActor> LeftSideCollision;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<AActor> RightSideCollision;

public:
	/** Register collision manager component used to track if all components loaded and GFP ready to initialize */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void RegisterCollisionManagerComponent(UGrsCollisionComponent* NewCollisionManagerComponent);

	/** Add spawned collision actors to be cached */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void AddCollisionActor(AActor* Actor);

	/** Returns TRUE if collision are spawned */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	bool IsCollisionsSpawned() const;

	/** Returns left side spawned collision or nullptr */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE AActor* GetLeftCollisionActor() const { return LeftSideCollision; }

	/** Returns right side spawned collision or nullptr */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE AActor* GetRightCollisionActor() const { return RightSideCollision; }

	/** Clears cached collision manager component */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void UnregisterCollisionManagerComponent();

	/** Clear cached collisions */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearCollisions();

protected:
	/** Contains list of player characters that were eliminated at least once per game(round) and character can't be a ghost anymore */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Revied Player Character"))
	TArray<TObjectPtr<ABmrPawn>> RevivedPlayerCharacters;

public:
	/** Checks if the target Player was already revived. Player can be revived only once
	 * @param PlayerToRevive The BmrPawn to revive
	 * @return false if player was revived once in game */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	bool IsRevivable(const ABmrPawn* PlayerToRevive) const;

	/** Set a player character as it was revived once */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void SetRevivedPlayer(ABmrPawn* PlayerToRevive);

	/** Reset revived players so they can be ghosts again */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void ResetRevivedPlayers();

	/*********************************************************************************************
	 * Ghost Characters
	 **********************************************************************************************/
protected:
	/** Current Character Manager Component */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]", meta = (BluePrintProtected))
	TObjectPtr<UGrsCharacterManagerComponent> CharacterManagerComponent;

	/** Ghost character spawned on left side of the map */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BluePrintProtected))
	TObjectPtr<AGrsPawn> GhostCharacterLeftSide;

	/** Ghost character spawned on right side of the map */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BluePrintProtected))
	TObjectPtr<AGrsPawn> GhostCharacterRightSide;

public:
	/** Register character manager component. */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void RegisterCharacterManagerComponent(UGrsCharacterManagerComponent* NewCharacterManagerComponent);

	/** Register character manager component. */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UGrsCharacterManagerComponent* GetCharacterManagerComponent() const { return CharacterManagerComponent; }

	/** Register ghost character to obtain it's side NONE if all sides occupied  */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	EGRSCharacterSide RegisterGhostCharacter(AGrsPawn* GhostPlayerCharacter);

	/*********************************************************************************************
	 * Pawn Component
	 **********************************************************************************************/
protected:
	/** Pawn Components attached to BmrPawn to track Pawn's state change */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TArray<TObjectPtr<UGrsPawnComponent>> PawnComponents;

public:
	/** Register a new Pawn component to track the pawn state */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void RegisterPawnComponent(UGrsPawnComponent* NewPawnComponent);

	/** Clears the registered pawn component once it deleted  */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void UnregisterPawnComponent(UGrsPawnComponent* PawnComponentToUnregister);

	/** Clears cached character manager component. */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void UnregisterCharacterManagerComponent();

	/** Clear cached ghost character by reference */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void UnregisterGhostCharacter(AGrsPawn* GhostPlayerCharacter);

	/** Clear cached ghost character references */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void ClearGhostCharacters();

	/*********************************************************************************************
	 * Treasury (temp)
	 **********************************************************************************************/
protected:
	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);

	/** Listen end game states to show/hide HUD temporarry */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EBmrEndGameState EndGameState);

	/** Changes the Bmr HUD visibility */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void ChangeHUDEndResultVisibility(bool bVisibility);
	
	/** Find and return a textblock element responsible for the end game result */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	class UTextBlock* GetTextBlockToHide(FName ResultTextBlockName);
};
