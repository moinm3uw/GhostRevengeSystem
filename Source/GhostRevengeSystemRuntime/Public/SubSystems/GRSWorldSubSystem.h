// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
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

protected:
	/** Contains all the assets and tweaks of Ghost Revenge System game feature.
	 * Note: Since Subsystem is code-only, there is config property set in BaseGhostRevengeSystem.ini.
	 * Property is put to subsystem because its instance is created before any other object.
	 * It can't be put to DevelopSettings class because it does work properly for MGF-modules. */
	//UPROPERTY(Config, VisibleInstanceOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Revenge System Data Asset"))
	//TSoftObjectPtr<const class UGRSDataAsset> DataAssetInternal;
	
	/** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
