// Copyright (c) Yevhenii Selivanov

#include "Components/GRSPlayerControllerComponent.h"

#include "Controllers/BmrPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// Sets default values for this component's properties
UGRSPlayerControllerComponent::UGRSPlayerControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Returns Player Controller of this component
ABmrPlayerController* UGRSPlayerControllerComponent::GetPlayerController() const
{
	return Cast<ABmrPlayerController>(GetOwner());
}

ABmrPlayerController& UGRSPlayerControllerComponent::GetPlayerControllerChecked() const
{
	ABmrPlayerController* MyPlayerController = GetPlayerController();
	checkf(MyPlayerController, TEXT("%s: 'MyPlayerController' is null"), *FString(__FUNCTION__));
	return *MyPlayerController;
}

// Returns current possessed pawn
APawn* UGRSPlayerControllerComponent::GetCurrentGhostCharacter() const
{
	return GetPlayerControllerChecked().GetPawn();
}

// Returns current possessed pawn with checkf
APawn& UGRSPlayerControllerComponent::GetCurrentPawnChecked() const
{
	APawn* CurrentPawn = GetPlayerControllerChecked().GetPawn();
	checkf(CurrentPawn, TEXT("%s: 'CurrentPawn' is null"), *FString(__FUNCTION__));
	return *CurrentPawn;
}

// Called when the game starts
void UGRSPlayerControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	GetPlayerControllerChecked().OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPossessedPawnChanged);
}

// Clears all transient data created by this component
void UGRSPlayerControllerComponent::OnUnregister()
{
	Super::OnUnregister();
	PerformCleanUp();
}

// Enables or disable input  context (enhanced input) depends on possession state. Called when possessed pawn changed
void UGRSPlayerControllerComponent::OnPossessedPawnChanged_Implementation(APawn* OldPawn, APawn* NewPawn)
{
	// --- case 1: possessed to ghost character (condition: NewPawn is a ghost character)
	if (NewPawn)
	{
		AGRSPlayerCharacter* GhostCharacter = Cast<AGRSPlayerCharacter>(NewPawn);
		if (GhostCharacter)
		{
			SetManagedInputContextEnabled(GetPlayerController(), true);

			// --- Clear splines
			GhostCharacter->ClearTrajectorySplines();
			GhostCharacter->ApplyExplosionGameplayEffect();
		}
	}

	// --- case 2: unpossess ghost character (OldPawn is a ghost character)
	if (OldPawn)
	{
		AGRSPlayerCharacter* GhostCharacter = Cast<AGRSPlayerCharacter>(OldPawn);
		if (GhostCharacter)
		{
			SetManagedInputContextEnabled(GetPlayerController(), false);
			GhostCharacter->RemoveActiveGameplayEffect();
		}
	}
}

// Enables or disables the input context
void UGRSPlayerControllerComponent::SetManagedInputContextEnabled(AController* PlayerController, bool bEnable)
{
	if (!PlayerController && PlayerController->IsLocalController())
	{
		return;
	}

	ABmrPlayerController* MyPlayerController = Cast<ABmrPlayerController>(PlayerController);
	if (!MyPlayerController)
	{
		return;
	}

	TArray<const UBmrInputMappingContext*> InputContexts;
	UBmrInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
	InputContexts.AddUnique(InputContext);

	if (!bEnable)
	{
		// --- Remove related input contexts
		MyPlayerController->RemoveInputContexts(InputContexts);
		return;
	}

	// --- Remove all previous input contexts managed by Controller
	MyPlayerController->RemoveInputContexts(InputContexts);

	// --- Add gameplay context as auto managed by Game State, so it will be enabled everytime the game is in the in-game state
	if (InputContext
	    && !InputContext->GetActiveForStates().IsEmpty())
	{
		MyPlayerController->SetupInputContexts(InputContexts);
	}
}

