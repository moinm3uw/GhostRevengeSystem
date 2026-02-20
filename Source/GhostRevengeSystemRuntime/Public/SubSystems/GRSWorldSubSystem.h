// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GRSWorldSubSystem.generated.h"

enum class EGRSCharacterSide : uint8;

/**
 * Implements the world subsystem to access different components in the module
 */
UCLASS(BlueprintType, Blueprintable, Config = "GhostRevengeSystem", DefaultConfig)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSWorldSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Subsystem's Lifecycle
	 **********************************************************************************************/

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGRSOnInitialize);

	/** Returns this Subsystem, is checked and will crash if it can't be obtained.*/
	static UGRSWorldSubSystem& Get();
	static UGRSWorldSubSystem& Get(const UObject& WorldContextObject);

	/* Delegate to inform that module is loaded. To have better loading control of the MGF  */
	UPROPERTY(BlueprintAssignable, Transient, Category = "[GhostRevengeSystem]")
	FGRSOnInitialize OnInitialize;

protected:
	/** Begin play of the subsystem */
	void OnWorldBeginPlay(UWorld& InWorld) override;

public:
	/** Is called to initialize the world subsystem. It's a BeginPlay logic for the GRS module */
	UFUNCTION(BlueprintNativeEvent, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnWorldSubSystemInitialize();

protected:
	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);

	/** Checks if all components present and invokes initialization */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void TryInit();

public:
	/** Clears all transient data created by this subsystem. */
	virtual void Deinitialize() override;

	/** Cleanup used on unloading module to remove properties that should not be available by other objects. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void PerformCleanUp();

	/*********************************************************************************************
	 * Data asset
	 **********************************************************************************************/
protected:
	/** Contains all the assets and tweaks of Ghost Revenge System game feature.
	 * Note: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.
	 * Property is put to subsystem because its instance is created before any other object.
	 * It can't be put to DevelopSettings class because it does work properly for MGF-modules. */
	UPROPERTY(Config, VisibleInstanceOnly, BlueprintReadWrite, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Data Asset"))
	TSoftObjectPtr<const class UGRSDataAsset> DataAssetInternal;

public:
	/** Returns the data asset that contains all the assets of Ghost Revenge System game feature.
	 * @see UGRSWorldSubsystem::GRSDataAssetInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]")
	const class UGRSDataAsset* GetGRSDataAsset() const;

	/*********************************************************************************************
	 * Side Collisions actors
	 **********************************************************************************************/
protected:
	/** Current Collision Manager Component used to identify if MGF is ready to be loaded */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]")
	TObjectPtr<class UGhostRevengeCollisionComponent> CollisionMangerComponent;

	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<class AActor> LeftSideCollision;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AActor> RightSideCollision;

public:
	/** Register collision manager component used to track if all components loaded and MGF ready to initialize */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterCollisionManagerComponent(class UGhostRevengeCollisionComponent* NewCollisionManagerComponent);

	/** Add spawned collision actors to be cached */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void AddCollisionActor(class AActor* Actor);

	/** Returns TRUE if collision are spawned */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	bool IsCollisionsSpawned();

	/** Returns left side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE class AActor* GetLeftCollisionActor() const { return LeftSideCollision ? LeftSideCollision : nullptr; }

	/** Returns right side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	FORCEINLINE class AActor* GetRightCollisionActor() const { return RightSideCollision ? RightSideCollision : nullptr; }

	/** Clears cached collision manager component */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void UnregisterCollisionManagerComponent();

	/** Clear cached collisions */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearCollisions();

	/*********************************************************************************************
	 * Ghost Characters
	 **********************************************************************************************/
protected:
	/** Current Character Manager Component */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "[GhostRevengeSystem]")
	TObjectPtr<class UGRSGhostCharacterManagerComponent> CharacterManagerComponent;

	/** Ghost character spawned on left side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "[GhostRevengeSystem]")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterLeftSide;

	/** Ghost character spawned on right side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "[GhostRevengeSystem]")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterRightSide;

public:
	/** Register character manager component. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterCharacterManagerComponent(class UGRSGhostCharacterManagerComponent* NewCharacterManagerComponent);

	/** Register character manager component. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	FORCEINLINE class UGRSGhostCharacterManagerComponent* GetGRSCharacterManagerComponent() const { return CharacterManagerComponent; }

	/** Register ghost character */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void RegisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Returns currently available ghost character or nullptr if there is no available ghosts. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	class AGRSPlayerCharacter* GetAvailableGhostCharacter();

	/** Clears cached character manager component. */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void UnregisterCharacterManagerComponent();

	/** Clear cached ghost character by reference */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void UnregisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Clear cached ghost character references */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	void ClearGhostCharacters();

	/*********************************************************************************************
	 * Treasury (temp)
	 **********************************************************************************************/
protected:
	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	void OnGameStateChanged(const struct FGameplayEventData& Payload);
};
