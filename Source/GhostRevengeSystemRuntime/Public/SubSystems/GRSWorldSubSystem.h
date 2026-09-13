// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// GFPM
#include "Subsystems/GfpmWorldSubsystem.h"

#include "GRSWorldSubSystem.generated.h"

enum class EBmrEndGameState : uint8;
class UGrsCharacterManagerComponent;
class UGrsPawnComponent;
class UGrsCollisionComponent;

/**
 * Implements the world subsystem to act as singleton with access to different components in the module.
 * Manages GFP overall loading status.
 */
/* @PR JanSeliv [Architecture] - god-object subsystem fuses 5 unrelated jobs into one non-replicated singleton every component hard-depends on: GFP load orchestration, revive-once rules, ghost side allocation, side-collision lifecycle, Bmr HUD visibility.
 * Split per NMM: thin readiness broker, side state on replicated PlayerState, side allocation own owner, collision lifecycle into GrsCollisionComponent, drop UI entirely */

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
	/** Checks if the system is ready to load.
	 * Currently strictly tied to FBmrGameStateTag::InGame and expected module to be loaded/unloaded on game start */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	bool IsReady() const;

	/*********************************************************************************************
	 * Collision Component
	 * Spawns and owns the side collisions itself, is tracked here only to know when GFP is ready.
	 * @see UGrsCollisionComponent
	 **********************************************************************************************/
protected:
	/** Current Collision Manager Component used to identify if GFP is ready to be loaded */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Collision Manager Component"))
	TObjectPtr<UGrsCollisionComponent> CollisionManagerComponent;

public:
	/** Register collision manager component used to track if all components loaded and GFP ready to initialize */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void RegisterCollisionManagerComponent(UGrsCollisionComponent* NewCollisionManagerComponent);

	/** Clears cached collision manager component */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void UnregisterCollisionManagerComponent();

	/** Returns currently registered collision manager component or nullptr if it's not registered yet */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UGrsCollisionComponent* GetCollisionManagerComponent() const { return CollisionManagerComponent; }

	/*********************************************************************************************
	 * Ghost Characters
	 **********************************************************************************************/
protected:
	/** Current Character Manager Component */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]", meta = (BluePrintProtected))
	TObjectPtr<UGrsCharacterManagerComponent> CharacterManagerComponent;

public:
	/** Register character manager component. */
	UFUNCTION(Category = "[GhostRevengeSystem]")
	void RegisterCharacterManagerComponent(UGrsCharacterManagerComponent* NewCharacterManagerComponent);

	/** Register character manager component. */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UGrsCharacterManagerComponent* GetCharacterManagerComponent() const { return CharacterManagerComponent; }

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
