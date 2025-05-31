// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Data/MyPrimaryDataAsset.h"
#include "DataAssets/MyInputMappingContext.h"
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
	FORCEINLINE class UStaticMesh* GetProjectileMesh() const { return StaticMesh; }
	
protected:
	/** Test value for the data assest*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Test stirng"))
	FString TestDataAssetStringInternal;

	/** Spawn location of Ghost Character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Ghost Character Spawn location"))
	FVector SpawnLocationInternal;

	/** Collision Asset transform of actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Collision Asset Transofrm"))
	FTransform CollisionTransformInternal;

	/** Input context for the GRSPlayerCharacter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Input Context", ShowOnlyInnerProperties))
	TObjectPtr<class UMyInputMappingContext> InputContextInternal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
	TSubclassOf<class AGRSBombProjectile> BombClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
	TObjectPtr<UStaticMesh> StaticMesh;
};
