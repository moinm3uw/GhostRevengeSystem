// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "Abilities/GameplayAbility.h"

#include "GrsThrowBombAbility.generated.h"

/**
 * Throws a bomb projectile by a ghost, the projectile flies by its own ProjectileMovementComponent.
 * Is granted from code by UGrsPlayerStateComponent while the match is in progress, the granted class is UGRSDataAsset::ThrowBombAbilityClass.
 * Ability is triggered by UGRSDataAsset::ThrowBombTag event sent by the ghost's client, the trigger is set in the blueprint child, where:
 * - Instigator is the ghost pawn;
 * - EventMagnitude is how long the throw was charged.
 * Projectile calculates the same arc as the charge preview from the ghost and the charge (see AGrsBombProjectile::Launch).
 * Is local predicted so the throw reaches the server, where the projectile is taken from the pool and launched from the ghost.
 * Projectile spawns the real bomb on server once the flight time is over, so the bomb ability triggered by UGRSDataAsset::TriggerBombTag has to be server activated.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsThrowBombAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Sets default values for this ability */
	UGrsThrowBombAbility();

	/** Returns true if this ability is triggered by given gameplay event, the trigger is set in the blueprint child */
	bool IsTriggeredByEvent(const FGameplayTag& EventTag) const;

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
	/** Starts the flight of the projectile taken from the pool (Object pooling patter).
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 * @param Thrower - Ghost that throws the bomb
	 * @param HoldTime - How long the throw was charged */
	virtual void OnTakeProjectileFromPoolCompleted(const TArray<struct FPoolObjectData>& CreatedObjects, class AGrsPawn* Thrower, float HoldTime);
};
