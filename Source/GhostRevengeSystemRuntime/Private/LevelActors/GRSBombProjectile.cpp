// Copyright (c) Yevhenii Selivanov


#include "LevelActors/GRSBombProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/GRSDataAsset.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AGRSBombProjectile::AGRSBombProjectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(5.0f);
	RootComponent = CollisionSphere;
	CollisionSphere->OnComponentHit.AddDynamic(this, &AGRSBombProjectile::OnHit);

	// Mesh
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(RootComponent);
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BombMesh->SetStaticMesh(UGRSDataAsset::Get().GetProjectileMesh());

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
}

void AGRSBombProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("GRS Projectile HIT"));
}

// Called every frame
void AGRSBombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
