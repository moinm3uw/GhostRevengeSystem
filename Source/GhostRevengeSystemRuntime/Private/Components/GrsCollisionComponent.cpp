// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsCollisionComponent.h"

// GRS
#include "Data/GRSDataAsset.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Controllers/BmrPlayerController.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "GrsGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsCollisionComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGrsCollisionComponent::UGrsCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(false);
}

// Called when the game starts
void UGrsCollisionComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- BeginPlay"), __LINE__, __FUNCTION__);
	UE_LOG(LogTemp, Log, TEXT("--- %s - %s"), *this->GetName(), GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	// Binds to local character ready to guarantee that the player controller is initialized
	// so we can safely use Widget's Subsystem
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);
}

// Clears all transient data created by this component.
void UGrsCollisionComponent::OnUnregister()
{
	Super::OnUnregister();

	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- OnUnregister"), __LINE__, __FUNCTION__);
	UE_LOG(LogTemp, Log, TEXT("--- CollisionPoolActorHandlersInternal Count: %d - %s"), CollisionPoolActorHandlersInternal.Num(), GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	if (CollisionPoolActorHandlersInternal.Num() > 0)
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}

	// --- perform clean up from subsystem MGF is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get().ClearCollisions();
	UGRSWorldSubSystem::Get().UnregisterCollisionManagerComponent();
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/

// Is called when local player character is ready to guarantee that they player controller is initialized
void UGrsCollisionComponent::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.RegisterCollisionManagerComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The spawner is considered as loaded only when the subsystem is loaded
void UGrsCollisionComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	// spawn collisions only once
	if (!UGRSWorldSubSystem::Get().IsCollisionsSpawned())
	{
		SpawnMapCollisionOnSide();
	}
}

//  Spawn a collision box the side of the map
void UGrsCollisionComponent::SpawnMapCollisionOnSide()
{
	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsCollisionComponent* This = WeakThis.Get())
		{
			This->OnTakeCollisionActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(CollisionPoolActorHandlersInternal, UGRSDataAsset::Get().GetCollisionsAssetClass(), 2, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a side collision asset from the pool manager (Object pooling patter)
void UGrsCollisionComponent::OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
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
