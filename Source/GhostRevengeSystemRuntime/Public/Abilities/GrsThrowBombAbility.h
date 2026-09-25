// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// Grs
#include "Data/GrsThrowTargetData.h"

// UE
#include "Abilities/GameplayAbility.h"

#include "GrsThrowBombAbility.generated.h"

/**
 * Throws a bomb projectile by a ghost along the same arc the ghost saw in the charge preview.
 * Is granted from code by UGrsPlayerStateComponent while the match is in progress, the granted class is UGRSDataAsset::ThrowBombAbilityClass.
 * Ability is triggered by UGRSDataAsset::ThrowBombTag event sent by the ghost's client, the trigger is set in the blueprint child, where:
 * - Instigator is the ghost pawn;
 * - TargetData contains FGrsThrowTargetData with the launch data of the charge preview.
 * Is local predicted so the event data reaches the server, where the projectile is taken from the pool.
 * The real bomb is placed by the thrower's client once the projectile lands.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsThrowBombAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Sets default values for this ability */
	UGrsThrowBombAbility();

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Actually activate ability, do not call this directly. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/*********************************************************************************************
	 * Throw
	 ********************************************************************************************* */
protected:
	/** Returns throw data sent by the client, or nullptr if the event has no such data */
	static const FGrsThrowTargetData* GetThrowData(const FGameplayEventData& EventData);

	/** Returns true if throw data sent by the client is possible for given ghost, so modified client can't throw a bomb anywhere */
	static bool IsValidThrowData(const FGrsThrowTargetData& ThrowData, const APawn& Thrower);

	/** Starts the flight of the projectile taken from the pool (Object pooling patter).
	 * Is not exposed to blueprints since GAS target data is not a blueprint type.
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 * @param Thrower - Ghost that throws the bomb
	 * @param ThrowData - Launch data of the charge preview */
	virtual void OnTakeProjectileFromPoolCompleted(const TArray<struct FPoolObjectData>& CreatedObjects, APawn* Thrower, const FGrsThrowTargetData& ThrowData);
};
