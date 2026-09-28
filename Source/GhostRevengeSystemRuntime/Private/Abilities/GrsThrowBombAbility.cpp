// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Abilities/GrsThrowBombAbility.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsBombProjectile.h"
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Structures/BmrCell.h"

// PoolManager
#include "PoolManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsThrowBombAbility)

// Sets default values for this ability
UGrsThrowBombAbility::UGrsThrowBombAbility()
{
	// Local predicted, so the event data sent with the throw reaches the server
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

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

	// --- predict the same arc the ghost saw in the charge preview: charge is sent as magnitude, the start of the arc as context origin
	// Client value is clamped, so a longer charge than allowed can't be sent
	const float HoldTime = FMath::Clamp(TriggerEventData->EventMagnitude, 0.f, UGRSDataAsset::Get().GetMaxChargingTime());
	const FVector StartLocation = GetThrowStartLocation(*TriggerEventData, *Thrower);
	FPredictProjectilePathResult PredictResult;
	if (!Thrower->PredictThrowPath(HoldTime, StartLocation, PredictResult))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(Thrower));

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const TWeakObjectPtr<AGrsPawn> WeakThrower = Thrower;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis, WeakThrower, PredictResult](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsThrowBombAbility* This = WeakThis.Get())
		{
			This->OnTakeProjectileFromPoolCompleted(CreatedObjects, WeakThrower.Get(), PredictResult);
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

// Returns where the thrown arc starts: sent by the client if it's close to the ghost, otherwise the ghost location on server
FVector UGrsThrowBombAbility::GetThrowStartLocation(const FGameplayEventData& EventData, const APawn& Thrower)
{
	const FVector ServerLocation = Thrower.GetActorLocation();
	if (!EventData.ContextHandle.IsValid()
	    || !EventData.ContextHandle.HasOrigin())
	{
		return ServerLocation;
	}

	// Ghost could move a bit on its client till the throw reached the server, so modified client can't throw from anywhere else
	static constexpr float MaxStartLocationError = FBmrCell::CellSize;
	const FVector& ClientLocation = EventData.ContextHandle.GetOrigin();
	return FVector::Dist(ClientLocation, ServerLocation) <= MaxStartLocationError ? ClientLocation : ServerLocation;
}

// Starts the flight of the projectile taken from the pool
void UGrsThrowBombAbility::OnTakeProjectileFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects, APawn* Thrower, const FPredictProjectilePathResult& PredictResult)
{
	if (!ensureMsgf(CreatedObjects.IsValidIndex(0), TEXT("ASSERT: [%i] %hs:\n'CreatedObjects' is empty, projectile is not taken from the pool!"), __LINE__, __FUNCTION__))
	{
		K2_EndAbility();
		return;
	}

	const FPoolObjectData& CreatedProjectile = CreatedObjects[0];
	if (Thrower)
	{
		CreatedProjectile.GetChecked<AGrsBombProjectile>().StartFlight(*Thrower, PredictResult);
	}
	else
	{
		// Ghost is gone while the pool was spawning the projectile, so nothing is thrown and the projectile is released back
		UPoolManagerSubsystem::Get().ReturnToPool(CreatedProjectile.Handle);
	}

	K2_EndAbility();
}
