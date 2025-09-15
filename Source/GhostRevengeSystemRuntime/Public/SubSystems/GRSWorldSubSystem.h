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

	/** Returns the side of the ghost character (left, or right)  */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	EGRSCharacterSide GetGhostPlayerCharacterSide(AGRSPlayerCharacter* PlayerCharacter);

	/** Broadcasts the activation of ghost character, can be called from outside */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void ActivateGhostCharacter(APlayerCharacter* PlayerCharacter);

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

	/** Ghost character spawned on left side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "C++")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterLeftSide;

	/** Ghost character spawned on right side of the map */
	UPROPERTY(VisibleDefaultsOnly, Category = "C++")
	TObjectPtr<class AGRSPlayerCharacter> GhostCharacterRightSide;

	/** Begin play of the subsystem */
	void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Listen game states to switch character skin. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/*********************************************************************************************
	 * Side Collisions actors
	 **********************************************************************************************/
public:
	/** Add spawned collision actors to be cached */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void AddCollisionActor(class AActor* Actor);

	/** Returns TRUE if collision are spawned */
	UFUNCTION(BlueprintCallable, Category = "C++")
	bool IsCollisionSpawned();

	/** Returns left side spawned collision or nullptr */
	UFUNCTION(BlueprintCallable, Category = "C++")
	class AActor* GetLeftCollisionActor();

protected:
	/** Left Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Left Side Collision"))
	TObjectPtr<class AActor> LeftSideCollisionInternal;

	/** Right Side collision */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Right Side Collision"))
	TObjectPtr<class AActor> RightSideCollisionInternal;
};
