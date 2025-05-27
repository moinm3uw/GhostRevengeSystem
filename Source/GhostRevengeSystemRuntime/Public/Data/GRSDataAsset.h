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

	/** Returns input context. 
	 * @see UGRSDataAsset::InputContextsInternal.*/
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE class UMyInputMappingContext* GetInputContext() const { return InputContextInternal; }
	
protected:
	/** Test value for the data assest*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Test stirng"))
	FString TestDataAssetStringInternal;

	/** Spawn location of actor*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Spawn Location to play"))
	FVector SpawnLocationInternal;

	/** Input context for the GRSPlayerCharacter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (BlueprintProtected, DisplayName = "Input Context", ShowOnlyInnerProperties))
	TObjectPtr<class UMyInputMappingContext> InputContextInternal;
};
