// Copyright (c) Yevhenii Selivanov

#include "Components/GhostRevengeCollisionComponent.h"

// GRS
#include "Data/GRSDataAsset.h"
#include "PoolManagerSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Controllers/BmrPlayerController.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrGameplayMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// UE
#include "Abilities/GameplayAbilityTypes.h"
#include "Net/UnrealNetwork.h"

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
	BIND_ON_LOCAL_PAWN_READY(this, ThisClass::OnLocalPawnReady);
}

// Is called when local player character is ready to guarantee that they player controller is initialized
void UGhostRevengeCollisionComponent::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
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
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

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
	APlayerController* PlayerController = UBmrBlueprintFunctionLibrary::GetLocalPlayerController(this);
	if (!ensureMsgf(PlayerController, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Spawn side collision
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AActor& SpawnedCollision = CreatedObject.GetChecked<AActor>();
		SpawnedCollision.SetOwner(PlayerController);
		UE_LOG(LogTemp, Log, TEXT("Spawned collision --- %s - %s"), *SpawnedCollision.GetName(), SpawnedCollision.HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

		// base cell for the calculation
		FBmrCell SpawnLocation;

		// calculate the distance from the center of current cell
		float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

		if (!UGRSWorldSubSystem::Get().GetLeftCollisionActor())
		{
			SpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
			SpawnLocation.Location.X = SpawnLocation.Location.X - CellSize;
		}
		else if (!UGRSWorldSubSystem::Get().GetRightCollisionActor())
		{
			SpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
			SpawnLocation.Location.X = SpawnLocation.Location.X + CellSize;
		}

		UGRSWorldSubSystem::Get().AddCollisionActor(&SpawnedCollision);

		FTransform CollisionTransfrom = UGRSDataAsset::Get().GetCollisionTransform();
		CollisionTransfrom.SetLocation(FVector(SpawnLocation.Location.X, 0, 0));
		SpawnedCollision.SetActorTransform(CollisionTransfrom);
	}
}
