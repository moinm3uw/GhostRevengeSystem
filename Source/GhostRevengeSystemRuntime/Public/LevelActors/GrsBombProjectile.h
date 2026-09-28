// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
#include "Engine/CurveTable.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GrsBombProjectile.generated.h"

/**
 * Bomb thrown by a ghost, moves between the points of the arc the ghost saw in the charge preview.
 * Only the server moves the projectile, clients receive its location by replicated movement, as it's done for pawns.
 * Is purely visual: the real bomb is spawned by the thrower's UGrsPlayerControllerComponent once the same flight time is over on its client.
 * Is pooled: prepared on server once GRS is ready (see UGrsProjectilePoolComponent), taken on each throw (see UGrsThrowBombAbility)
 * and returned back to the pool on landing.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGrsBombProjectile : public AActor
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Lifecycle
	 **********************************************************************************************/
public:
	/** Sets default values for this actor's properties */
	AGrsBombProjectile();

protected:
	/** Returns properties that are replicated for the lifetime of the actor channel */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server only: moves the bomb between the points of the arc, is enabled only while the bomb is flying */
	virtual void Tick(float DeltaTime) override;

	/*********************************************************************************************
	 * Components
	 **********************************************************************************************/
protected:
	/** Root of the projectile, has no collision since the bomb flies over walls to the end of the arc */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<class USphereComponent> CollisionSphere = nullptr;

	/** Visual of the bomb, the same mesh and material as the bomb the thrower places */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<class UStaticMeshComponent> BombMesh = nullptr;

	/*********************************************************************************************
	 * Flight
	 **********************************************************************************************/
public:
	/** Launches this projectile, is called on server right after it's taken from the pool.
	 * @param InThrower - Ghost that throws the bomb
	 * @param PredictResult - Arc predicted on server, the same one the ghost saw in the charge preview */
	void StartFlight(APawn& InThrower, const FPredictProjectilePathResult& PredictResult);

protected:
	/** Ghost that threw the bomb, resolves the bomb mesh and material on every machine */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Thrower", Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<APawn> Thrower = nullptr;

	/** Server only: points of the arc the projectile moves between */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	FPredictProjectilePathResult FlightPath;

	/** Server only: time passed since the throw */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	float FlightElapsedTime = 0.f;

	/** Server only: how long the flight takes with the projectile flight speed, is taken from the data asset once per throw */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	float FlightDuration = 0.f;

	/** Server only: how the bomb goes along the arc during its flight, is taken from the data asset once per throw */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	FCurveTableRowHandle FlightCurve;

	/** Returns progress along the arc from 0 to 1 for given flight progress from 0 to 1, is linear if the flight curve is not set */
	float GetArcProgress(float FlightProgress) const;

	/** Applies the bomb visuals of the new thrower on clients */
	UFUNCTION()
	void OnRep_Thrower();

	/** Returns location between the two points of the arc the bomb is at, at given time since the throw */
	FVector GetLocationOnPath(float Time) const;

	/** Applies the same mesh and material as the bomb the thrower places */
	void ApplyBombVisuals();

	/** Server only: returns the projectile to the pool once it reached the end of the arc, what hides it on all machines */
	void OnLanded();
};
