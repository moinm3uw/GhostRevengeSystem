// Copyright (c) Yevhenii Selivanov


#include "GRSActorRep.h"


// Sets default values
AGRSActorRep::AGRSActorRep()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Replicate an actor
	bReplicates = true;
	SetReplicates(true);
	SetReplicateMovement(true);

	// Do not rotate player by camera
	bUseControllerRotationYaw = false;
}

// Called when the game starts or when spawned
void AGRSActorRep::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("AGRSPlayerCharacter::BeginPlay"));
}

// Called every frame
void AGRSActorRep::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
