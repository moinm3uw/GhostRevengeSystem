// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GRSActorRep.generated.h"

UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGRSActorRep : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGRSActorRep();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
