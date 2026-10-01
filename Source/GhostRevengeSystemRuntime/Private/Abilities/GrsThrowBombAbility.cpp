// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Abilities/GrsThrowBombAbility.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsBombProjectile.h"
#include "LevelActors/GrsPawn.h"

// PoolManager
#include "PoolManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsThrowBombAbility)

// Sets default values for this ability
UGrsThrowBombAbility::UGrsThrowBombAbility()
{
	// Instance is kept alive to receive the pool callback
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// Returns true if this ability is triggered by given gameplay event, the trigger is set in the blueprint child
bool UGrsThrowBombAbility::IsTriggeredByEvent(const FGameplayTag& EventTag) const
{
	return AbilityTriggers.ContainsByPredicate([&EventTag](const FAbilityTriggerData& Trigger)
	{
		return Trigger.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent
		       && Trigger.TriggerTag == EventTag;
	});
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Actually activate ability, do not call this directly
void UGrsThrowBombAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	checkf(ActorInfo, TEXT("ERROR: [%i] %hs:\n'ActorInfo' is null!"), __LINE__, __FUNCTION__);
	checkf(TriggerEventData, TEXT("ERROR: [%i] %hs:\n'TriggerEventData' is null!"), __LINE__, __FUNCTION__);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	// Only the server takes the projectile, clients receive it replicated
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ false, /*bWasCancelled*/ false);
		return;
	}

	// Instigator of the event is the ghost pawn, event data keeps it const
	AGrsPawn* Thrower = const_cast<AGrsPawn*>(Cast<AGrsPawn>(TriggerEventData->Instigator.Get()));
	if (!ensureMsgf(Thrower, TEXT("ASSERT: [%i] %hs:\n'Thrower' is not a ghost in the throw event!"), __LINE__, __FUNCTION__))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	// Charge is sent as magnitude, client value is clamped, so a longer charge than allowed can't be sent
	const float HoldTime = FMath::Clamp(TriggerEventData->EventMagnitude, 0.f, UGRSDataAsset::Get().GetMaxChargingTime());

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s, HoldTime: %f"), __LINE__, __FUNCTION__, *GetNameSafe(Thrower), HoldTime);

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const TWeakObjectPtr<AGrsPawn> WeakThrower = Thrower;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis, WeakThrower, HoldTime](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsThrowBombAbility* This = WeakThis.Get())
		{
			This->OnTakeProjectileFromPoolCompleted(CreatedObjects, WeakThrower.Get(), HoldTime);
		}
	};

	// --- Take actor; handle is not kept since the projectile returns itself to the pool on landing
	TArray<FPoolObjectHandle> ProjectileHandles;
	constexpr int32 AmountOfProjectilesToTake = 1;
	UPoolManagerSubsystem::Get().TakeFromPoolArray(ProjectileHandles, UGRSDataAsset::Get().GetProjectileClass(), AmountOfProjectilesToTake, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

/*********************************************************************************************
 * Throw
 ********************************************************************************************* */

// Starts the flight of the projectile taken from the pool
void UGrsThrowBombAbility::OnTakeProjectileFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects, AGrsPawn* Thrower, float HoldTime)
{
	if (!ensureMsgf(CreatedObjects.IsValidIndex(0), TEXT("ASSERT: [%i] %hs:\n'CreatedObjects' is empty, projectile is not taken from the pool!"), __LINE__, __FUNCTION__))
	{
		K2_EndAbility();
		return;
	}

	// Ghost could be gone while the pool was spawning the projectile, or its arc can't be calculated, then nothing is thrown and the projectile is released back
	const FPoolObjectData& CreatedProjectile = CreatedObjects[0];
	if (!Thrower
	    || !CreatedProjectile.GetChecked<AGrsBombProjectile>().Launch(*Thrower, HoldTime))
	{
		UPoolManagerSubsystem::Get().ReturnToPool(CreatedProjectile.Handle);
	}

	K2_EndAbility();
}
