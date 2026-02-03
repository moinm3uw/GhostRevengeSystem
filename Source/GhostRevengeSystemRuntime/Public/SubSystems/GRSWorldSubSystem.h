// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "Subsystems/WorldSubsystem.h"

#include "GRSWorldSubSystem.generated.h"

/**
 * Implements the world subsystem to access different components in the module
 */
UCLASS(BlueprintType, Blueprintable, Config = "GhostRevengeSystem", DefaultConfig)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSWorldSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGRSOnInitialize);

	/** Returns this Subsystem, is checked and will crash if it can't be obtained.*/
	static UGRSWorldSubSystem& Get();
	static UGRSWorldSubSystem& Get(const UObject& WorldContextObject);

	/** Is called to initialize the world subsystem. It's a BeginPlay logic for the GRS module */
	UFUNCTION(BlueprintNativeEvent, Category = "C++", meta = (BlueprintProtected))
	void OnWorldSubSystemInitialize();

	/** Clears all transient data created by this subsystem. */
	virtual void Deinitialize() override;

	/** Cleanup used on unloading module to remove properties that should not be available by other objects. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void PerformCleanUp();

	/* Delegate to inform that module is loaded. To have better loading control of the MGF  */
	UPROPERTY(BlueprintAssignable, Transient, Category = "C++")
	FGRSOnInitialize OnInitialize;

	/** Returns the data asset that contains all the assets of Ghost Revenge System game feature.
	 * @see UGRSWorldSubsystem::GRSDataAssetInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const class UGRSDataAsset* GetGRSDataAsset() const;

	/** Return a ghost character. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	class AGRSPlayerCharacter* GetGhostPlayerCharacter();

	/** Register character manager component. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void RegisterCharacterManagerComponent(class UGRSGhostCharacterManagerComponent* CharacterManagerComponent);

	/** Register collision manager component used to track if all components loaded and MGF ready to initialize */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void RegisterCollisionManagerComponent(class UGhostRevengeCollisionComponent* NewCollisionManagerComponent);

	/** Register ghost character */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void RegisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter);

	/** Register character manager component. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	FORCEINLINE class UGRSGhostCharacterManagerComponent* GetGRSCharacterManagerComponent() const { return CharacterMangerComponent; }

	/** Returns the left side ghost player character */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	FORCEINLINE class AGRSPlayerCharacter* GetGRSPlayerCharacterLeftSide() const { return GhostCharacterLeftSide; }

	/** Returns the left side ghost player character */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	FORCEINLINE class AGRSPlayerCharacter* GetGRSPlayerCharacterRightSide() const { return GhostCharacterRightSide; }

	/** Returns last activated ghost character to enable input context */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	FORCEINLINE class AGRSPlayerCharacter* GetLastActivatedGhostCharacter() const { return LastActivatedGhostCharacter; }

	/** Set the last activated ghost character */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void SetLastActivatedGhostCharacter(AGRSPlayerCharacter* GhostCharacter);

	/** Returns the side of the ghost character (left, or right)  */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	EGRSCharacterSide GetGhostPlayerCharacterSide(AGRSPlayerCharacter* PlayerCharacter);

	/** Returns currently available ghost character or nullptr if there is no available ghosts. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	class AGRSPlayerCharacter* GetAvailableGhostCharacter();

protected:
	/** Contains all the assets and tweaks of Ghost Revenge System game feature.
	 * Note: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.
	 * Property is put to subsystem because its instance is created before any other object.
	 * It can't be put to DevelopSettings class because it does work properly for MGF-modules. */
	UPROPERTY(Config, VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Data Asset"))
	TSoftObjectPtr<const class UGRSDataAsset> DataAssetInternal;

	/** Current Character Manager Component */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++")
	TObjectPtr<class UGRSGhostCharacterManagerComponent> CharacterMangerComponent;

	/** Current Collision Manager Component used to identify if MGF is ready to be loaded */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++")
	TObjectPtr<class UGhostRevengeCollisionComponent> CollisionMangerComponent;

	/** Ghost character spawned on left side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "C++")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterLeftSide;

	/** Ghost character spawned on right side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "C++")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterRightSide;

	/** Last activated ghost character */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AGRSPlayerCharacter> LastActivatedGhostCharacter;

	/** Begin play of the subsystem */
	void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Checks if all components present and invokes initialization */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void TryInit();

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalPawnReady(class ABmrPawn* PlayerCharacter, int32 CharacterID);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(EBmrCurrentGameState CurrentGameState);

	/*********************************************************************************************
	 * Side Collisions actors
	 **********************************************************************************************/
public:
	/** Add spawned collision actors to be cached */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void AddCollisionActor(class AActor* Actor);

	/** Returns TRUE if collision are spawned */
	UFUNCTION(BlueprintCallable, Category = "C++")
	bool IsCollisionsSpawned();

	/** Returns left side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "C++")
	class AActor* GetLeftCollisionActor();

	/** Returns right side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "C++")
	class AActor* GetRightCollisionActor();

protected:
	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<class AActor> LeftSideCollisionInternal;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AActor> RightSideCollisionInternal;
};
