// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// GRS
#include "Components/GrsCollisionComponent.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "GrsGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Controllers/BmrPlayerController.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsCollisionComponent)

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

	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	// Binds to local character ready to guarantee that the player controller is initialized
	// so we can safely use Widget's Subsystem
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);
}

// Clears all transient data created by this component.
void UGrsCollisionComponent::OnUnregister()
{
	Super::OnUnregister();

	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	if (!CollisionPoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}

	// --- perform clean up from subsystem GFP is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.ClearCollisions();
	WorldSubsystem.UnregisterCollisionManagerComponent();
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/

// Is called when local player character is ready to guarantee that they player controller is initialized
void UGrsCollisionComponent::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGRSWorldSubSystem::Get().RegisterCollisionManagerComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The spawner is considered as loaded only when the subsystem is loaded
void UGrsCollisionComponent::OnInitialize_Implementation(const FGameplayEventData& Payload)
{
	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	// spawn collisions only once
	if (!UGRSWorldSubSystem::Get().IsCollisionsSpawned())
	{
		SpawnMapCollisionOnSide();
	}
}

//  Spawn a collision box the side of the map
void UGrsCollisionComponent::SpawnMapCollisionOnSide()
{
	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

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
	// @PR JanSeliv [Coding Standards] - magic literal 2 (left + right sides), extract to constexpr var
	UPoolManagerSubsystem::Get().TakeFromPoolArray(CollisionPoolActorHandlersInternal, UGRSDataAsset::Get().GetCollisionsAssetClass(), 2, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a side collision asset from the pool manager (Object pooling patter)
void UGrsCollisionComponent::OnTakeCollisionActorsFromPoolCompleted_Implementation(const TArray<FPoolObjectData>& CreatedObjects)
{
	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'MyBmrPawn' is null"), __LINE__, __FUNCTION__);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	APlayerController* PlayerController = Cast<APlayerController>(UBmrBlueprintFunctionLibrary::GetLocalPlayerController(this));
	if (!ensureMsgf(PlayerController, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Spawn side collision
	UGRSWorldSubSystem& GrsWorldSubSystem = UGRSWorldSubSystem::Get();
	const UGRSDataAsset& GrsDataAsset = UGRSDataAsset::Get();
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AActor& SpawnedCollision = CreatedObject.GetChecked<AActor>();
		SpawnedCollision.SetOwner(PlayerController);

		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *SpawnedCollision.GetName());

		// base cell for the calculation
		FBmrCell SpawnLocation;

		// calculate the distance from the center of current cell
		const float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2.0f);

		if (!GrsWorldSubSystem.GetLeftCollisionActor())
		{
			SpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
			SpawnLocation.Location.X = SpawnLocation.Location.X - CellSize;
		}
		else if (!GrsWorldSubSystem.GetRightCollisionActor())
		{
			SpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
			SpawnLocation.Location.X = SpawnLocation.Location.X + CellSize;
		}

		GrsWorldSubSystem.AddCollisionActor(&SpawnedCollision);

		FTransform CollisionTransform = GrsDataAsset.GetCollisionTransform();
		CollisionTransform.SetLocation(FVector(SpawnLocation.Location.X, 0.0f, 0.0f));
		SpawnedCollision.SetActorTransform(CollisionTransform);
	}
}
