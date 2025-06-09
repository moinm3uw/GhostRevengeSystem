// Copyright (c) Valerii Rotermel &  Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostRevengeSystemSpotComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeSystemSpotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGhostRevengeSystemSpotComponent();

	/** Returns the Skeletal Mesh of the Bomber character. */
	UFUNCTION(BlueprintPure, Category = "C++")
	class UMySkeletalMeshComponent* GetMySkeletalMeshComponent() const;
	class UMySkeletalMeshComponent& GetMeshChecked() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
};
