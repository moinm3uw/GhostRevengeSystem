// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "LevelActors/GrsBombProjectile.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs

// Bmr
#include "DataRegistries/BmrBombRow.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// UE
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsBombProjectile)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this actor's properties
AGrsBombProjectile::AGrsBombProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(true);
	bAlwaysRelevant = true; // ghosts throw from outside of the map, so relevancy should not depend on the distance, is cheap for a few pooled actors
	SetHidden(true);

	// collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(1.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = CollisionSphere;

	// mesh
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(RootComponent);
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Returns properties that are replicated for the lifetime of the actor channel
void AGrsBombProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Thrower, Params);
}

// Server only: moves the bomb between the points of the arc, is enabled only while the bomb is flying
void AGrsBombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority()
	    || FlightPath.PathData.IsEmpty())
	{
		return;
	}

	// Flight progress is mapped by the flight curve to the progress along the arc, so the bomb can fly faster in the beginning and slower in the end
	FlightElapsedTime += DeltaTime;
	const float FlightProgress = FlightDuration > 0.f ? FMath::Clamp(FlightElapsedTime / FlightDuration, 0.f, 1.f) : 1.f;
	const float ArcTime = GetArcProgress(FlightProgress) * FlightPath.PathData.Last().Time;
	SetActorLocation(GetLocationOnPath(ArcTime));

	if (FlightProgress >= 1.f)
	{
		OnLanded();
	}
}

// Returns progress along the arc from 0 to 1 for given flight progress from 0 to 1, is linear if the flight curve is not set
float AGrsBombProjectile::GetArcProgress(float FlightProgress) const
{
	// Bomb always lands at the end of the arc, even if the curve doesn't end with 1
	if (FlightProgress >= 1.f)
	{
		return 1.f;
	}

	float ArcProgress = FlightProgress;
	if (FlightCurve.CurveTable)
	{
		static const FString ContextString = TEXT("ProjectileFlightCurve");
		FlightCurve.Eval(FlightProgress, /*out*/ &ArcProgress, ContextString);
	}

	return FMath::Clamp(ArcProgress, 0.f, 1.f);
}

/*********************************************************************************************
 * Flight
 **********************************************************************************************/

// Launches this projectile, is called on server right after it's taken from the pool
void AGrsBombProjectile::StartFlight(APawn& InThrower, const FPredictProjectilePathResult& PredictResult)
{
	if (!ensureMsgf(HasAuthority(), TEXT("ASSERT: [%i] %hs:\n'StartFlight' has to be called on server only!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(PredictResult.PathData.Num() >= 2, TEXT("ASSERT: [%i] %hs:\n'PredictResult' has no arc to fly along!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(&InThrower));

	SetInstigator(&InThrower);
	Thrower = &InThrower;

	FlightPath = PredictResult;
	FlightElapsedTime = 0.f;

	// Same flight time the thrower's client waits to spawn the bomb, so the bomb appears right when its projectile lands
	const UGRSDataAsset& GrsDataAsset = UGRSDataAsset::Get();
	FlightDuration = GrsDataAsset.GetProjectileFlightTime(PredictResult);
	FlightCurve = GrsDataAsset.GetProjectileFlightCurve();
	SetActorLocation(GetLocationOnPath(FlightElapsedTime));

	// Rep notify is not called on the server, so visuals are applied here directly
	ApplyBombVisuals();

	// Pool shows and ticks only reused projectiles, while newly spawned ones stay hidden without tick, so the flight enables both by itself
	// Hidden state is replicated, so clients see it too
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	ForceNetUpdate();
}

// Applies the bomb visuals of the new thrower on clients
void AGrsBombProjectile::OnRep_Thrower()
{
	if (Thrower)
	{
		ApplyBombVisuals();
	}
}

// Returns location between the two points of the arc the bomb is at, at given time since the throw
FVector AGrsBombProjectile::GetLocationOnPath(float Time) const
{
	const TArray<FPredictProjectilePathPointData>& PathPoints = FlightPath.PathData;
	if (PathPoints.IsEmpty())
	{
		return GetActorLocation();
	}

	for (int32 Index = 1; Index < PathPoints.Num(); ++Index)
	{
		const FPredictProjectilePathPointData& NextPoint = PathPoints[Index];
		if (Time <= NextPoint.Time)
		{
			const FPredictProjectilePathPointData& PreviousPoint = PathPoints[Index - 1];
			const float Alpha = FMath::GetRangePct(PreviousPoint.Time, NextPoint.Time, Time);
			return FMath::Lerp<FVector>(PreviousPoint.Location, NextPoint.Location, FMath::Clamp(Alpha, 0.f, 1.f));
		}
	}

	return PathPoints.Last().Location;
}

// Applies the same mesh and material as the bomb the thrower places
void AGrsBombProjectile::ApplyBombVisuals()
{
	// Same row the bomb resolves in ABmrBombAbilityActor::ApplyMesh() for its instigator
	const FBmrBombRow& BombRow = FBmrBombRow::GetBombRow(Thrower);
	UStaticMesh* NewBombMesh = Cast<UStaticMesh>(BombRow.Mesh.Get());
	if (!ensureMsgf(NewBombMesh, TEXT("ASSERT: [%i] %hs:\n'NewBombMesh' is not valid static mesh!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	BombMesh->SetStaticMesh(NewBombMesh);

	// Projectile is reused by different players, so previous thrower's material is reset first, what also falls back to the mesh default slot
	BombMesh->EmptyOverrideMaterials();

	// Same material the bomb picks in ABmrBombAbilityActor::ApplyMaterial(): cycled by player id, so each player gets own material when sharing the same bomb row
	const int32 BombMaterialsNum = FBmrBombRow::GetBombMaterialsNum();
	const APlayerState* ThrowerPlayerState = Thrower ? Thrower->GetPlayerState() : nullptr;
	if (BombRow.Material.Get()
	    && BombMaterialsNum > 0
	    && ThrowerPlayerState)
	{
		const int32 MaterialIndex = FMath::Abs(ThrowerPlayerState->GetPlayerId()) % BombMaterialsNum;
		if (UMaterialInterface* BombMaterial = FBmrBombRow::GetBombMaterial(MaterialIndex))
		{
			BombMesh->SetMaterial(0, BombMaterial);
		}
	}
}

// Server only: returns the projectile to the pool once it reached the end of the arc, what hides it on all machines
void AGrsBombProjectile::OnLanded()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(Thrower));

	FlightPath = FPredictProjectilePathResult();

	// Projectile is released back instead of being destroyed, so next throw reuses it
	UPoolManagerSubsystem& PoolManager = UPoolManagerSubsystem::Get();
	const FPoolObjectHandle& ProjectileHandle = PoolManager.FindPoolHandleByObject(this);
	if (ensureMsgf(ProjectileHandle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'ProjectileHandle' is not valid, projectile is not in the pool!"), __LINE__, __FUNCTION__))
	{
		PoolManager.ReturnToPool(ProjectileHandle);
	}
}
