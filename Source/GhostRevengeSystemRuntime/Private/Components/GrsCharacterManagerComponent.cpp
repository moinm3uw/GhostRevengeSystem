// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsCharacterManagerComponent.h"

// Grs
#include "Data/GRSDataAsset.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
// @PR JanSeliv [Coding Standards] - DalSubsystem from DataAssetsLoader plugin miscategorized under Bmr group, move under own `// DataAssetsLoader` marker like sibling cpp
#include "DalSubsystem.h"

// UE
// @PR JanSeliv [Coding Standards] - unused include, GrsPawnComponent not referenced in cpp, remove it. Also Grs type miscategorized under UE group
#include "Components/GrsPawnComponent.h"
// @PR JanSeliv [Coding Standards] - unused include, World nor GetWorld referenced in cpp, remove it
#include "Engine/World.h"
// @PR JanSeliv [Coding Standards] - own module header, provides LogGrs, misgrouped under UE marker. Move to own plugin group right after own .h, UE group is engine headers only
#include "GhostRevengeSystemRuntimeModule.h"

// @PR JanSeliv [Coding Standards] - cpp has reflection in own .h, uncomment UE_INLINE_GENERATED_CPP_BY_NAME, commented-out form disables it. Applies across module
// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsCharacterManagerComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGrsCharacterManagerComponent::UGrsCharacterManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	/* @PR JanSeliv [Architecture] - component only does BeginPlay -> ListenForDataAsset -> RegisterCharacterManagerComponent, owns no data and manages no characters, only product is presence to pass readiness count plus warming data asset everyone gets via UGRSDataAsset::Get(), also SetIsReplicatedByDefault(true) with zero replicated props.
	 * Delete component, base readiness on real participants, at minimum drop SetIsReplicatedByDefault(true) like sibling GrsCollisionComponent sets false */
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
