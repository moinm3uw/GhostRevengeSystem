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
#include "GhostRevengeSystemRuntimeModule.h"
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
	
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	// Binds to local character ready to guarantee that the player controller is initialized
	// so we can safely use Widget's Subsystem
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);
}

// Clears all transient data created by this component.
void UGrsCollisionComponent::OnUnregister()
{
	Super::OnUnregister();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	if (CollisionPoolActorHandlersInternal.Num() > 0)
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}

	// --- perform clean up from subsystem GFP is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get(this).ClearCollisions();
	UGRSWorldSubSystem::Get(this).UnregisterCollisionManagerComponent();
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/

// Is called when local player character is ready to guarantee that they player controller is initialized
void UGrsCollisionComponent::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get(this);
	WorldSubsystem.RegisterCollisionManagerComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The spawner is considered as loaded only when the subsystem is loaded
void UGrsCollisionComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	// spawn collisions only once
	if (!UGRSWorldSubSystem::Get(this).IsCollisionsSpawned())
	{
		SpawnMapCollisionOnSide();
	}
}

//  Spawn a collision box the side of the map
void UGrsCollisionComponent::SpawnMapCollisionOnSide()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
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
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

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
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *SpawnedCollision.GetName());
		// base cell for the calculation
		FBmrCell SpawnLocation;

		// calculate the distance from the center of current cell
		float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

		if (!UGRSWorldSubSystem::Get(this).GetLeftCollisionActor())
		{
			SpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
			SpawnLocation.Location.X = SpawnLocation.Location.X - CellSize;
		}
		else if (!UGRSWorldSubSystem::Get(this).GetRightCollisionActor())
		{
			SpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
			SpawnLocation.Location.X = SpawnLocation.Location.X + CellSize;
		}

		UGRSWorldSubSystem::Get(this).AddCollisionActor(&SpawnedCollision);

		FTransform CollisionTransfrom = UGRSDataAsset::Get().GetCollisionTransform();
		CollisionTransfrom.SetLocation(FVector(SpawnLocation.Location.X, 0, 0));
		SpawnedCollision.SetActorTransform(CollisionTransfrom);
	}
}
