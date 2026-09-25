// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Abilities/GrsThrowBombAbility.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsBombProjectile.h"

// Bmr
#include "Structures/BmrCell.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// UE
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsThrowBombAbility)

// Sets default values for this ability
UGrsThrowBombAbility::UGrsThrowBombAbility()
{
	// Local predicted, so the throw data sent with the event reaches the server
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Instance is kept alive to receive the pool callback
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
	APawn* Thrower = const_cast<APawn*>(Cast<APawn>(TriggerEventData->Instigator.Get()));
	const FGrsThrowTargetData* ThrowData = GetThrowData(*TriggerEventData);
	if (!ensureMsgf(Thrower && ThrowData, TEXT("ASSERT: [%i] %hs:\n'Thrower' or 'ThrowData' is not valid in the throw event!"), __LINE__, __FUNCTION__)
	    || !IsValidThrowData(*ThrowData, *Thrower))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(Thrower));

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const TWeakObjectPtr<APawn> WeakThrower = Thrower;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis, WeakThrower, ThrowDataCopy = *ThrowData](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGrsThrowBombAbility* This = WeakThis.Get())
		{
			This->OnTakeProjectileFromPoolCompleted(CreatedObjects, WeakThrower.Get(), ThrowDataCopy);
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

// Returns throw data sent by the client, or nullptr if the event has no such data
const FGrsThrowTargetData* UGrsThrowBombAbility::GetThrowData(const FGameplayEventData& EventData)
{
	const FGameplayAbilityTargetData* TargetData = EventData.TargetData.IsValid(0) ? EventData.TargetData.Get(0) : nullptr;
	const bool bIsThrowData = TargetData && TargetData->GetScriptStruct() == FGrsThrowTargetData::StaticStruct();
	return bIsThrowData ? static_cast<const FGrsThrowTargetData*>(TargetData) : nullptr;
}

// Returns true if throw data sent by the client is possible for given ghost, so modified client can't throw a bomb anywhere
bool UGrsThrowBombAbility::IsValidThrowData(const FGrsThrowTargetData& ThrowData, const APawn& Thrower)
{
	const UGRSDataAsset& GrsDataAsset = UGRSDataAsset::Get();

	// Ghost could move a bit on its client till the throw reached the server
	static constexpr float MaxStartLocationError = FBmrCell::CellSize;
	const bool bIsValidStart = FVector::Dist(ThrowData.Start, Thrower.GetActorLocation()) <= MaxStartLocationError;

	const bool bIsValidFlightTime = ThrowData.FlightTime > 0.f
	                                && ThrowData.FlightTime <= GrsDataAsset.GetChargePredictParams().MaxSimTime;

	// Launch velocity is built in UGrsPlayerControllerComponent::PredictProjectilePath: unit direction plus velocity params scaled by the charge on X
	// Charge can exceed its max by one frame, since it's accumulated by frame delta before checked
	static constexpr float ChargeTimeError = 0.1f;
	static constexpr float QuantizationError = 1.f;
	const FVector VelocityParams = GrsDataAsset.GetVelocityParams().GetAbs();
	const float MaxChargeTime = GrsDataAsset.GetMaxChargingTime() + ChargeTimeError;
	const FVector MaxLaunchVelocity(1.f + VelocityParams.X * MaxChargeTime, VelocityParams.Y, 1.f + VelocityParams.Z);
	const bool bIsValidVelocity = ThrowData.LaunchVelocity.Size() <= MaxLaunchVelocity.Size() + QuantizationError;

	const bool bIsValid = bIsValidStart && bIsValidFlightTime && bIsValidVelocity;
	UE_CLOG(!bIsValid, LogGrs, Warning, TEXT("[%i] %hs: Rejected throw of %s: start %s, flight time %s, velocity %s"), __LINE__, __FUNCTION__, *Thrower.GetName(),
	        bIsValidStart ? TEXT("OK") : TEXT("INVALID"), bIsValidFlightTime ? TEXT("OK") : TEXT("INVALID"), bIsValidVelocity ? TEXT("OK") : TEXT("INVALID"));
	return bIsValid;
}

// Starts the flight of the projectile taken from the pool
void UGrsThrowBombAbility::OnTakeProjectileFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects, APawn* Thrower, const FGrsThrowTargetData& ThrowData)
{
	if (!ensureMsgf(CreatedObjects.IsValidIndex(0), TEXT("ASSERT: [%i] %hs:\n'CreatedObjects' is empty, projectile is not taken from the pool!"), __LINE__, __FUNCTION__))
	{
		K2_EndAbility();
		return;
	}

	const FPoolObjectData& CreatedProjectile = CreatedObjects[0];
	if (Thrower)
	{
		CreatedProjectile.GetChecked<AGrsBombProjectile>().StartFlight(*Thrower, ThrowData);
	}
	else
	{
		// Ghost is gone while the pool was spawning the projectile, so nothing is thrown and the projectile is released back
		UPoolManagerSubsystem::Get().ReturnToPool(CreatedProjectile.Handle);
	}

	K2_EndAbility();
}
