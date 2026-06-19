// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "DalPrimaryDataAsset.h"

// UE
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStaticsTypes.h" // FPredictProjectilePathParams

#include "GRSDataAsset.generated.h"

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

	// @PR JanSeliv [Coding Standards] - use module category "[GhostRevengeSystem]", "C++" reserved for editor-utils plugins, applies across file
	/** Returns the Grs player character class */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	TSubclassOf<class AGrsPawn> GetGrsActorClass() const { return GrsActorClass; }

	// @PR JanSeliv [Coding Standards] - pure getter pairs BlueprintCallable with BlueprintPure per project convention, bare BlueprintPure used nowhere else, applies across file
	/** Returns if the display of trajectory is enabled */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE bool ShouldDisplayTrajectory() const { return bEnableTrajectoryVisualInternal; }

	/** Returns if the display of trajectory is enabled */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE bool ShouldSpawnBombOnMaxChargeTime() const { return bSpawnBombOnMaxChargingTimeInternal; }

	// @PR JanSeliv [Coding Standards] - return struct getter by const&, by-value copies whole struct each call, applies across file (FVector, FTransform, FVector2D, FPredictProjectilePathParams getters)
	/** Returns spawn location */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FVector GetSpawnLocation() const { return SpawnLocationInternal; }

	/** Returns collision transform */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE FTransform GetCollisionTransform() const { return CollisionTransformInternal; }

	/** Returns input context.
	 * @see UGRSDataAsset::InputContextsInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UBmrInputMappingContext* GetInputContext() const { return InputContextInternal; }

	// @PR JanSeliv [Coding Standards] - drop class keyword before TSubclassOf, template alias not class type, inner AGrsBombProjectile already forward-declared
	/** Returns projectile class
	 * @see UGRSDataAsset::BombClass.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class TSubclassOf<class AGrsBombProjectile> GetProjectileClass() const { return BombClass; }

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

	// @PR JanSeliv [Coding Standards] - type repeats 3+ times, forward declare once at top instead of inline class specifier each use, applies across file (UMaterialInterface, UGameplayEffect)
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

	// @PR JanSeliv [Coding Standards] - getter returning TSubclassOf ends with Class postfix, applies across file (also GetPlayerReviveEffect)
	/** Returns the explosion damage gameplay effect applied when the bomb detonates.
	 * @see UGRSDataAsset::ExplosionDamageEffectInternal */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE TSubclassOf<class UGameplayEffect> GetExplosionDamageEffect() const { return ExplosionDamageEffectInternal; }

	/** Returns the player character revive gameplay effect applied to a player character.
	 * @see UGRSDataAsset::PlayerReviveEffectInternal */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE TSubclassOf<class UGameplayEffect> GetPlayerReviveEffect() const { return PlayerReviveEffectInternal; }

	/** Returns the trigger bomb placement tag */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE FGameplayTag GetTriggerBombTag() const { return TriggerBombTag; }

	// @PR JanSeliv [Coding Standards] - fix typo in getter name, "Revie" should be "Revive", BP-exposed name ships misspelled
	/** Returns the revive player character trigger tag */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE FGameplayTag GetReviePlayerCharacterTriggerTag() const { return ReviveCharacterTriggerTag; }

protected:
	// @PR JanSeliv [Coding Standards] - protected member needs Internal suffix per module convention, applies across file (BombClass, AimingAreaStaticMesh, TriggerBombTag, ReviveCharacterTriggerTag)
	/** Grs Player Character Data Asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	TSubclassOf<class AGrsPawn> GrsActorClass = nullptr;

	// @PR JanSeliv [Coding Standards] - init member in .h, TObjectPtr = nullptr and bool = false, applies across file (InputContextInternal, BombClass, bEnableTrajectoryVisualInternal, bSpawnBombOnMaxChargingTimeInternal, mesh/material TObjectPtr members)
	/** Input mapping context for the GRSPlayerCharacter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Input Mapping Context", ShowOnlyInnerProperties))
	TObjectPtr<class UBmrInputMappingContext> InputContextInternal;

	// @PR JanSeliv [Coding Standards] - fix typo "transofrm" in editor DisplayName, applies across file (also TrajectoryMeshScaleInternal DisplayName)
	// @PR JanSeliv [Coding Standards] - designer-tweakable data asset member is EditDefaultsOnly + BlueprintReadOnly, code reads via const getter, applies across file (all EditAnywhere/BlueprintReadWrite members)
	/** A collision used to define the area where ghost character can move around. Placed on the sides of the map */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "A box collision asset transofrm spawned on sides of map"))
	FTransform CollisionTransformInternal;

	// @PR JanSeliv [Coding Standards] - protected BP-exposed member missing meta = (BlueprintProtected), C++ access must mirror in BP, applies across file (AimingTrajectoryMeshInternal, TrajectoryMaterialInternal, AimingMaterialInternal, AimingAreaStaticMesh)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TSubclassOf<class AGrsBombProjectile> BombClass;

	// @PR JanSeliv [Coding Standards] - custom category must reflect module name in brackets "[GhostRevengeSystem]", not bare grouping name, applies across file (all "Trajectory Visual" and "Temporarry" members)
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

	// @PR JanSeliv [Coding Standards] - fix typo "Temporarry" in BP Category, applies across file (also AimingAreaStaticMesh)
	/** Spawn location of Ghost Character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporarry", meta = (BlueprintProtected, DisplayName = "Ghost Character Spawn Location"))
	FVector SpawnLocationInternal;

	/** Aiming area mesh element */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temporarry")
	TObjectPtr<UStaticMesh> AimingAreaStaticMesh;

	// @PR JanSeliv [Coding Standards] - UPROPERTY missing Category specifier, add Category = "[GhostRevengeSystem]", applies across file (GrsActorClass, ExplosionDamageEffectInternal, PlayerReviveEffectInternal, TriggerBombTag, ReviveCharacterTriggerTag)
	/** Asset that contains scalable collision. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Collisions Asset", ShowOnlyInnerProperties))
	TSubclassOf<class AActor> CollisionsAssetInternal = nullptr;

	/** Explosion damage gameplay effect applied when the bomb detonates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Explosion Damage Effect", ShowOnlyInnerProperties))
	TSubclassOf<class UGameplayEffect> ExplosionDamageEffectInternal = nullptr;

	// @PR JanSeliv [Coding Standards] - DisplayName "Player Death Effect" mislabels revive member, designer picks wrong effect, rename to "Player Revive Effect"
	/** Player revive gameplay effect player character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Player Death Effect", ShowOnlyInnerProperties))
	TSubclassOf<class UGameplayEffect> PlayerReviveEffectInternal = nullptr;

	// @PR JanSeliv [Coding Standards] - FGameplayTag member restricts editor picker via meta=(Categories="Root") to its tag subtree, applies across file (also ReviveCharacterTriggerTag)
	/** A tag used for GAS to trigger bomb placement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Trigger Bomb Tag", ShowOnlyInnerProperties))
	FGameplayTag TriggerBombTag = FGameplayTag::EmptyTag;

	/** A tag used for GAS to trigger bomb placement */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Revive Player Character Tag", ShowOnlyInnerProperties))
	FGameplayTag ReviveCharacterTriggerTag = FGameplayTag::EmptyTag;
};
