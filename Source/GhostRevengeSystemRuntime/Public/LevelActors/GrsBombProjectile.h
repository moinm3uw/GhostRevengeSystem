// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"

#include "GrsBombProjectile.generated.h"

/**
 * Bomb thrown by a ghost, flies by its ProjectileMovementComponent along the same arc the ghost saw in the charge preview:
 * start location, launch velocity and gravity are calculated from the ghost and its charge the same way as AGrsPawn::PredictThrowPath does.
 * Only the server moves the projectile, clients receive its location by replicated movement.
 * Looks like the bomb the thrower places: mesh and material are applied from the replicated instigator on every machine.
 * Once the flight time is over, the server spawns the real bomb at the nearest free cell under the projectile and hides the projectile in the same frame.
 * Is pooled: prepared on server once GRS is ready (see UGrsProjectilePoolComponent), taken on each throw (see UGrsThrowBombAbility)
 * and returned back to the pool once the flight time is over.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGrsBombProjectile : public AActor
{
	GENERATED_BODY()

public:
	/** Sets default values for this actor's properties */
	AGrsBombProjectile();

	/** Server only: launches this projectile from the thrower along the same arc the ghost saw in the charge preview.
	 * @param Thrower - Ghost that throws the bomb, the arc starts at its location
	 * @param HoldTime - How long the throw was charged, the longer it's charged the further the bomb is thrown
	 * @return false if the arc can't be calculated, e.g. the ghost side is not known yet */
	bool Launch(class AGrsPawn& Thrower, float HoldTime);

protected:
	/** Visual of the bomb, is the root the projectile movement moves, has the same mesh and material as the bomb the thrower places */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<class UStaticMeshComponent> BombMesh = nullptr;

	/** Moves the bomb, its velocity and gravity are set on launch to follow the predicted arc */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement = nullptr;

	/** Server only: spawns the bomb and hides the projectile once the flight time is over */
	FTimerHandle FlightTimerHandle;

	/** Applies the bomb visuals of the new thrower on clients, the thrower is the replicated instigator */
	virtual void OnRep_Instigator() override;

	/** Applies the same mesh and material as the bomb the thrower places */
	void ApplyBombVisuals();

	/** Server only: spawns the bomb at the nearest free cell under the projectile and returns the projectile to the pool, what hides it on all machines */
	void OnFlightTimeOver();

	/** Server only: activates the bomb ability of the thrower at the nearest free cell under the projectile */
	void SpawnBomb();
};
