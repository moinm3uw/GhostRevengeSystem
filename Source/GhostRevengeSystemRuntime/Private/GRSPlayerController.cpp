// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "GRSPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"

void AGRSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = Cast<UEnhancedInputComponent>(this->InputComponent);
	if (EnhancedInputComponent)
	{
		// Move
		EnhancedInputComponent->BindAction(MoveAction.Get(), ETriggerEvent::Triggered, this, &AGRSPlayerController::Move);
	}
}

void AGRSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	this->PlayerCharacter = Cast<AGRSPlayerCharacter>(InPawn);

	TObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputLolcalPlayerSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer());
	if (InputLolcalPlayerSubsystem)
	{
		InputLolcalPlayerSubsystem->AddMappingContext(this->CurrentInputMappingContext.Get(), 0);
	}
}

void AGRSPlayerController::Move(const struct FInputActionValue& ActionValue)
{
	const FVector2d MoveMentVector = ActionValue.Get<FVector2d>();

	const FRotator Rotation = this->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	//	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxes(EAxis::X);
	//	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxes(EAxis::Y);

	//	this->PlayerCharacter->AddMovementInput(ForwardDirection, MoveMentVector.Y);
	//	this->PlayerCharacter->AddMovementInput(RightDirection, MoveMentVector.X);
}
