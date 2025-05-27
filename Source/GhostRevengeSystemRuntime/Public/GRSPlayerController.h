// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "GRSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGRSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GRS_Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputMappingContext> CurrentInputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GRS_Input, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	TObjectPtr<AGRSPlayerCharacter> PlayerCharacter;

public:
	virtual void SetupInputComponent() override;

protected:
	virtual void OnPossess(APawn *InPawn) override;

	void Move(const struct FInputActionValue& ActionValue);
};
