// Copyright (c) Yevhenii Selivanov

#include "Components/GrsPlayerControllerComponent.h"

#include "Components/GrsPlayerStateComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "DalSubsystem.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/BmrInputAction.h"
#include "DataAssets/BmrInputMappingContext.h"
#include "DataAssets/BmrPlayerInputDataAsset.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/BmrPlayerState.h"
#include "GrsUtils.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GrsPawn.h"
#include "MyUtilsLibraries/InputUtilsLibrary.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPlayerControllerComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGrsPlayerControllerComponent::UGrsPlayerControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Returns Player Controller of this component
ABmrPlayerController* UGrsPlayerControllerComponent::GetPlayerController() const
{
	return Cast<ABmrPlayerController>(GetOwner());
}

ABmrPlayerController& UGrsPlayerControllerComponent::GetPlayerControllerChecked() const
{
	ABmrPlayerController* MyPlayerController = GetPlayerController();
	checkf(MyPlayerController, TEXT("%s: 'MyPlayerController' is null"), *FString(__FUNCTION__));
	return *MyPlayerController;
}

// Returns current possessed pawn
APawn* UGrsPlayerControllerComponent::GetCurrentPawn() const
{
	return GetPlayerControllerChecked().GetPawn();
}

// Returns current possessed pawn with checkf
APawn& UGrsPlayerControllerComponent::GetCurrentPawnChecked() const
{
	APawn* CurrentPawn = GetPlayerControllerChecked().GetPawn();
	checkf(CurrentPawn, TEXT("%s: 'CurrentPawn' is null"), *FString(__FUNCTION__));
	return *CurrentPawn;
}

// Called when the game starts
void UGrsPlayerControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	GetPlayerControllerChecked().OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPossessedPawnChanged);
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	APawn& CurrentPawn = GetCurrentPawnChecked();
	ABmrPlayerState* BmrPlayerState = Cast<ABmrPlayerState>(CurrentPawn.GetPlayerState());
	checkf(BmrPlayerState, TEXT("ERROR: [%i] %hs:\n'PlayerState' is null!"), __LINE__, __FUNCTION__);
	BmrPlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);
}

// Clears all transient data created by this component
void UGrsPlayerControllerComponent::OnUnregister()
{
	DisableGhostInputs(); // --- disables ghost input on local client
	UnpossessGhostPawn(); // --- unpossess ghost pawn

	Super::OnUnregister();
}

// Called when player's match result was changed (Win, lose, draw or none applied).
void UGrsPlayerControllerComponent::OnEndGameStateChanged_Implementation(EBmrEndGameState EndGameState)
{
	if (EndGameState == EBmrEndGameState::Lose)
	{
	}
}

