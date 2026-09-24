// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// GFPM
#include "Subsystems/GfpmWorldSubsystem.h"

#include "GRSWorldSubSystem.generated.h"

class UGrsPawnComponent;
class UGrsCollisionComponent;

/**
 * Implements the world subsystem to act as singleton with access to different components in the module.
 * Manages GFP overall loading status.
 */

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
	
	/** Called when the owning game feature plugin activates (loaded by Game feature plugin manager)
	 * Waits for the data asset and subscribes to local pawn ready event */
	virtual void OnGameFeatureInitialize_Implementation() override;
	
	/** Called when the owning game feature plugin deactivates (unloaded by Game feature plugin manager)
	 * Clears all transient data created by this subsystem */
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
	 * Data Asset
	 **********************************************************************************************/
protected:
	/** Is set once the GRS data asset is loaded.
	 * GFP can't be ready before that, since UGRSDataAsset::Get() crashes on an unloaded asset. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	bool bIsDataAssetLoaded = false;

	/** Called when the GRS data asset is loaded and available. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnDataAssetLoaded(const class UGRSDataAsset* DataAsset);

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

protected:
	/** Listen game states to try initializing the GFP once the match starts */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);
};
