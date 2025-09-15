// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Data/MyPrimaryDataAsset.h"
#include "DataAssets/MyInputMappingContext.h"
#include "Kismet/GameplayStaticsTypes.h"

#include "GRSDataAsset.generated.h"

/**
 * Contains all ghost revenge assets used in the module
 */
UCLASS(Blueprintable, BlueprintType)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSDataAsset : public UMyPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Returns the progression data asset or crash when can not be obtained. */
	static const UGRSDataAsset& Get();

	/** Returns if the display of trajectory is enabled */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE bool ShouldDisplayTrajectory() const { return bEnableTrajectoryVisualInternal; }

	/** Returns if the display of trajectory is enabled */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE bool ShouldSpawnBombOnMaxChargeTime() const { return bSpawnBombOnMaxChargingTimeInternal; }

	/** Returns spawn location */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FVector GetSpawnLocation() const { return SpawnLocationInternal; }

	/** Returns collision transform */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FTransform GetCollisionTransform() const { return CollisionTransformInternal; }

	/** Returns input context.
	 * @see UGRSDataAsset::InputContextsInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UMyInputMappingContext* GetInputContext() const { return InputContextInternal; }

	/** Returns projectile class
	 * @see UGRSDataAsset::BombClass.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class TSubclassOf<class AGRSBombProjectile> GetProjectileClass() const { return BombClass; }

	/** Returns projectile mesh
	 * @see UGRSDataAsset::StaticMesh.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UStaticMesh* GetProjectileMesh() const { return AimingAreaStaticMesh; }

	/** Returns projectile mesh
	 * @see UGRSDataAsset::ChargeTrajectoryMeshInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UStaticMesh* GetChargeMesh() const { return AimingTrajectoryMeshInternal; }

	/** Returns projectile predict parameters
	 * @see UGRSDataAsset::PredictParams.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FPredictProjectilePathParams GetChargePredictParams() const { return PredictParamsInternal; }

	/** Returns projectile predict velocity
	 * @see UGRSDataAsset::VelocityInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FVector GetVelocityParams() const { return VelocityInternal; }

	/** Returns projectile predict velocity
	 * @see UGRSDataAsset::TrajectoryMaterialInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UMaterialInterface* GetTrajectoryMaterial() const { return TrajectoryMaterialInternal; }

	/** Returns projectile predict velocity
	 * @see UGRSDataAsset::AimingMaterialInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UMaterialInterface* GetAimingMaterial() const { return AimingMaterialInternal; }

	/** Returns Trajectory Scale
	 * @see UGRSDataAsset::TrajectoryMeshScaleInternal */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FVector2D GetTrajectoryMeshScale() const { return TrajectoryMeshScaleInternal; }

	/** Get collision asset class for the sides */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE TSubclassOf<class AActor> GetCollisionsAssetClass() const { return CollisionsAssetInternal; }

protected:
	/** Input mapping context for the GRSPlayerCharacter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Input Mapping Context", ShowOnlyInnerProperties))
	TObjectPtr<class UMyInputMappingContext> InputContextInternal;

	/** A collision used to define the area where ghost character can move around. Placed on the sides of the map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "A box collision asset transofrm spawned on sides of map"))
	FTransform CollisionTransformInternal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TSubclassOf<class AGRSBombProjectile> BombClass;

	/** Parameter to control trajectory visual display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Display trajectory"))
	bool bEnableTrajectoryVisualInternal;

	/** Parameter to control if bomb should be spawned once reached a max charging time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Spawn Bomb once Maximum Charge Time Reached"))
	bool bSpawnBombOnMaxChargingTimeInternal;

	/** Projectile path params */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Predict projectile path params"))
	FPredictProjectilePathParams PredictParamsInternal;

	/** Velocity of the prediction calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Trajectory Velocity multiplier in each of the directions"))
	FVector VelocityInternal;

	/** A visual mesh that represents trajectory of aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual")
	TObjectPtr<UStaticMesh> AimingTrajectoryMeshInternal;

	/** A visual mesh that represents trajectory of aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual")
	TObjectPtr<class UMaterialInterface> TrajectoryMaterialInternal;

	/** A visual mesh that represents aiming area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual")
	TObjectPtr<class UMaterialInterface> AimingMaterialInternal;

	/** Trajectory mesh scale for aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Aim Trajectory Transofrm"))
	FVector2D TrajectoryMeshScaleInternal;

	/** Spawn location of Ghost Character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporarry", meta = (BlueprintProtected, DisplayName = "Ghost Character Spawn Location"))
	FVector SpawnLocationInternal;

	/** Aiming area mesh element */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporarry")
	TObjectPtr<UStaticMesh> AimingAreaStaticMesh;

	/** Asset that contains scalable collision. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Collisions Asset", ShowOnlyInnerProperties))
	TSubclassOf<class AActor> CollisionsAssetInternal = nullptr;
};
