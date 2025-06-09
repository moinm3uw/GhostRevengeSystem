// Copyright (c) Valerii Rotermel &  Yevhenii Selivanov


#include "Components/GhostRevengeSystemSpotComponent.h"

#include "Components/MySkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Sets default values for this component's properties
UGhostRevengeSystemSpotComponent::UGhostRevengeSystemSpotComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

UMySkeletalMeshComponent* UGhostRevengeSystemSpotComponent::GetMySkeletalMeshComponent() const
{
	return GetOwner()->FindComponentByClass<UMySkeletalMeshComponent>();
}


UMySkeletalMeshComponent& UGhostRevengeSystemSpotComponent::GetMeshChecked() const
{
	UMySkeletalMeshComponent* Mesh = GetMySkeletalMeshComponent();
	checkf(Mesh, TEXT("'Mesh' is nullptr, can not get mesh for '%s' spot."), *GetNameSafe(this));
	return *Mesh;
}

// Called when the game starts
void UGhostRevengeSystemSpotComponent::BeginPlay()
{
	Super::BeginPlay();

	UGRSWorldSubSystem::Get().RegisterSpotComponent(this);
}
