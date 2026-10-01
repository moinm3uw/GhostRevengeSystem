// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "LevelActors/GrsBombProjectile.h"

#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsPawn.h"

// Bmr
#include "DataRegistries/BmrBombRow.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrCell.h"
#include "Structures/BmrGameStateTag.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstance.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsBombProjectile)

// Sets default values for this actor's properties
AGrsBombProjectile::AGrsBombProjectile()
{
	bReplicates = true;
	SetReplicatingMovement(true);
	bAlwaysRelevant = true; // ghosts throw from outside of the map, so relevancy should not depend on the distance, is cheap for a few pooled actors
	SetHidden(true);

	// mesh, is purely visual: has no collision, so it flies along the predicted arc without being stopped by anything
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = BombMesh;

	// movement, is not auto activated since the pool spawns the projectile far away, it's activated on launch instead
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = BombMesh;
	ProjectileMovement->bAutoActivate = false;
}

// Server only: launches this projectile from the thrower along the same arc the ghost saw in the charge preview
bool AGrsBombProjectile::Launch(AGrsPawn& Thrower, float HoldTime)
{
	if (!ensureMsgf(HasAuthority(), TEXT("ASSERT: [%i] %hs:\n'Launch' has to be called on server only!"), __LINE__, __FUNCTION__))
	{
		return false;
	}

	// --- pick a direction based on the side of the map (left or right) the server allocated for this ghost
	const UGrsPlayerStateComponent* GrsPlayerStateComponent = Thrower.GetGrsPlayerStateComponent();
	const EGRSCharacterSide GhostSide = GrsPlayerStateComponent ? GrsPlayerStateComponent->GetGhostSide() : EGRSCharacterSide::None;
	if (GhostSide == EGRSCharacterSide::None)
	{
		return false;
	}

	const float SideSign = GhostSide == EGRSCharacterSide::Left ? 1.0f : -1.0f;

	// 45-degree vector between up and right
	const FVector UpRight45 = (Thrower.GetActorForwardVector() + Thrower.GetActorUpVector()).GetSafeNormal();

	// Set launch velocity (forward direction with some upward angle), the longer the charge the further the throw
	const FVector VelocityParams = UGRSDataAsset::Get().GetVelocityParams();

	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = Thrower.GetActorLocation();
	Params.LaunchVelocity = FVector(UpRight45.X + SideSign * (VelocityParams.X * HoldTime), VelocityParams.Y, UpRight45.Z + VelocityParams.Z);
	Params.ActorsToIgnore.Add(&Thrower);

	// Flight speed plays the same arc faster or slower: velocity is scaled by it and gravity by its square,
	// so the curvature stays the same while the predicted time of each point is divided by the speed.
	// Simulation time and frequency are scaled as well, so the arc ends at the same point and is traced with the same amount of steps
	// Gravity is the world one if not overridden, it's taken from the movement, so the projectile falls with exactly the predicted gravity
	const float WorldGravityZ = ProjectileMovement->UMovementComponent::GetGravityZ();
	const float FlightSpeed = UGRSDataAsset::Get().GetProjectileFlightSpeed();
	const float GravityZ = FMath::IsNearlyZero(Params.OverrideGravityZ) ? WorldGravityZ : Params.OverrideGravityZ;
	Params.LaunchVelocity *= FlightSpeed;
	Params.OverrideGravityZ = GravityZ * FMath::Square(FlightSpeed);
	Params.MaxSimTime /= FlightSpeed;
	Params.SimFrequency *= FlightSpeed;

	FPredictProjectilePathResult PredictResult;
	UGameplayStatics::PredictProjectilePath(this, Params, PredictResult);
	if (PredictResult.PathData.Num() < 2)
	{
		return false;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s, HoldTime: %f"), __LINE__, __FUNCTION__, *Thrower.GetName(), HoldTime);

	// Instigator is replicated, so clients apply the same visuals in OnRep_Instigator, while rep notify is not called on the server
	SetInstigator(&Thrower);
	ApplyBombVisuals();
	SetActorLocation(Params.StartLocation);

	// Same gravity as the arc is predicted with
	ProjectileMovement->ProjectileGravityScale = !FMath::IsNearlyZero(WorldGravityZ) ? Params.OverrideGravityZ / WorldGravityZ : 1.f;

	ProjectileMovement->Velocity = Params.LaunchVelocity;
	ProjectileMovement->Activate(/*bReset*/ true);

	SetActorHiddenInGame(false);

	// Projectile has no collision, so it reaches the end of the predicted arc right when its flight time is over
	const float FlightTime = PredictResult.PathData.Last().Time;
	GetWorldTimerManager().SetTimer(FlightTimerHandle, this, &ThisClass::OnFlightTimeOver, FlightTime, /*bLoop*/ false);

	ForceNetUpdate();
	return true;
}

// Applies the bomb visuals of the new thrower on clients, the thrower is the replicated instigator
void AGrsBombProjectile::OnRep_Instigator()
{
	Super::OnRep_Instigator();

	if (GetInstigator())
	{
		ApplyBombVisuals();
	}
}

// Applies the same mesh and material as the bomb the thrower places
void AGrsBombProjectile::ApplyBombVisuals()
{
	// Same row the bomb resolves in ABmrBombAbilityActor::ApplyMesh() for its instigator
	const APawn* Thrower = GetInstigator();
	const FBmrBombRow& BombRow = FBmrBombRow::GetBombRow(Thrower);
	UStaticMesh* NewBombMesh = Cast<UStaticMesh>(BombRow.Mesh.Get());
	if (!ensureMsgf(NewBombMesh, TEXT("ASSERT: [%i] %hs: 'NewBombMesh' is not valid static mesh!"), __LINE__, __FUNCTION__))
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

// Server only: spawns the bomb at the nearest free cell under the projectile and returns the projectile to the pool, what hides it on all machines
void AGrsBombProjectile::OnFlightTimeOver()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(GetInstigator()));

	// Bomb is spawned and the projectile is hidden in the same frame, so both are replicated to clients together
	SpawnBomb();

	ProjectileMovement->Deactivate();

	// Projectile is released back instead of being destroyed, so next throw reuses it
	UPoolManagerSubsystem& PoolManager = UPoolManagerSubsystem::Get();
	const FPoolObjectHandle& ProjectileHandle = PoolManager.FindPoolHandleByObject(this);
	if (ensureMsgf(ProjectileHandle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'ProjectileHandle' is not valid, projectile is not in the pool!"), __LINE__, __FUNCTION__))
	{
		PoolManager.ReturnToPool(ProjectileHandle);
	}
}

// Server only: activates the bomb ability of the thrower at the nearest free cell under the projectile
void AGrsBombProjectile::SpawnBomb()
{
	// Ghost could be no longer in control, e.g. it's revived or the match is over while the bomb was flying
	APawn* Thrower = GetInstigator();
	if (!IsValid(Thrower)
	    || !Thrower->GetController()
	    || !ABmrGameState::Get().HasMatchingGameplayTag(FBmrGameStateTag::InGame))
	{
		return;
	}

	const FBmrCell SpawnBombCell = UBmrCellUtilsLibrary::GetNearestFreeCell(FBmrCell(GetActorLocation()));

	// Activate bomb ability of the thrower, the event is sent on server, so the bomb ability has to be server activated
	FGameplayEventData EventData;
	EventData.EventTag = UGRSDataAsset::Get().GetTriggerBombTag();
	EventData.Instigator = Thrower;
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(SpawnBombCell);
	UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
}
