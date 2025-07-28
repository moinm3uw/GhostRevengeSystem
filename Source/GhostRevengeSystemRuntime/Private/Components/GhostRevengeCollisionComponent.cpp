// Copyright (c) Yevhenii Selivanov


#include "Components/GhostRevengeCollisionComponent.h"

#include "PoolManagerSubsystem.h"
#include "Components/MouseActivityComponent.h"
#include "DataAssets/GeneratedMapDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GhostRevengeCollisionComponent)

// Sets default values for this component's properties
UGhostRevengeCollisionComponent::UGhostRevengeCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Called when the game starts
void UGhostRevengeCollisionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Listen to handle input for each game state
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Returns properties that are replicated for the lifetime of the actor channel.
void UGhostRevengeCollisionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	if (LeftSideCollisionInternal)
	{
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LeftSideCollisionInternal, Params);
	}

	if (RightSideCollisionInternal)
	{
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RightSideCollisionInternal, Params);
	}
}

//  Spawn a collision box the side of the map
void UGhostRevengeCollisionComponent::SpawnMapCollisionOnSide_Implementation()
{
	// --- Only server can spawn character and posses it
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// --- Return to Pool Manager the list of handles which is not needed (if there are any) 
	/*
	if (!CollisionPoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}
	*/

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGhostRevengeCollisionComponent* This = WeakThis.Get())
		{
			This->OnTakeCollisionActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(CollisionPoolActorHandlersInternal, UGeneratedMapDataAsset::Get().GetCollisionsAssetClass(), 2, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a side collision asset from the pool manager (Object pooling patter)
void UGhostRevengeCollisionComponent::OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// Spawn side collision
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AActor& SpawnedCollision = CreatedObject.GetChecked<AActor>();
		SpawnedCollision.SetReplicates(true);

		// base cell for the calculation
		FCell SpawnLocation;

		// calculate the distance from the center of current cell 
		float CellSize = FCell::CellSize + (FCell::CellSize / 2);

		if (!LeftSideCollisionInternal)
		{
			SpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopLeft);
			SpawnLocation.Location.X = SpawnLocation.Location.X - CellSize;
			LeftSideCollisionInternal = SpawnedCollision;
		}
		else
		{
			SpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopRight);
			SpawnLocation.Location.X = SpawnLocation.Location.X + CellSize;
			RightSideCollisionInternal = SpawnedCollision;
		}

		SpawnedCollision.SetActorTransform(UGRSDataAsset::Get().GetCollisionTransform());
		SpawnedCollision.SetActorLocation(FVector(SpawnLocation.Location.X, 0, 0));
	}
}

// Remove a collision box the sides of the map
void UGhostRevengeCollisionComponent::RemoveMapCollisionOnSide()
{
	// --- Return to pool character
	if (!CollisionPoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}
}


// Called when game state is changed.
void UGhostRevengeCollisionComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	// --- Only server can spawn character and posses it
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	switch (CurrentGameState)
	{
	case ECurrentGameState::GameStarting:
		{
			SpawnMapCollisionOnSide();
			break;
		}
	case ECurrentGameState::Menu:
		{
			RemoveMapCollisionOnSide();
			break;
		}
	default:
		break;
	}
}
