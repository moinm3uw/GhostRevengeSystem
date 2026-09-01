// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsPawnComponent.h"

// Grs
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h"
#include "GrsGameplayTags.h"
#include "GrsUtils.h"
#include "LevelActors/GrsPawn.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Structures/BmrGameplayTags.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "GameplayEffectTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawnComponent)

// Sets default values for this component's properties
UGrsPawnComponent::UGrsPawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

//  Returns BmrPawn of this component
ABmrPawn* UGrsPawnComponent::GetBmrPawn() const
{
	return Cast<ABmrPawn>(GetOwner());
}

ABmrPawn& UGrsPawnComponent::GetBmrPawnChecked() const
{
	ABmrPawn& MyBmrPawnRef = *GetBmrPawn();
	checkf(&MyBmrPawnRef, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);
	return MyBmrPawnRef;
}

// Called when the game starts
void UGrsPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* CurrentOwner = GetOwner();
	if (CurrentOwner)
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	}

	UGRSWorldSubSystem::Get().RegisterPawnComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_PawnReady, this, &ThisClass::OnPawnReady);
}

// Clears all transient data created by this component
void UGrsPawnComponent::OnUnregister()
{
	const AActor* CurrentOwner = GetOwner();
	if (CurrentOwner)
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	}

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	UGRSWorldSubSystem::Get().UnregisterPawnComponent(this);

	UPoolManagerSubsystem* PoolManager = UPoolManagerSubsystem::GetPoolManager();
	if (PoolManager
	    && !GrsPawnPoolManagerHandlers.IsEmpty())
	{
		for (FPoolObjectHandle& GrsPoolObjectHandle : GrsPawnPoolManagerHandlers)
		{
			const FPoolObjectData& SpawnObject = PoolManager->FindPoolObjectByHandle(GrsPoolObjectHandle);
			if (SpawnObject.IsValid())
			{
				PoolManager->ReturnToPool(GrsPoolObjectHandle);
			}
		}

		GrsPawnPoolManagerHandlers.Empty();
	}

	Super::OnUnregister();
}

// Event that fires when any pawn is spawned, possessed, and replicated. Is a ready trigger for this component to listen whole module to be ready
void UGrsPawnComponent::OnPawnReady(const FGameplayEventData& Payload)
{
	const AActor* CurrentOwner = GetOwner();
	if (CurrentOwner)
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	}

	const ABmrPawn* OwnerBmrPawn = GetBmrPawn();
	const ABmrPawn* InstigatorPawn = Cast<ABmrPawn>(Payload.Instigator);

	if (OwnerBmrPawn == InstigatorPawn)
	{
		UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
	}
}

// A pawn could be loaded/replicated faster than GFP is fully loaded therefore waiting for whole module to be initialized is required
void UGrsPawnComponent::OnInitialize_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	const AActor* CurrentOwner = GetOwner();
	if (!ensureMsgf(CurrentOwner, TEXT("ASSERT: [%i] %hs:\n'CurrentOwner' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	if (CurrentOwner->HasAuthority())
	{
		AddGhostCharacter();
	}
}

// Spawn ghost character when a module is initialized
void UGrsPawnComponent::AddGhostCharacter()
{
	const AActor* CurrentOwner = GetOwner();
	if (CurrentOwner)
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	}

	// --- Return to Pool Manager items first as they are no longer needed
	if (!GrsPawnPoolManagerHandlers.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(GrsPawnPoolManagerHandlers);
		GrsPawnPoolManagerHandlers.Empty();
	}

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsPawnComponent* This = WeakThis.Get())
		{
			This->OnTakeGrsPawnsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	constexpr int32 AmountOfActorsToSpawn = 1;
	UPoolManagerSubsystem::Get().TakeFromPoolArray(GrsPawnPoolManagerHandlers, UGRSDataAsset::Get().GetGrsActorClass(), AmountOfActorsToSpawn, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

//  Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGrsPawnComponent::OnTakeGrsPawnsFromPoolCompleted_Implementation(const TArray<FPoolObjectData>& CreatedGhostPawns)
{
	const int32 CurrentPlayerId = GetBmrPawnChecked().GetPlayerId();
	const AActor* CurrentOwner = GetOwner();
	if (CurrentOwner)
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs ( %s ) PlayerID: %i"), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), CurrentPlayerId);
	}

	// --- Setup spawned characters
	for (const FPoolObjectData& CreatedGhostPawn : CreatedGhostPawns)
	{
		AGrsPawn& GhostCharacter = CreatedGhostPawn.GetChecked<AGrsPawn>();

		GhostCharacter.InitPawn(CurrentPlayerId);
		GhostCharacter.SetActorLocation(UGrsUtils::MaxPos);
	}
}
