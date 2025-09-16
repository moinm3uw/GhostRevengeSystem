// Copyright (c) Yevhenii Selivanov

#include "Components/GhostRevengeCollisionComponent.h"

#include "Controllers/MyPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "PoolManagerSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GhostRevengeCollisionComponent)

// Sets default values for this component's properties
UGhostRevengeCollisionComponent::UGhostRevengeCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGhostRevengeCollisionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UGRSWorldSubSystem::Get().OnInitialize.AddUniqueDynamic(this, &ThisClass::OnInitialize);
}

// The spawner is considered as loaded only when the subsystem is loaded
void UGhostRevengeCollisionComponent::OnInitialize()
{
	// Listen to handle input for each game state
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
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
		case ECurrentGameState::Menu:
		{
			// potentially do not needed logic. Remove later.
			// RemoveMapCollisionOnSide();
			break;
		}
		case ECurrentGameState::GameStarting:
		{
			// check if collision spawned, ignore if yes.
			if (!UGRSWorldSubSystem::Get().IsCollisionsSpawned())
			{
				SpawnMapCollisionOnSide();
			}
			break;
		}
		default:
			break;
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

//  Spawn a collision box the side of the map
void UGhostRevengeCollisionComponent::SpawnMapCollisionOnSide_Implementation()
{
	// --- Only server can spawn character and posses it
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Clean previous
	// RemoveMapCollisionOnSide(); // potentially do not needed logic. Remove later.

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
	UPoolManagerSubsystem::Get().TakeFromPoolArray(CollisionPoolActorHandlersInternal, UGRSDataAsset::Get().GetCollisionsAssetClass(), 2, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a side collision asset from the pool manager (Object pooling patter)
void UGhostRevengeCollisionComponent::OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	APlayerController* PlayerController = UMyBlueprintFunctionLibrary::GetLocalPlayerController(this);
	if (!ensureMsgf(PlayerController, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Spawn side collision
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AActor& SpawnedCollision = CreatedObject.GetChecked<AActor>();
		SpawnedCollision.SetOwner(PlayerController);
		UE_LOG(LogTemp, Warning, TEXT("Spawned collision --- %s - %s"), *SpawnedCollision.GetName(), SpawnedCollision.HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

		// base cell for the calculation
		FCell SpawnLocation;

		// calculate the distance from the center of current cell
		float CellSize = FCell::CellSize + (FCell::CellSize / 2);

		if (!UGRSWorldSubSystem::Get().GetLeftCollisionActor())
		{
			SpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopLeft);
			SpawnLocation.Location.X = SpawnLocation.Location.X - CellSize;
		}
		else if (!UGRSWorldSubSystem::Get().GetRightCollisionActor())
		{
			SpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopRight);
			SpawnLocation.Location.X = SpawnLocation.Location.X + CellSize;
		}

		UGRSWorldSubSystem::Get().AddCollisionActor(&SpawnedCollision);

		SpawnedCollision.SetActorTransform(UGRSDataAsset::Get().GetCollisionTransform());
		SpawnedCollision.SetActorLocation(FVector(SpawnLocation.Location.X, 0, 0));
	}
}
