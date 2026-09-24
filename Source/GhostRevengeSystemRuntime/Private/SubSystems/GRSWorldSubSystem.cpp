// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

// GRS
#include "Components/GrsCollisionComponent.h"
#include "Components/GrsPawnComponent.h"
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h"
#include "GrsGameplayTags.h"

// DataAssetsLoader
#include "DalSubsystem.h"

// Bmr
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"

// MyEditorUtils
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSWorldSubSystem)

/*********************************************************************************************
 * Subsystem's Lifecycle
 **********************************************************************************************/

// Returns this Subsystem, is checked and will crash if it can't be obtained
UGRSWorldSubSystem& UGRSWorldSubSystem::Get()
{
	const UWorld* World = UUtilsLibrary::GetPlayWorld();
	checkf(World, TEXT("'World' is null [%i] %hs"), __LINE__, __FUNCTION__);
	UGRSWorldSubSystem* ThisSubsystem = World->GetSubsystem<ThisClass>();
	checkf(ThisSubsystem, TEXT("[%i] %hs: 'GRSWorldSubSystem' is null"), __LINE__, __FUNCTION__);
	return *ThisSubsystem;
}

// Called when the owning game feature plugin activates (loaded by Game feature plugin manager)
// Waits for the data asset and subscribes to local pawn ready event
void UGRSWorldSubSystem::OnGameFeatureInitialize_Implementation()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	UDalSubsystem::Get().ListenForDataAsset<UGRSDataAsset>(this, &ThisClass::OnDataAssetLoaded);
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSWorldSubSystem::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Checks if all components present and invokes initialization
void UGRSWorldSubSystem::TryInit()
{
	// --- check if managers have characters and collisions spawned if not - broadcast, yes -> return
	if (IsReady())
	{
		FGameplayEventData EventData;
		EventData.EventTag = GrsGameplayTags::Event::GameFeaturePluginReady;
		UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
	}
}

// Checks if the system is ready to load.
//  Currently strictly tied to FBmrGameStateTag::InGame and expected module to be loaded/unloaded
bool UGRSWorldSubSystem::IsReady() const
{
	const ABmrGameState& GameState = ABmrGameState::Get();
	bool bisReady = bIsDataAssetLoaded
	                && CollisionManagerComponent
	                && PawnComponents.Num() == GrsMaxPlayers
	                && GameState.HasMatchingGameplayTag(FBmrGameStateTag::InGame);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, bisReady ? TEXT("READY") : TEXT("NOT READY"));
	return bisReady;
}

// Clears all transient data created by this subsystem
// Called when the owning game feature plugin deactivates (unloaded by Game feature plugin manager)
void UGRSWorldSubSystem::OnGameFeatureDeinitialize_Implementation()
{
	PerformCleanUp();
}

// Cleanup used on unloading module to remove properties that should not be available by other objects.
void UGRSWorldSubSystem::PerformCleanUp()
{
	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	// Clear cached GameFeaturePluginReady so late-binding listeners receive fresh data on GRS load
	UGlobalMessageSubsystem::ClearCachedMessages(GrsGameplayTags::Event::GameFeaturePluginReady);

	bIsDataAssetLoaded = false;
	UnregisterCollisionManagerComponent();
}

/*********************************************************************************************
 * Collision Component
 **********************************************************************************************/

// Register collision manager component used to track if all components loaded and GFP ready to initialize
void UGRSWorldSubSystem::RegisterCollisionManagerComponent(UGrsCollisionComponent* NewCollisionManagerComponent)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	if (!ensureMsgf(NewCollisionManagerComponent != CollisionManagerComponent, TEXT("ASSERT: [%i] %hs:\n'CollisionMangerComponent' is being overriden twice!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	if (NewCollisionManagerComponent)
	{
		CollisionManagerComponent = NewCollisionManagerComponent;
	}

	TryInit(); // try to initialize
}

// Clears cached collision manager component
void UGRSWorldSubSystem::UnregisterCollisionManagerComponent()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	CollisionManagerComponent = nullptr;
}

/*********************************************************************************************
 * Data Asset
 **********************************************************************************************/

// Called when the GRS data asset is loaded and available
void UGRSWorldSubSystem::OnDataAssetLoaded_Implementation(const UGRSDataAsset* DataAsset)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	bIsDataAssetLoaded = true;
	TryInit();
}

/*********************************************************************************************
 * Pawn Component
 **********************************************************************************************/

// Register a new Pawn component to track the pawn state
void UGRSWorldSubSystem::RegisterPawnComponent(UGrsPawnComponent* NewPawnComponent)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs:\n'"), __LINE__, __FUNCTION__);
	if (!NewPawnComponent)
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s %i"), __LINE__, __FUNCTION__, NewPawnComponent->GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), PawnComponents.Num());

	if (!ensureMsgf(PawnComponents.Num() < GrsMaxPlayers, TEXT("ASSERT: [%i] %hs:\n'PawnComponents' is more than expected!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PawnComponents.AddUnique(NewPawnComponent);
	TryInit();
}

// Clears the registered pawn component once it deleted
void UGRSWorldSubSystem::UnregisterPawnComponent(UGrsPawnComponent* PawnComponentToUnregister)
{
	if (!PawnComponentToUnregister || PawnComponents.IsEmpty())
	{
		return;
	}

	PawnComponents.Remove(PawnComponentToUnregister);
}

// Listen game states to try initializing the GFP once the match starts
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	bool bHasInGameTag = Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame);
	if (bHasInGameTag)
	{
		// Revive state is reset by each UGrsPlayerStateComponent that listens for the same game state change
		TryInit();
	}
}