// Move the player character
void UGRSPlayerControllerComponent::MovePlayer(const FInputActionValue& ActionValue)
{
	if (GetPlayerControllerChecked().IsMoveInputIgnored())
	{
		return;
	}

	// input is a Vector2D
	const FVector2D MovementVector = ActionValue.Get<FVector2D>();

	// Find out which way is forward
	const FRotator ForwardRotation = UBmrCellUtilsLibrary::GetLevelGridRotation();

	// Get forward vector
	const FVector ForwardDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::X);
	// const FVector ForwardDirection = FVector().ZeroVector;

	// Get right vector
	const FVector RightDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::Y);
	// const FVector RightDirection = FVector().ZeroVector;;

	APawn* CurrentPawn = GetPlayerControllerChecked().GetPawn();
	if (!ensureMsgf(CurrentPawn, TEXT("ASSERT: [%i] %hs:\n'CurrentPawn' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	CurrentPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
	CurrentPawn->AddMovementInput(RightDirection, MovementVector.X);
}

// Hold button to increase trajectory on button release trow bomb
void UGRSPlayerControllerComponent::ChargeBomb(const FInputActionValue& ActionValue)
{
	ShowVisualTrajectory();

	if (CurrentHoldTimeInternal < 1.0f)
	{
		CurrentHoldTimeInternal = CurrentHoldTimeInternal + GetWorld()->GetDeltaSeconds();
	}
	else
	{
		if (UGRSDataAsset::Get().ShouldSpawnBombOnMaxChargeTime())
		{
			AGRSPlayerCharacter* GhostCharacter = Cast<AGRSPlayerCharacter>(GetPlayerControllerChecked().GetPawn());
			if (!GhostCharacter)
			{
				return;
			}
			GhostCharacter->ThrowProjectile();
		}
		CurrentHoldTimeInternal = 0;
	}

	UE_LOG(LogTemp, Log, TEXT("GRS: Current hold time value: %f"), CurrentHoldTimeInternal);
}

//  Add and update visual representation of charging (aiming) progress as trajectory
void UGRSPlayerControllerComponent::ShowVisualTrajectory()
{
	AGRSPlayerCharacter* GhostCharacter = Cast<AGRSPlayerCharacter>(GetPlayerControllerChecked().GetPawn());
	if (!GhostCharacter)
	{
		return;
	}

	FPredictProjectilePathResult Result;

	// Configure PredictProjectilePath settings and get result
	PredictProjectilePath(Result);

	// Aiming area - show visual element in the of predicted end
	GhostCharacter->AddMeshToEndProjectilePath(Result.LastTraceDestination.Location);

	// show trajectory visual
	if (UGRSDataAsset::Get().ShouldDisplayTrajectory() && Result.PathData.Num() > 0)
	{
		GhostCharacter->AddSplinePoints(Result);
		GhostCharacter->AddSplineMesh(Result);
	}
}

// Configure PredictProjectilePath settings and get result
void UGRSPlayerControllerComponent::PredictProjectilePath(FPredictProjectilePathResult& PredictResult)
{
	// Set launch velocity (forward direction with some upward angle)
	FVector LaunchVelocity = UGRSDataAsset::Get().GetVelocityParams();
	// 45-degree vector between up and right
	FVector UpRight45 = (GetCurrentPawnChecked().GetActorForwardVector() + GetCurrentPawnChecked().GetActorUpVector()).GetSafeNormal();

	// Predict and draw the trajectory
	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = GetCurrentPawnChecked().GetActorLocation();

	// --- pick a direction based on the side of the map (left or right)
	const float SideSign = UGRSWorldSubSystem::Get().GetCharacterSideFromActor(Cast<AActor>(&GetCurrentPawnChecked())) == EGRSCharacterSide::Left ? 1.0f : -1.0f;

	Params.LaunchVelocity = FVector(UpRight45.X + SideSign * (LaunchVelocity.X * CurrentHoldTimeInternal), LaunchVelocity.Y, UpRight45.Z + LaunchVelocity.Z);
	Params.ActorsToIgnore.Add(GetCurrentGhostCharacter());

	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, PredictResult);
}

// Throw projectile event, bound to onetime button press
void UGRSPlayerControllerComponent::ThrowProjectile()
{
	AGRSPlayerCharacter* GhostCharacter = Cast<AGRSPlayerCharacter>(GetPlayerControllerChecked().GetPawn());
	if (!GhostCharacter)
	{
		return;
	}
	GhostCharacter->ThrowProjectile();
}

// Clean up the character for the MGF unload
void UGRSPlayerControllerComponent::PerformCleanUp()
{
	ABmrPlayerController* PlayerController = Cast<ABmrPlayerController>(GetOwner());
	if (!PlayerController)
	{
		return;
	}

	// Remove all previous input contexts managed by Controller
	TArray<const UBmrInputMappingContext*> InputContexts;
	const UBmrInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
	InputContexts.AddUnique(InputContext);
	PlayerController->RemoveInputContexts(InputContexts);
}