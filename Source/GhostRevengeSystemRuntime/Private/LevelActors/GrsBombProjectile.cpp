// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "LevelActors/GrsBombProjectile.h"

#include "Data/GRSDataAsset.h"
#include "Data/GrsThrowTargetData.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs

// Bmr
#include "DataRegistries/BmrBombRow.h"
#include "Structures/BmrCell.h"
#include "GameFramework/BmrGameState.h"
#include "Structures/BmrGameStateTag.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsBombProjectile)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values
AGrsBombProjectile::AGrsBombProjectile()
{
	// Tick moves the bomb only while it flies
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Only throw data is replicated, the arc is evaluated on each machine
	bReplicates = true;
	SetReplicatingMovement(false);

	// Ghosts throw from outside of the map, so relevancy should not depend on the distance, is cheap for a few pooled actors
	bAlwaysRelevant = true;

	// Pool prepares objects without notifying them about inactive state, so start hidden
	SetHidden(true);

	// Collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(1.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = CollisionSphere;

	// Mesh
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(RootComponent);
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Returns properties that are replicated for the lifetime of the actor channel
void AGrsBombProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Flight, Params);
}

// Moves the bomb along the arc, is enabled only while the bomb is flying
void AGrsBombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float FlightElapsedTime = GetFlightElapsedTime();
	SetActorLocation(GetFlightLocation(FlightElapsedTime));

	if (FlightElapsedTime >= Flight.FlightTime)
	{
		OnLanded();
	}
}

/*********************************************************************************************
 * Flight
 **********************************************************************************************/

// Starts a new flight of this projectile, is called on server right after it's taken from the pool
void AGrsBombProjectile::StartFlight(APawn& Thrower, const FGrsThrowTargetData& ThrowData)
{
	if (!ensureMsgf(HasAuthority(), TEXT("ASSERT: [%i] %hs:\n'StartFlight' has to be called on server only!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(&Thrower));

	SetInstigator(&Thrower);

	// Same gravity rule as UGameplayStatics::PredictProjectilePath, so the arc matches the charge preview
	const float OverrideGravityZ = UGRSDataAsset::Get().GetChargePredictParams().OverrideGravityZ;

	Flight.Thrower = &Thrower;
	Flight.Start = ThrowData.Start;
	Flight.LaunchVelocity = ThrowData.LaunchVelocity;
	Flight.GravityZ = FMath::IsNearlyZero(OverrideGravityZ) ? GetWorld()->GetGravityZ() : OverrideGravityZ;
	Flight.FlightTime = ThrowData.FlightTime;
	Flight.LaunchServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	ForceNetUpdate();

	// Rep notify is not called on the server, so visuals are applied here directly
	OnRep_Flight();
}

// Starts the flight visuals on all machines once throw data is received, is called directly on server
void AGrsBombProjectile::OnRep_Flight()
{
	if (!Flight.Thrower)
	{
		return;
	}

	const float FlightElapsedTime = GetFlightElapsedTime();
	if (FlightElapsedTime >= Flight.FlightTime)
	{
		// Throw arrived after it should have already landed (flight is shorter than ping), so the thrower still has to place its bomb
		// Other clients have nothing to show
		if (IsThrowerLocallyControlled())
		{
			OnLanded();
		}
		return;
	}

	ApplyBombVisuals();

	SetActorLocation(GetFlightLocation(FlightElapsedTime));

	// Pool does not replicate tick state, so clients enable it by themselves
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
}

// Returns true on the machine that controls the thrower's player, where the bomb placement ability is predicted
bool AGrsBombProjectile::IsThrowerLocallyControlled() const
{
	// Is checked by the player's ASC rather than the ghost pawn, so the bomb is still placed if the ghost was unpossessed during the flight
	const UAbilitySystemComponent* ThrowerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Flight.Thrower);
	return ThrowerASC
	       && ThrowerASC->AbilityActorInfo.IsValid()
	       && ThrowerASC->AbilityActorInfo->IsLocallyControlled();
}

// Returns time passed since the throw by server time, clamped by the flight time
float AGrsBombProjectile::GetFlightElapsedTime() const
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState)
	{
		return Flight.FlightTime;
	}

	const float ServerTime = GameState->GetServerWorldTimeSeconds();
	return FMath::Clamp(ServerTime - Flight.LaunchServerTime, 0.f, Flight.FlightTime);
}

// Returns location on the arc at given time since the throw, is the same formula PredictProjectilePath integrates
FVector AGrsBombProjectile::GetFlightLocation(float Time) const
{
	const FVector GravityOffset(0.f, 0.f, 0.5f * Flight.GravityZ * FMath::Square(Time));
	return Flight.Start + Flight.LaunchVelocity * Time + GravityOffset;
}

// Applies the same mesh and material as the bomb the thrower places
void AGrsBombProjectile::ApplyBombVisuals()
{
	// Same row the bomb resolves in ABmrBombAbilityActor::ApplyMesh() for its instigator
	const FBmrBombRow& BombRow = FBmrBombRow::GetBombRow(Flight.Thrower);
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
	const APlayerState* ThrowerPlayerState = Flight.Thrower ? Flight.Thrower->GetPlayerState() : nullptr;
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

// Hides the projectile once it reached the end of the arc, on the thrower's client places the real bomb, on server returns the projectile to the pool
void AGrsBombProjectile::OnLanded()
{
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true); // clients hide it locally without waiting for the server

	// Bomb ability is local predicted, so like regular bombs it's triggered by the thrower's client
	// A throw that was still flying when the match ended must not place a bomb
	if (IsThrowerLocallyControlled()
	    && ABmrGameState::Get().HasMatchingGameplayTag(FBmrGameStateTag::InGame))
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: Thrower: %s"), __LINE__, __FUNCTION__, *GetNameSafe(Flight.Thrower));

		// Cell could be occupied during the flight, so the nearest free one to the end of the arc is resolved on landing
		FBmrCell LandingCell;
		LandingCell.Location = GetFlightLocation(Flight.FlightTime);
		const FBmrCell SpawnBombCell = UBmrCellUtilsLibrary::GetNearestFreeCell(LandingCell);

		// Activate bomb ability
		FGameplayEventData EventData;
		EventData.EventTag = UGRSDataAsset::Get().GetTriggerBombTag();
		EventData.Instigator = Flight.Thrower;
		EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(SpawnBombCell);
		UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
	}

	if (!HasAuthority())
	{
		return;
	}

	// Projectile is released back instead of being destroyed, so next throw reuses it
	UPoolManagerSubsystem& PoolManager = UPoolManagerSubsystem::Get();
	const FPoolObjectHandle& ProjectileHandle = PoolManager.FindPoolHandleByObject(this);
	if (ensureMsgf(ProjectileHandle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'ProjectileHandle' is not valid, projectile is not in the pool!"), __LINE__, __FUNCTION__))
	{
		PoolManager.ReturnToPool(ProjectileHandle);
	}
}
