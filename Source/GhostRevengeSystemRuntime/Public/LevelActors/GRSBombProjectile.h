// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GRSBombProjectile.generated.h"

UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGRSBombProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGRSBombProjectile();

	UFUNCTION(BlueprintCallable)
	void Launch(const FVector& LaunchVelocity);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "[GhostRevengeSystem]")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "[GhostRevengeSystem]")
	class UStaticMeshComponent* BombMesh;

	UPROPERTY(VisibleAnywhere, Category = "[GhostRevengeSystem]")
	class UProjectileMovementComponent* ProjectileMovement;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	    FVector NormalImpulse, const FHitResult& Hit);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
