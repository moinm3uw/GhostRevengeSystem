// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Data/MyPrimaryDataAsset.h"
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

protected:
	/** Test value for the data assest*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Test stirng"))
	FString TestDataAssetStringInternal;

	/** Spawn location of actor*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Spawn Location to play"))
	FVector SpawnLocationInternal;
};
