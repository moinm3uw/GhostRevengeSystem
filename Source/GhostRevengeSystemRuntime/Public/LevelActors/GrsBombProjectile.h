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

	// @PR JanSeliv [Coding Standards] - missing module category, use Category = "[GhostRevengeSystem]" like neighbor UFUNCTIONs
	UFUNCTION(BlueprintCallable)
	void Launch(const FVector& LaunchVelocity);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// @PR JanSeliv [Coding Standards] - protected BP-exposed UFUNCTION needs meta = (BlueprintProtected), mirror C++ access like neighbor OnGameStateChanged
	/** Called when the GRS data asset is loaded and available */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[GhostRevengeSystem]")
	void OnDataAssetLoaded(const class UGRSDataAsset* DataAsset);

	// @PR JanSeliv [Coding Standards] - CreateDefaultSubobject component uses VisibleDefaultsOnly across module, not VisibleAnywhere, match neighbor GrsPawn components, applies across file (BombMesh, ProjectileMovement)
	// @PR JanSeliv [Coding Standards] - wrap UObject member in TObjectPtr and init nullptr, raw pointer no init, applies across file (BombMesh, ProjectileMovement)
	// @PR JanSeliv [Coding Standards] - BP-expose UPROPERTY with BlueprintReadOnly + meta=(BlueprintProtected) like neighbor GrsPawn components, applies across file
	UPROPERTY(VisibleAnywhere, Category = "[GhostRevengeSystem]")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "[GhostRevengeSystem]")
	class UStaticMeshComponent* BombMesh;

	UPROPERTY(VisibleAnywhere, Category = "[GhostRevengeSystem]")
	class UProjectileMovementComponent* ProjectileMovement;

	// @PR JanSeliv [Coding Standards] - On-callback must be BlueprintNativeEvent like OnDataAssetLoaded, add module Category = "[GhostRevengeSystem]"
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	    FVector NormalImpulse, const FHitResult& Hit);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
