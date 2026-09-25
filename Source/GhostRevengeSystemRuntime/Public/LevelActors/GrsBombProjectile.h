// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Actor.h"

#include "GrsBombProjectile.generated.h"

struct FGrsThrowTargetData;

/**
 * Replicated data of a single throw.
 * Is replicated once per throw, every machine evaluates the same arc from it, so movement itself is not replicated.
 */
USTRUCT(BlueprintType)
struct GHOSTREVENGESYSTEMRUNTIME_API FGrsBombFlight
{
	GENERATED_BODY()

	/** Ghost that threw the bomb, resolves the bomb mesh and material on every machine */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]")
	TObjectPtr<APawn> Thrower = nullptr;

	/** World location the arc starts from */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]")
	FVector_NetQuantize Start = FVector::ZeroVector;

	/** Velocity the arc was predicted with in the charge preview */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]")
	FVector_NetQuantize10 LaunchVelocity = FVector::ZeroVector;

	/** Gravity the arc was predicted with, is resolved on server by the same rule as UGameplayStatics::PredictProjectilePath */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]")
	float GravityZ = 0.f;

	/** Time the arc takes till its end */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]")
	float FlightTime = 0.f;

	/** Server time the bomb was thrown at, changes every throw, so the rep notify is always called for a reused pooled projectile */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]")
	float LaunchServerTime = 0.f;
};

/**
 * Bomb thrown by a ghost, flies along the same arc the ghost saw in the charge preview.
 * Once it lands, the thrower's client places the real bomb there, the same way regular bombs are placed, since the bomb ability is local predicted.
 * Is pooled: prepared on server once GRS is ready (see UGrsProjectilePoolComponent), taken on each throw (see UGrsThrowBombAbility)
 * and returned back to the pool on landing.
 * Only the throw data is replicated, every machine evaluates the arc by server time, so the flight is the same on all clients.
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

	/** Moves the bomb along the arc, is enabled only while the bomb is flying */
	virtual void Tick(float DeltaTime) override;

	/*********************************************************************************************
	 * Components
	 **********************************************************************************************/
protected:
	/** Root of the projectile, has no collision since the bomb flies over walls to the cell decided on throw */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<class USphereComponent> CollisionSphere = nullptr;

	/** Visual of the bomb, the same mesh and material as the bomb the thrower places */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TObjectPtr<class UStaticMeshComponent> BombMesh = nullptr;

	/*********************************************************************************************
	 * Flight
	 **********************************************************************************************/
public:
	/** Starts a new flight of this projectile, is called on server right after it's taken from the pool.
	 * @param Thrower - Ghost that throws the bomb
	 * @param ThrowData - Launch data of the charge preview the ghost saw */
	void StartFlight(APawn& Thrower, const FGrsThrowTargetData& ThrowData);

protected:
	/** Data of the current throw, is replicated once per throw */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Flight", Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	FGrsBombFlight Flight;

	/** Starts the flight visuals on all machines once throw data is received, is called directly on server */
	UFUNCTION()
	void OnRep_Flight();

	/** Returns true on the machine that controls the thrower's player, where the bomb placement ability is predicted */
	bool IsThrowerLocallyControlled() const;

	/** Returns time passed since the throw by server time, clamped by the flight time */
	float GetFlightElapsedTime() const;

	/** Returns location on the arc at given time since the throw, is the same formula PredictProjectilePath integrates */
	FVector GetFlightLocation(float Time) const;

	/** Applies the same mesh and material as the bomb the thrower places */
	void ApplyBombVisuals();

	/** Hides the projectile once it reached the end of the arc, on the thrower's client places the real bomb, on server returns the projectile to the pool */
	void OnLanded();
};
