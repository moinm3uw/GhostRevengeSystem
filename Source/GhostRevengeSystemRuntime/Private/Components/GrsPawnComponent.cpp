// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsPawnComponent.h"

// Grs
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawn.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"
#include "Structures/BmrGameplayTags.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GhostRevengeSystemRuntimeModule.h"
#include "GrsUtils.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawnComponent)

class UGRSWorldSubSystem;
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
	ABmrPawn* MyBmrPawn = GetBmrPawn();
	checkf(MyBmrPawn, TEXT("%s: 'MyBmrPawn' is null"), *FString(__FUNCTION__));
	return *MyBmrPawn;
}

// Called when the game starts
void UGrsPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get(this);
	WorldSubsystem.RegisterPawnComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_PawnReady, this, &ThisClass::Player_PawnReady);
}

// Clears all transient data created by this component
void UGrsPawnComponent::OnUnregister()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	UGRSWorldSubSystem::Get(this).UnRegisterPawnComponent(this);

	UPoolManagerSubsystem* PoolManager = UPoolManagerSubsystem::GetPoolManager();
	if (PoolManager
	    && !GrsPawnPoolManagerHandlers.IsEmpty())
	{
		for (FPoolObjectHandle GrsPoolObjectHandle : GrsPawnPoolManagerHandlers)
		{
			const FPoolObjectData& SpawnObject = PoolManager->FindPoolObjectByHandle(GrsPoolObjectHandle);
			if (SpawnObject.IsValid())
			{
				PoolManager->ReturnToPool(GrsPoolObjectHandle);
			}
		}

		GrsPawnPoolManagerHandlers.Empty();
	}

	UGRSWorldSubSystem::Get(this).UnRegisterPawnComponent(this);

	Super::OnUnregister();
}

// Event that fires when any pawn is spawned, possessed, and replicated. Is a ready trigger for this component to listen whole module to be ready
void UGrsPawnComponent::Player_PawnReady(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	const ABmrPawn* OwnerBmrPawn = Cast<ABmrPawn>(GetOwner());
	const ABmrPawn* InstigatorPawn = Cast<ABmrPawn>(Payload.Instigator);

	if (OwnerBmrPawn == InstigatorPawn)
	{
		UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
	}
}

// A pawn could be loaded/replicated faster than GFP is fully loaded therefore waiting for whole module to be initialized is required
void UGrsPawnComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (GetOwner()->HasAuthority())
	{
		AddGhostCharacter();
	}
}

// Spawn ghost character when a module is initialized
void UGrsPawnComponent::AddGhostCharacter()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	// --- Return to Pool Manager items first as they are no longer needed
	if (!GrsPawnPoolManagerHandlers.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(GrsPawnPoolManagerHandlers);
		GrsPawnPoolManagerHandlers.Empty();
	}

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeGrsPawnsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsPawnComponent* This = WeakThis.Get())
		{
			This->OnTakeGrsPawnsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(GrsPawnPoolManagerHandlers, UGRSDataAsset::Get().GetGrsActorClass(), 1, OnTakeGrsPawnsFromPoolCompleted, ESpawnRequestPriority::High);
}

//  Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGrsPawnComponent::OnTakeGrsPawnsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedGhostPawns)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs ( %s ) PlayerID: %i"), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), GetBmrPawn()->GetPlayerId());
	// --- Setup spawned characters
	for (const FPoolObjectData& CreatedGhostPawn : CreatedGhostPawns)
	{
		AGrsPawn& GhostCharacter = CreatedGhostPawn.GetChecked<AGrsPawn>();

		GhostCharacter.InitPawn(GetBmrPawn()->GetPlayerId());
		GhostCharacter.SetActorLocation(UGrsUtils::MaxPos);
	}
}
