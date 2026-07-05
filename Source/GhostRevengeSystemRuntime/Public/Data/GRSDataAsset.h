// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "DalPrimaryDataAsset.h"

// UE
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStaticsTypes.h" // FPredictProjectilePathParams

#include "GRSDataAsset.generated.h"

class UMaterialInterface;
class UGameplayEffect;

/**
 * Contains all ghost revenge assets used in the module
 */
UCLASS(Blueprintable, BlueprintType)
class GHOSTREVENGESYSTEMRUNTIME_API UGRSDataAsset : public UDalPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Returns the progression data asset or crash when can not be obtained. */
	static const UGRSDataAsset& Get();

	/** Returns the Grs player character class */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = "[GhostRevengeSystem]")
	TSubclassOf<class AGrsPawn> GetGrsActorClass() const { return GrsActorClass; }

	/** Returns if the display of trajectory is enabled */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE bool ShouldDisplayTrajectory() const { return bEnableTrajectoryVisual; }

	/** Returns if the display of trajectory is enabled */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE bool ShouldSpawnBombOnMaxChargeTime() const { return bSpawnBombOnMaxChargingTime; }

	/** Returns spawn location */
	FORCEINLINE const FVector& GetSpawnLocation() const { return SpawnLocation; }

	/** Returns collision transform */
	FORCEINLINE const FTransform& GetCollisionTransform() const { return CollisionTransform; }

	/** Returns input context.
	 * @see UGRSDataAsset::InputContextsInternal.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE class UBmrInputMappingContext* GetInputContext() const { return InputContext; }

	/** Returns projectile class
	 * @see UGRSDataAsset::BombClass.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE TSubclassOf<class AGrsBombProjectile> GetProjectileClass() const { return BombClass; }

	/** Returns projectile mesh
	 * @see UGRSDataAsset::StaticMesh.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE class UStaticMesh* GetProjectileMesh() const { return AimingAreaStaticMesh; }

	/** Returns projectile mesh
	 * @see UGRSDataAsset::ChargeTrajectoryMeshInternal.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE class UStaticMesh* GetChargeMesh() const { return AimingTrajectoryMesh; }

	/** Returns projectile predict parameters
	 * @see UGRSDataAsset::PredictParams.*/
	FORCEINLINE const FPredictProjectilePathParams& GetChargePredictParams() const { return PredictParams; }

	/** Returns projectile predict velocity
	 * @see UGRSDataAsset::VelocityInternal.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE FVector GetVelocityParams() const { return Velocity; }

	/** Returns projectile predict velocity
	 * @see UGRSDataAsset::TrajectoryMaterialInternal.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UMaterialInterface* GetTrajectoryMaterial() const { return TrajectoryMaterial; }

	/** Returns projectile predict velocity
	 * @see UGRSDataAsset::AimingMaterialInternal.*/
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE UMaterialInterface* GetAimingMaterial() const { return AimingMaterial; }

	/** Returns Trajectory Scale
	 * @see UGRSDataAsset::TrajectoryMeshScaleInternal */
	FORCEINLINE const FVector2D& GetTrajectoryMeshScale() const { return TrajectoryMeshScale; }

	/** Get collision asset class for the sides */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE TSubclassOf<class AActor> GetCollisionsAssetClass() const { return CollisionsAsset; }

	/** Returns the explosion damage gameplay effect applied when the bomb detonates.
	 * @see UGRSDataAsset::ExplosionDamageEffectInternal */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE TSubclassOf<UGameplayEffect> GetExplosionDamageEffectClass() const { return ExplosionDamageEffect; }

	/** Returns the player character revive gameplay effect applied to a player character.
	 * @see UGRSDataAsset::PlayerReviveEffectInternal */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE TSubclassOf<UGameplayEffect> GetPlayerReviveEffectClass() const { return PlayerReviveEffect; }

	/** Returns the trigger bomb placement tag */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE FGameplayTag GetTriggerBombTag() const { return TriggerBombTag; }

	/** Returns the revive player character trigger tag */
	UFUNCTION(BlueprintPure, BlueprintPure, Category = "[GhostRevengeSystem]")
	FORCEINLINE FGameplayTag GetRevivePlayerCharacterTriggerTag() const { return ReviveCharacterTriggerTag; }

protected:
	/** Grs Player Character Data Asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, ShowOnlyInnerProperties))
	TSubclassOf<class AGrsPawn> GrsActorClass = nullptr;

	/** Input mapping context for the GRSPlayerCharacter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Input Mapping Context", ShowOnlyInnerProperties))
	TObjectPtr<class UBmrInputMappingContext> InputContext = nullptr;

	/** A collision used to define the area where ghost character can move around. Placed on the sides of the map */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "A box collision asset transform spawned on sides of map"))
	FTransform CollisionTransform;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected))
	TSubclassOf<class AGrsBombProjectile> BombClass;

	/** Parameter to control trajectory visual display */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Should Display trajectory"))
	bool bEnableTrajectoryVisual = false;

	/** Parameter to control if bomb should be spawned once reached a max charging time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Spawn Bomb once Maximum Charge Time Reached"))
	bool bSpawnBombOnMaxChargingTime = false;

	/** Projectile path params */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Predict projectile path params"))
	FPredictProjectilePathParams PredictParams;

	/** Velocity of the prediction calculation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Trajectory Velocity multiplier in each of the directions"))
	FVector Velocity;

	/** A visual mesh that represents trajectory of aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected))
	TObjectPtr<UStaticMesh> AimingTrajectoryMesh = nullptr;

	/** A visual mesh that represents trajectory of aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected))
	TObjectPtr<UMaterialInterface> TrajectoryMaterial = nullptr;

	/** A visual mesh that represents aiming area */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected))
	TObjectPtr<UMaterialInterface> AimingMaterial = nullptr;

	/** Trajectory mesh scale for aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Trajectory Visual", meta = (BlueprintProtected, DisplayName = "Aim Trajectory Transform"))
	FVector2D TrajectoryMeshScale;

	/** Spawn location of Ghost Character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Temporary", meta = (BlueprintProtected, DisplayName = "Ghost Character Spawn Location"))
	FVector SpawnLocation;

	/** Aiming area mesh element */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem] | Temporary", meta = (BlueprintProtected))
	TObjectPtr<UStaticMesh> AimingAreaStaticMesh = nullptr;

	/** Asset that contains scalable collision. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Collisions Asset", ShowOnlyInnerProperties))
	TSubclassOf<class AActor> CollisionsAsset = nullptr;

	/** Explosion damage gameplay effect applied when the bomb detonates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Explosion Damage Effect", ShowOnlyInnerProperties))
	TSubclassOf<UGameplayEffect> ExplosionDamageEffect = nullptr;

	/** Player revive gameplay effect player character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (BlueprintProtected, DisplayName = "Player Revive Effect", ShowOnlyInnerProperties))
	TSubclassOf<UGameplayEffect> PlayerReviveEffect = nullptr;

	/** A tag used for GAS to trigger bomb placement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (Categories = "Event", BlueprintProtected, DisplayName = "Trigger Bomb Tag", ShowOnlyInnerProperties))
	FGameplayTag TriggerBombTag = FGameplayTag::EmptyTag;

	/** A tag used for GAS to trigger bomb placement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "[GhostRevengeSystem]", meta = (Categories = "Event", BlueprintProtected, DisplayName = "Revive Player Character Tag", ShowOnlyInnerProperties))
	FGameplayTag ReviveCharacterTriggerTag = FGameplayTag::EmptyTag;
};
