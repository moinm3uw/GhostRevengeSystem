// Copyright (c) Yevhenii Selivanov

#include "Components/GrsPawnComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrPawn.h"
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawn.h"
#include "PoolManagerSubsystem.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

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

/*
// Returns GrsPlayerStateComponent obtaining from pawn's player id
UGrsPlayerStateComponent* UGrsPawnComponent::GetGrsPlayerStateComponent() const
{
    return UGRSWorldSubSystem::Get().GetPlayerStateComponent(GetBmrPawn()->GetPlayerId());
}


UGrsPlayerStateComponent* UGrsPawnComponent::GetGrsPlayerStateComponentChecked() const
{
    UGrsPlayerStateComponent* GrsPlayerStateComponent = GetGrsPlayerStateComponent();
    checkf(GrsPlayerStateComponent, TEXT("%s: 'GrsPlayerStateComponent' is null"), *FString(__FUNCTION__));
    return GrsPlayerStateComponent;
}
*/

// Called when the game starts
void UGrsPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("UGrsPawnComponent::BeginPlay  --- %s - %s"), *this->GetName(), GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.RegisterPawnComponent(this);
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// Clears all transient data created by this component
void UGrsPawnComponent::OnUnregister()
{
	UGRSWorldSubSystem::Get().UnRegisterPawnComponent(this);

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

	UGRSWorldSubSystem::Get().UnRegisterPawnComponent(this);

	Super::OnUnregister();
}

// A pawn could be loaded/replicated faster than MGF(GFP) is fully loaded therefore waiting for whole module to be initialized is required
void UGrsPawnComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	if (GetOwner()->HasAuthority())
	{
		AddGhostCharacter();
	}
}

void UGrsPawnComponent::AddGhostCharacter()
{
	// --- Return to Pool Manager the list of handles which is not needed (if there are any)
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
			This->OnTakeActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(GrsPawnPoolManagerHandlers, UGRSDataAsset::Get().GetGrsActorClass(), 1, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

//  Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGrsPawnComponent::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// --- Setup spawned characters
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGrsPawn& GhostCharacter = CreatedObject.GetChecked<AGrsPawn>();
		UE_LOG(LogTemp, Log, TEXT("Spawned ghost character --- %s - %s"), *GhostCharacter.GetName(), GhostCharacter.HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

		GhostCharacter.InitPawn(GetBmrPawn()->GetPlayerId());
		GhostCharacter.RegisterPawnComponent(this);

		FBmrCell ActorSpawnLocation;
		float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
		ActorSpawnLocation.Location.Z = GetBmrPawn()->GetActorLocation().Z; // temporary, debug row

		GhostCharacter.SetActorLocation(ActorSpawnLocation);
	}
}
