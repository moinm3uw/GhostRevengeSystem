// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsPawnComponent.h"

// Grs
#include "GhostRevengeSystemRuntimeModule.h"
/* @PR JanSeliv [Coding Standards] - unused includes, remove, type never referenced in cpp: GrsPlayerStateComponent, BmrCellUtilsLibrary,
 * AbilitySystemComponent, AbilitySystemGlobals. Latter two only transitively pull FGameplayEventData, include GameplayEffectTypes.h instead. Applies across file */
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GrsGameplayTags.h"
#include "GrsUtils.h"
#include "LevelActors/GrsPawn.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Structures/BmrGameplayTags.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

// @PR JanSeliv [Coding Standards] - own .h has reflection, UE_INLINE_GENERATED_CPP_BY_NAME must be active after includes + 1 empty line, currently commented out
#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawnComponent)

// @PR JanSeliv [Coding Standards] - no forward-declare in .cpp, GRSWorldSubSystem.h already included, remove redundant decl
class UGRSWorldSubSystem;
// Sets default values for this component's properties
UGrsPawnComponent::UGrsPawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

//  Returns BmrPawn of this component
ABmrPawn* UGrsPawnComponent::GetBmrPawn() const
{
	return Cast<ABmrPawn>(GetOwner());
}

ABmrPawn& UGrsPawnComponent::GetBmrPawnChecked() const
{
	// @PR JanSeliv [Coding Standards] - MyBmrPawn dereferenced at return, declare as reference with Ref suffix not pointer, e.g `ABmrPawn& BmrPawnRef`
	ABmrPawn* MyBmrPawn = GetBmrPawn();
	// @PR JanSeliv [Coding Standards] - use %hs with __FUNCTION__ directly, drop %s + *FString() wrap
	checkf(MyBmrPawn, TEXT("%s: 'MyBmrPawn' is null"), *FString(__FUNCTION__));
	return *MyBmrPawn;
}

// Called when the game starts
void UGrsPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	// @PR JanSeliv [Coding Standards] - GetOwner() deref without null check, applies across file log lines, cache owner and guard
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem::Get().RegisterPawnComponent(this);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_PawnReady, this, &ThisClass::Player_PawnReady);
}

// Clears all transient data created by this component
void UGrsPawnComponent::OnUnregister()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	UGRSWorldSubSystem::Get().UnRegisterPawnComponent(this);

	UPoolManagerSubsystem* PoolManager = UPoolManagerSubsystem::GetPoolManager();
	if (PoolManager
	    && !GrsPawnPoolManagerHandlers.IsEmpty())
	{
		// @PR JanSeliv [Coding Standards] - iterate FPoolObjectHandle struct element by const&, copies handle each loop
		for (FPoolObjectHandle GrsPoolObjectHandle : GrsPawnPoolManagerHandlers)
		{
			const FPoolObjectData& SpawnObject = PoolManager->FindPoolObjectByHandle(GrsPoolObjectHandle);
			if (SpawnObject.IsValid())
			{
				PoolManager->ReturnToPool(GrsPoolObjectHandle);
			}
		}

		GrsPawnPoolManagerHandlers.Empty();
	}

	// @PR JanSeliv [Coding Standards] - duplicate UnRegisterPawnComponent call, already invoked above this func, remove redundant call
	UGRSWorldSubSystem::Get().UnRegisterPawnComponent(this);

	Super::OnUnregister();
}

// Event that fires when any pawn is spawned, possessed, and replicated. Is a ready trigger for this component to listen whole module to be ready
// @PR JanSeliv [Coding Standards] - drop elaborated specifier `struct` in .cpp def, include FGameplayEventData header use plain type, applies across file to OnInitialize
void UGrsPawnComponent::Player_PawnReady(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	// @PR JanSeliv [Coding Standards] - reuse existing GetBmrPawn() accessor, never re-Cast<ABmrPawn>(GetOwner()) inline
	const ABmrPawn* OwnerBmrPawn = Cast<ABmrPawn>(GetOwner());
	const ABmrPawn* InstigatorPawn = Cast<ABmrPawn>(Payload.Instigator);

	if (OwnerBmrPawn == InstigatorPawn)
	{
		UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
	}
}

// A pawn could be loaded/replicated faster than GFP is fully loaded therefore waiting for whole module to be initialized is required
void UGrsPawnComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// @PR JanSeliv [Coding Standards] - GetOwner() deref in control-flow branch without null check, not covered by log-line systemic note, cache owner and guard
	if (GetOwner()->HasAuthority())
	{
		AddGhostCharacter();
	}
}

// Spawn ghost character when a module is initialized
void UGrsPawnComponent::AddGhostCharacter()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	// --- Return to Pool Manager items first as they are no longer needed
	if (!GrsPawnPoolManagerHandlers.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(GrsPawnPoolManagerHandlers);
		GrsPawnPoolManagerHandlers.Empty();
	}

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	// @PR JanSeliv [Coding Standards] - local lambda shadows member func same name OnTakeGrsPawnsFromPoolCompleted, name lambda distinct, neighbor GrsCollisionComponent uses OnTakeActorsFromPoolCompleted lambda vs OnTakeCollisionActorsFromPoolCompleted member
	const FOnSpawnAllCallback OnTakeGrsPawnsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsPawnComponent* This = WeakThis.Get())
		{
			This->OnTakeGrsPawnsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	// @PR JanSeliv [Coding Standards] - magic literal `1` for Amount, extract to named constexpr, neighbor TakeFromPoolArray always passes named count never bare literal
	UPoolManagerSubsystem::Get().TakeFromPoolArray(GrsPawnPoolManagerHandlers, UGRSDataAsset::Get().GetGrsActorClass(), 1, OnTakeGrsPawnsFromPoolCompleted, ESpawnRequestPriority::High);
}

//  Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGrsPawnComponent::OnTakeGrsPawnsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedGhostPawns)
{
	// @PR JanSeliv [Coding Standards] - GetBmrPawn() return deref without null check, use existing GetBmrPawnChecked() accessor, applies across file (also line below in loop)
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs ( %s ) PlayerID: %i"), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), GetBmrPawn()->GetPlayerId());
	// --- Setup spawned characters
	// @PR JanSeliv [Coding Standards] - cache GetBmrPawn()->GetPlayerId() into local once before loop, re-fetched per iteration below and at log above
	for (const FPoolObjectData& CreatedGhostPawn : CreatedGhostPawns)
	{
		AGrsPawn& GhostCharacter = CreatedGhostPawn.GetChecked<AGrsPawn>();

		GhostCharacter.InitPawn(GetBmrPawn()->GetPlayerId());
		GhostCharacter.SetActorLocation(UGrsUtils::MaxPos);
	}
}
