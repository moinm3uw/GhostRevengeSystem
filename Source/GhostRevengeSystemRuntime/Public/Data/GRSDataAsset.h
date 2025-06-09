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

	/** Returns test string data */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FString GetTestString() const { return TestDataAssetStringInternal; }

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

protected:
	/** Test value for the data asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (BlueprintProtected, DisplayName = "Test stirng"))
	FString TestDataAssetStringInternal;

	/** Spawn location of Ghost Character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (BlueprintProtected, DisplayName = "Spawn location"))
	FVector SpawnLocationInternal;

	/** Collision Asset transform of actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (BlueprintProtected, DisplayName = "Collision Asset Transofrm"))
	FTransform CollisionTransformInternal;

	/** Projectile path params */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (BlueprintProtected, DisplayName = "Predict projectile path params"))
	FPredictProjectilePathParams PredictParamsInternal;

	/** Velocity of the prediction calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (BlueprintProtected, DisplayName = "Velocity in each of the directions"))
	FVector VelocityInternal;

	/** Input context for the GRSPlayerCharacter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (BlueprintProtected, DisplayName = "Input Context", ShowOnlyInnerProperties))
	TObjectPtr<class UMyInputMappingContext> InputContextInternal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++|Bomb")
	TSubclassOf<class AGRSBombProjectile> BombClass;

	/** Aiming area mesh element */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	TObjectPtr<UStaticMesh> AimingAreaStaticMesh;

	/** A visual mesh that represents trajectory of aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	TObjectPtr<UStaticMesh> AimingTrajectoryMeshInternal;

	/** A visual mesh that represents trajectory of aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	TObjectPtr<class UMaterialInterface> TrajectoryMaterialInternal;

	/** A visual mesh that represents aiming area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	TObjectPtr<class UMaterialInterface> AimingMaterialInternal;

	/** Trajectory mesh scale for aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory", meta = (BlueprintProtected, DisplayName = "Aim Trajectory Transofrm"))
	FVector2D TrajectoryMeshScaleInternal;
};
