// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "Abilities/GameplayAbility.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GrsThrowBombAbility.generated.h"

/**
 * Throws a bomb projectile by a ghost along the same arc the ghost saw in the charge preview.
 * Is granted from code by UGrsPlayerStateComponent while the match is in progress, the granted class is UGRSDataAsset::ThrowBombAbilityClass.
 * Ability is triggered by UGRSDataAsset::ThrowBombTag event sent by the ghost's client, the trigger is set in the blueprint child, where:
 * - Instigator is the ghost pawn;
 * - EventMagnitude is how long the throw was charged;
 * - ContextHandle origin is where the arc starts.
 * Is local predicted so the event data reaches the server, where the same arc is predicted and the projectile is taken from the pool.
 * Projectile is purely visual: the real bomb is spawned by the thrower's UGrsPlayerControllerComponent once the flight time is over.
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
	/** Returns where the thrown arc starts: sent by the client if it's close to the ghost, otherwise the ghost location on server */
	static FVector GetThrowStartLocation(const FGameplayEventData& EventData, const APawn& Thrower);

	/** Starts the flight of the projectile taken from the pool (Object pooling patter).
	 * @param CreatedObjects - Handles of objects from Pool Manager
	 * @param Thrower - Ghost that throws the bomb
	 * @param PredictResult - Arc predicted on server, the same one the ghost saw in the charge preview */
	virtual void OnTakeProjectileFromPoolCompleted(const TArray<struct FPoolObjectData>& CreatedObjects, APawn* Thrower, const FPredictProjectilePathResult& PredictResult);
};
