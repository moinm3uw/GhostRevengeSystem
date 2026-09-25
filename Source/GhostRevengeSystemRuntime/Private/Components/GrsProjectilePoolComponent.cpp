// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Components/GrsProjectilePoolComponent.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "GrsGameplayTags.h"
#include "LevelActors/GrsBombProjectile.h" // GetProjectileClass()

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsProjectilePoolComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGrsProjectilePoolComponent::UGrsProjectilePoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Called when the game starts
void UGrsProjectilePoolComponent::BeginPlay()
{
	Super::BeginPlay();

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// Clears all transient data created by this component
void UGrsProjectilePoolComponent::OnUnregister()
{
	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	// Projectiles that are still being prepared are released back, pool itself is emptied by Pool Manager on GFP unload
	if (!ProjectilePoolActorHandlersInternal.IsEmpty())
	{
		if (UPoolManagerSubsystem* PoolManager = UPoolManagerSubsystem::GetPoolManager())
		{
			PoolManager->ReturnToPoolArray(ProjectilePoolActorHandlersInternal);
		}
		ProjectilePoolActorHandlersInternal.Empty();
	}

	bIsProjectilePoolPrepared = false;

	Super::OnUnregister();
}

/*********************************************************************************************
 * Projectiles Pool
 **********************************************************************************************/

// Starting point once whole module is ready(loaded) to be initialized, prepares projectiles in the pool on server
void UGrsProjectilePoolComponent::OnInitialize_Implementation(const FGameplayEventData& Payload)
{
	const AActor* CurrentOwner = GetOwner();
	checkf(CurrentOwner, TEXT("[%i] %hs 'CurrentOwner' is null"), __LINE__, __FUNCTION__);

	// Pool is server-only, clients receive replicated projectiles; is prepared only once since the pool survives between matches
	const bool bIsBeingPrepared = !ProjectilePoolActorHandlersInternal.IsEmpty();
	if (!CurrentOwner->HasAuthority()
	    || bIsProjectilePoolPrepared
	    || bIsBeingPrepared)
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) --- "), __LINE__, __FUNCTION__);

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsProjectilePoolComponent* This = WeakThis.Get())
		{
			This->OnTakeProjectilesFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actors
	const UGRSDataAsset& GrsDataAsset = UGRSDataAsset::Get();
	UPoolManagerSubsystem::Get().TakeFromPoolArray(ProjectilePoolActorHandlersInternal, GrsDataAsset.GetProjectileClass(), GrsDataAsset.GetProjectilePoolSize(), OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Puts prepared projectiles back to the pool, so throws take ready ones without spawning
void UGrsProjectilePoolComponent::OnTakeProjectilesFromPoolCompleted_Implementation(const TArray<FPoolObjectData>& CreatedObjects)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Prepared %i projectiles"), __LINE__, __FUNCTION__, CreatedObjects.Num());

	// Returning hides them and disables their tick
	UPoolManagerSubsystem::Get().ReturnToPoolArray(ProjectilePoolActorHandlersInternal);
	ProjectilePoolActorHandlersInternal.Empty();

	bIsProjectilePoolPrepared = true;
}
