// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GrsBombProjectile.h"

// Grs
#include "Data/GRSDataAsset.h"

// Bmr
#include "Bomber.h"

// DataAssetLoader
#include "DalSubsystem.h"

// UE
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/ProjectileMovementComponent.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsBombProjectile)

// Sets default values
AGrsBombProjectile::AGrsBombProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(1.0f);
	RootComponent = CollisionSphere;

	// Setup collision to allow overlap players with each other, but block all other actors
	CollisionSphere->CanCharacterStepUpOn = ECB_Yes;
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Player0, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Player1, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Player2, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Player3, ECR_Overlap);

	CollisionSphere->OnComponentHit.AddDynamic(this, &AGrsBombProjectile::OnHit);

	// Mesh
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(RootComponent);
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Projectile movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void AGrsBombProjectile::Launch(const FVector& LaunchVelocity)
{
	ProjectileMovement->Velocity = LaunchVelocity;
}

// Called when the game starts or when spawned
void AGrsBombProjectile::BeginPlay()
{
	Super::BeginPlay();

	UDalSubsystem::Get().ListenForDataAsset<UGRSDataAsset>(this, &ThisClass::OnDataAssetLoaded);
}

// Called when the GRS data asset is loaded and available
void AGrsBombProjectile::OnDataAssetLoaded_Implementation(const UGRSDataAsset* DataAsset)
{
	BombMesh->SetStaticMesh(DataAsset->GetProjectileMesh());
}

void AGrsBombProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Log, TEXT("GRS Projectile HIT"));
}

// Called every frame
void AGrsBombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
