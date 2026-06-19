// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsCollisionComponent.h"

// GRS
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h"
#include "GrsGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
// @PR JanSeliv [Coding Standards] - unused include, .cpp uses plain APlayerController not ABmrPlayerController, drop it
#include "Controllers/BmrPlayerController.h"
// @PR JanSeliv [Coding Standards] - unused include, no ABmrGameState reference in this .cpp, drop it
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
// @PR JanSeliv [Coding Standards] - unused include, no UGameplayStatics use in this .cpp, drop it
#include "Kismet/GameplayStatics.h"
// @PR JanSeliv [Coding Standards] - unused include, no DOREPLIFETIME in this .cpp, drop it
#include "Net/UnrealNetwork.h"

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

	// @PR JanSeliv [Coding Standards] - GetOwner() deref without null check, applies across file log lines, cache owner and guard
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

	// @PR JanSeliv [Coding Standards] - use !IsEmpty() not Num() > 0
	if (CollisionPoolActorHandlersInternal.Num() > 0)
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
// @PR JanSeliv [Coding Standards] - include `Abilities/GameplayAbilityTypes.h` in UE group, never rely on transitive from GlobalMessageSubsystem.h
void UGrsCollisionComponent::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem::Get().RegisterCollisionManagerComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The spawner is considered as loaded only when the subsystem is loaded
// @PR JanSeliv [Coding Standards] - drop elaborated `struct` specifier in .cpp, use plain FGameplayEventData like OnLocalPawnReady_Implementation
void UGrsCollisionComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	// spawn collisions only once
	if (!UGRSWorldSubSystem::Get().IsCollisionsSpawned())
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
	// @PR JanSeliv [Coding Standards] - magic literal 2 (left + right sides), extract to constexpr var
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
		// @PR JanSeliv [Coding Standards] - mark const, use float literal 2.f not int 2 in float math
		float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

		// @PR JanSeliv [Coding Standards] - UGRSWorldSubSystem::Get() called repeatedly per loop iteration, cache to local ref once like OnUnregister `WorldSubsystem`
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

		// @PR JanSeliv [Coding Standards] - typo `CollisionTransfrom` local, rename to CollisionTransform across all 3 uses
		FTransform CollisionTransfrom = UGRSDataAsset::Get().GetCollisionTransform();
		// @PR JanSeliv [Coding Standards] - int literals in FVector ctor, use float literals 0.f not 0
		CollisionTransfrom.SetLocation(FVector(SpawnLocation.Location.X, 0, 0));
		SpawnedCollision.SetActorTransform(CollisionTransfrom);
	}
}
