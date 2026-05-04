// Copyright (c) Yevhenii Selivanov

#include "Components/GRSGhostCharacterManagerComponent.h"

// GRS
#include "Data/GRSDataAsset.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "DalSubsystem.h"

// UE
#include "Components/GrsPawnComponent.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSGhostCharacterManagerComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGRSGhostCharacterManagerComponent::UGRSGhostCharacterManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGRSGhostCharacterManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- BeginPlay"), __LINE__, __FUNCTION__);
	UDalSubsystem::Get().ListenForDataAsset<UGRSDataAsset>(this, &ThisClass::OnDataAssetLoaded);
}

// Called when the GRS data asset is loaded and available
void UGRSGhostCharacterManagerComponent::OnDataAssetLoaded_Implementation(const UGRSDataAsset* DataAsset)
{
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- OnDataAssetLoaded_Implementation RegisterCharacterManagerComponent"), __LINE__, __FUNCTION__);
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.RegisterCharacterManagerComponent(this);
}

// Clears all transient data created by this component.
void UGRSGhostCharacterManagerComponent::OnUnregister()
{
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- OnUnregister UnregisterCharacterManagerComponent"), __LINE__, __FUNCTION__);
	Super::OnUnregister();

	// --- perform clean up from subsystem MGF is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get().UnregisterCharacterManagerComponent();
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/
