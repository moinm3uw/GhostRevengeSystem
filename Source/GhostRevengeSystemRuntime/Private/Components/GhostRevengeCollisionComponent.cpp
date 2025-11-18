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

	// Binds to local character ready to guarantee that the player controller is initialized
	// so we can safely use Widget's Subsystem
	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

// Is called when local player character is ready to guarantee that they player controller is initialized
void UGhostRevengeCollisionComponent::OnLocalCharacterReady_Implementation(class APlayerCharacter* Character, int32 CharacterID)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.OnInitialize.AddUniqueDynamic(this, &ThisClass::OnInitialize);
	WorldSubsystem.RegisterCollisionManagerComponent(this);
	WorldSubsystem.OnWorldSubSystemInitialize();
}

// Clears all transient data created by this component.
void UGhostRevengeCollisionComponent::OnUnregister()
{
	Super::OnUnregister();

	if (CollisionPoolActorHandlersInternal.Num() > 0)
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
		UPoolManagerSubsystem::Get().EmptyPool(UGRSDataAsset::Get().GetCollisionsAssetClass());
	}
}

// The spawner is considered as loaded only when the subsystem is loaded
void UGhostRevengeCollisionComponent::OnInitialize()
{
	// spawn collisions only once
	if (!UGRSWorldSubSystem::Get().IsCollisionsSpawned())
	{
		SpawnMapCollisionOnSide();
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
