// Copyright (c) Yevhenii Selivanov

#include "LevelActors/GRSBombProjectile.h"

#include "Bomber.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/GRSDataAsset.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AGRSBombProjectile::AGRSBombProjectile()
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

	CollisionSphere->OnComponentHit.AddDynamic(this, &AGRSBombProjectile::OnHit);

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

void AGRSBombProjectile::Launch(const FVector& LaunchVelocity)
{
	ProjectileMovement->Velocity = LaunchVelocity;
}

// Called when the game starts or when spawned
void AGRSBombProjectile::BeginPlay()
{
	Super::BeginPlay();
	BombMesh->SetStaticMesh(UGRSDataAsset::Get().GetProjectileMesh());
}

void AGRSBombProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Log, TEXT("GRS Projectile HIT"));
}

// Called every frame
void AGRSBombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
