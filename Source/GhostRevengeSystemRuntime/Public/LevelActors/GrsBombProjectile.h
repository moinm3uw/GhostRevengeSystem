// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GrsBombProjectile.generated.h"

UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGrsBombProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGrsBombProjectile();

	UFUNCTION(BlueprintCallable)
	void Launch(const FVector& LaunchVelocity);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Called when the GRS data asset is loaded and available */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]")
	void OnDataAssetLoaded(const class UGRSDataAsset* DataAsset);

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
