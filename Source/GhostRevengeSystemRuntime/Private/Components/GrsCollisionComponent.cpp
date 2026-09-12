// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// GRS
#include "Components/GrsCollisionComponent.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawn.h" // EGRSCharacterSide
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

namespace GrsSideCollisions
{
	/** Sides of the map that have to be bounded, exactly one collision actor is spawned per each of them. */
	static constexpr EGRSCharacterSide Sides[] = {EGRSCharacterSide::Left, EGRSCharacterSide::Right};

	/** Number of side collisions to spawn, is driven by the sides that have to be bounded. */
	static constexpr int32 Num = UE_ARRAY_COUNT(Sides);
}

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

	ClearCollisions();

	// --- perform clean up from subsystem GFP is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get().UnregisterCollisionManagerComponent();
}

/*********************************************************************************************
 * Side Collisions actors
 **********************************************************************************************/

// Returns TRUE if collision are spawned
bool UGrsCollisionComponent::IsCollisionsSpawned() const
{
	const bool bIsSpawned = LeftSideCollisionInternal && RightSideCollisionInternal;
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, bIsSpawned ? TEXT("TRUE") : TEXT("FALSE"));
	return bIsSpawned;
}

// Returns spawned collisions back to the pool they were taken from and clears cached references
void UGrsCollisionComponent::ClearCollisions()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// Collisions are pooled actors owned by this component, so they are released back to the pool instead of being destroyed
	if (!CollisionPoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}

	LeftSideCollisionInternal = nullptr;
	RightSideCollisionInternal = nullptr;
}

// Caches the spawned collision actor as the one that bounds given side of the map
void UGrsCollisionComponent::SetCollisionActorBySide(EGRSCharacterSide Side, AActor* CollisionActor)
{
	switch (Side)
	{
	case EGRSCharacterSide::Left:
		LeftSideCollisionInternal = CollisionActor;
		break;
	case EGRSCharacterSide::Right:
		RightSideCollisionInternal = CollisionActor;
		break;
	default:
		ensureMsgf(false, TEXT("ASSERT: [%i] %hs:\n'Side' has to be Left or Right to bound the map!"), __LINE__, __FUNCTION__);
		break;
	}
}

// Returns the world location where the collision actor has to be placed to bound given side of the map
FVector UGrsCollisionComponent::GetCollisionLocationBySide(EGRSCharacterSide Side)
{
	// Distance from the center of the corner cell, so the collision is placed right outside of the level
	static constexpr float DistanceFromCorner = FBmrCell::CellSize + FBmrCell::CellSize / 2.f;

	switch (Side)
	{
	case EGRSCharacterSide::Left:
	{
		const FBmrCell CornerCell = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
		return FVector(CornerCell.Location.X - DistanceFromCorner, 0.f, 0.f);
	}
	case EGRSCharacterSide::Right:
	{
		const FBmrCell CornerCell = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
		return FVector(CornerCell.Location.X + DistanceFromCorner, 0.f, 0.f);
	}
	default:
		ensureMsgf(false, TEXT("ASSERT: [%i] %hs:\n'Side' has to be Left or Right to bound the map!"), __LINE__, __FUNCTION__);
		return FVector::ZeroVector;
	}
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
	if (!IsCollisionsSpawned())
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
	UPoolManagerSubsystem::Get().TakeFromPoolArray(CollisionPoolActorHandlersInternal, UGRSDataAsset::Get().GetCollisionsAssetClass(), GrsSideCollisions::Num, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
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

	if (!ensureMsgf(CreatedObjects.Num() == GrsSideCollisions::Num, TEXT("ASSERT: [%i] %hs:\n'CreatedObjects' contains %i objects while %i side collisions are expected!"), __LINE__, __FUNCTION__, CreatedObjects.Num(), GrsSideCollisions::Num))
	{
		return;
	}

	// Spawn side collision
	const FTransform& CollisionTransform = UGRSDataAsset::Get().GetCollisionTransform();
	for (int32 Index = 0; Index < GrsSideCollisions::Num; ++Index)
	{
		AActor& SpawnedCollision = CreatedObjects[Index].GetChecked<AActor>();
		SpawnedCollision.SetOwner(PlayerController);

		const EGRSCharacterSide Side = GrsSideCollisions::Sides[Index];

		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs %s: --- %s "), __LINE__, __FUNCTION__, CurrentOwner->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *SpawnedCollision.GetName());

		FTransform NewCollisionTransform = CollisionTransform;
		NewCollisionTransform.SetLocation(GetCollisionLocationBySide(Side));
		SpawnedCollision.SetActorTransform(NewCollisionTransform);

		SetCollisionActorBySide(Side, &SpawnedCollision);
	}
}
