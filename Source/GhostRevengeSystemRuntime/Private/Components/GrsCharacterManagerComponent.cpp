// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsCharacterManagerComponent.h"

// Grs
#include "Data/GRSDataAsset.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "DalSubsystem.h"

// UE
#include "Components/GrsPawnComponent.h"
#include "Engine/World.h"
#include "GhostRevengeSystemRuntimeModule.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsCharacterManagerComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGrsCharacterManagerComponent::UGrsCharacterManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGrsCharacterManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	UDalSubsystem::Get().ListenForDataAsset<UGRSDataAsset>(this, &ThisClass::OnDataAssetLoaded);
}

// Called when the GRS data asset is loaded and available
void UGrsCharacterManagerComponent::OnDataAssetLoaded_Implementation(const UGRSDataAsset* DataAsset)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	UGRSWorldSubSystem::Get().RegisterCharacterManagerComponent(this);
}