// Listen game states to reset player controller state
void UGrsPlayerControllerComponent::OnGameStateChanged_Implementation(const struct FGameplayEventData& Payload)
{
	// --- for cases when game is restarted or freshly started
	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::GameStarting))
	{
		DisableGhostInputs(); // --- disables ghost input on local client
		UnpossessGhostPawn(); // --- unpossess ghost pawn
	}

	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		ABmrPlayerState* BmrPlayerState = GetPlayerControllerChecked().GetPlayerState<ABmrPlayerState>();
		if (!ensureMsgf(BmrPlayerState, TEXT("ASSERT: [%i] %hs:\n'BmrPlayerState' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}
		BmrPlayerState->OnOpponentsKilledNumChanged.AddUniqueDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);

		AActor* CurrentOwner = GetOwner();
		APawn* CurrentPossessedPawn = GetCurrentPawn();
		ABmrPawn* CurrentPawn = Cast<ABmrPawn>(CurrentPossessedPawn);
		if (!ensureMsgf(CurrentPawn, TEXT("ASSERT: [%i] %hs:\n'CurrentPawn' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}

		if (MainBmrPlayerPawn != CurrentPawn)
		{
			MainBmrPlayerPawn = CurrentPawn;
		}
	}
}

// Is increased when this player kills an opponent
void UGrsPlayerControllerComponent::OnOpponentsKilledNumChanged_Implementation(int32 OpponentsKilledNum)
{
	// --- ignore reset cases
	if (OpponentsKilledNum < 1)
	{
		return;
	}

	DisableGhostInputs(); // --- disables ghost input on local client
	UnpossessGhostPawn(); // --- unpossess ghost pawn
}

// Unpossess current pawn from ghost to BmwPlayerPawn
void UGrsPlayerControllerComponent::UnpossessGhostPawn()
{
	ABmrPlayerController* PlayerController = GetPlayerController();
	if (!MainBmrPlayerPawn
	    || !PlayerController
	    || !PlayerController->HasAuthority())
	{
		return;
	}

	// --- if pawn is empty possess back to BmrPawn
	TObjectPtr<APawn> CurrentPossessedPawn = PlayerController->GetPawn();
	if (!CurrentPossessedPawn)
	{
		// --- Always possess to player character when ghost character is no longer in control
		PlayerController->Possess(MainBmrPlayerPawn);
	}
	else
	{
		// --- if pawn is ghost unpossess back to BmrPawn
		AGrsPawn* GhostPawn = Cast<AGrsPawn>(CurrentPossessedPawn);
		if (GhostPawn)
		{
			PlayerController->UnPossess();
			PlayerController->Possess(MainBmrPlayerPawn);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PlayerController is %s"), __LINE__, __FUNCTION__, PlayerController ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- MainPlayerPawn is %s"), __LINE__, __FUNCTION__, MainBmrPlayerPawn ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PlayerCharacter: %s"), __LINE__, __FUNCTION__, *GetNameSafe(MainBmrPlayerPawn));

	MainBmrPlayerPawn = nullptr; // --- reset player character reference
}

// Disables current enhanced input and input bindings
void UGrsPlayerControllerComponent::DisableGhostInputs()
{
	const ABmrPlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return;
	}

	// -- disable inputs
	if (const UGRSDataAsset* DataAsset = UDalSubsystem::GetDataAsset<UGRSDataAsset>())
	{
		const UBmrInputMappingContext* InputContext = DataAsset->GetInputContext();
		UInputUtilsLibrary::UnbindInputActionsInContext(PlayerController, InputContext);
		UInputUtilsLibrary::SetInputContextEnabled(PlayerController, false, InputContext);
	}
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/

// Enables or disable input  context (enhanced input) depends on possession state. Called when possessed pawn changed
void UGrsPlayerControllerComponent::OnPossessedPawnChanged_Implementation(APawn* OldPawn, APawn* NewPawn)
{
	// --- case 1: possessed to ghost character (condition: NewPawn is a ghost character)
	if (NewPawn)
	{
		AGrsPawn* GhostCharacter = Cast<AGrsPawn>(NewPawn);
		if (GhostCharacter)
		{
			SetManagedInputContextEnabled(GetPlayerController(), true);
		}
	}
}

// Enables or disables the input context
void UGrsPlayerControllerComponent::SetManagedInputContextEnabled(AController* PlayerController, bool bEnable)
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[%i] %hs: --- PlayerController is IsLocalController() %s"), __LINE__, __FUNCTION__, PlayerController->IsLocalController() ? TEXT("TRUE") : TEXT("FALSE"));
	ABmrPlayerController* MyPlayerController = Cast<ABmrPlayerController>(PlayerController);
	if (!MyPlayerController)
	{
		return;
	}

	UBmrInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();

	// --- due to strange stacking behavior in UE when 2nd time enable input context it is not the latest enabled we have to specify exactly contextPriority.
	// --- to do so we have not the best solution - override only conflicting InputMappingContext in our case BmrInputContext
	// --- maybe somewhere in future we will have context priority manager as better solution
	TArray<const UBmrInputMappingContext*> BmrInputContexts;
	UBmrPlayerInputDataAsset::Get().GetAllGameplayInputContexts(/*out*/ BmrInputContexts);

	int32 HighestContextPriority = -1;
	for (const UBmrInputMappingContext* BmrInputContext : BmrInputContexts)
	{
		if (HighestContextPriority < BmrInputContext->GetContextPriority())
		{
			HighestContextPriority = BmrInputContext->GetContextPriority();
		}
	}
	HighestContextPriority++;

	if (!bEnable)
	{
		// --- Remove related input contexts
		UInputUtilsLibrary::SetInputContextEnabled(this, bEnable, InputContext, HighestContextPriority);
		return;
	}

	// --- Remove all previous input context
	UInputUtilsLibrary::SetInputContextEnabled(this, false, InputContext, HighestContextPriority);

	// --- Add gameplay context as auto managed by Game State, so it will be enabled everytime the game is in the in-game state
	if (InputContext
	    && !InputContext->GetActiveForStates().IsEmpty())
	{
		if (InputContext)
		{
			MyPlayerController->BindInputActionsInContext(InputContext);
			UInputUtilsLibrary::SetInputContextEnabled(this, bEnable, InputContext, HighestContextPriority);
		}
	}
}

/*********************************************************************************************
 * Ghost Pawn Controller
 **********************************************************************************************/

// Move the player character
void UGrsPlayerControllerComponent::MovePlayer(const FInputActionValue& ActionValue)
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
void UGrsPlayerControllerComponent::ChargeBomb(const FInputActionValue& ActionValue)
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
			AGrsPawn* GhostCharacter = Cast<AGrsPawn>(GetPlayerControllerChecked().GetPawn());
			if (!GhostCharacter)
			{
				return;
			}
			GhostCharacter->ThrowProjectile();
		}
		CurrentHoldTimeInternal = 0;
	}

	// UE_LOG(LogTemp, Log, TEXT("GRS: Current hold time value: %f"), CurrentHoldTimeInternal);
}

//  Add and update visual representation of charging (aiming) progress as trajectory
void UGrsPlayerControllerComponent::ShowVisualTrajectory()
{
	AGrsPawn* GhostCharacter = Cast<AGrsPawn>(GetPlayerControllerChecked().GetPawn());
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
void UGrsPlayerControllerComponent::PredictProjectilePath(FPredictProjectilePathResult& PredictResult)
{
	// Set launch velocity (forward direction with some upward angle)
	FVector LaunchVelocity = UGRSDataAsset::Get().GetVelocityParams();
	// 45-degree vector between up and right
	FVector UpRight45 = (GetCurrentPawnChecked().GetActorForwardVector() + GetCurrentPawnChecked().GetActorUpVector()).GetSafeNormal();

	// Predict and draw the trajectory
	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = GetCurrentPawnChecked().GetActorLocation();

	// --- pick a direction based on the side of the map (left or right)
	const float SideSign = UGrsUtils::GetCharacterSideFromActor(Cast<AActor>(&GetCurrentPawnChecked())) == EGRSCharacterSide::Left ? 1.0f : -1.0f;

	Params.LaunchVelocity = FVector(UpRight45.X + SideSign * (LaunchVelocity.X * CurrentHoldTimeInternal), LaunchVelocity.Y, UpRight45.Z + LaunchVelocity.Z);
	Params.ActorsToIgnore.Add(GetCurrentPawn());

	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, PredictResult);
}

// Throw projectile event, bound to onetime button press
void UGrsPlayerControllerComponent::ThrowProjectile()
{
	AGrsPawn* GhostCharacter = Cast<AGrsPawn>(GetPlayerControllerChecked().GetPawn());
	if (!GhostCharacter)
	{
		return;
	}
	GhostCharacter->ThrowProjectile();
}
