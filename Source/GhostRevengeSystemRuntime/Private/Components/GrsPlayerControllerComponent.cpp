// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsPlayerControllerComponent.h"

// Grs
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Controllers/BmrPlayerController.h"
#include "DataAssets/BmrInputMappingContext.h"
#include "DataAssets/BmrPlayerInputDataAsset.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// MyEditorUtils
#include "MyUtilsLibraries/InputUtilsLibrary.h"
#include "Subsystems/GlobalMessageSubsystem.h"

// DataAssetsLoader
#include "DalSubsystem.h"

// GameFeaturePluginsManager
#include "GfpmUtils.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPlayerControllerComponent)

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
	checkf(MyPlayerController, TEXT("[%i] %hs:: 'MyPlayerController' is null"), __LINE__, __FUNCTION__);
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
	checkf(CurrentPawn, TEXT("[%i] %hs:: 'CurrentPawn' is null"), __LINE__, __FUNCTION__);
	return *CurrentPawn;
}

// Called when the game starts
void UGrsPlayerControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	ABmrPlayerController& PlayerControllerRef = GetPlayerControllerChecked();
	PlayerControllerRef.OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPossessedPawnChanged);
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Clears all transient data created by this component
void UGrsPlayerControllerComponent::OnUnregister()
{
	
	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	if (ABmrPlayerController* PlayerController = GetPlayerController())
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::OnPossessedPawnChanged);

		if (ABmrPlayerState* BmrPlayerState = PlayerController->GetPlayerState<ABmrPlayerState>())
		{
			BmrPlayerState->OnOpponentsKilledNumChanged.RemoveDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
		}
	}

	DisableGhostInputs(); // --- disables ghost input on local client
	UnpossessGhostPawn(); // --- unpossess ghost pawn

	Super::OnUnregister();
}

// Listen game states to reset player controller state
void UGrsPlayerControllerComponent::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
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

		APawn* CurrentPossessedPawn = GetCurrentPawn();
		ABmrPawn* CurrentPawn = Cast<ABmrPawn>(CurrentPossessedPawn);
		if (!ensureMsgf(CurrentPawn, TEXT("ASSERT: [%i] %hs:\n'CurrentPawn' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}

		MainBmrPlayerPawn = CurrentPawn;
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
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs (%s) Started \n "), __LINE__, __FUNCTION__, GetPlayerControllerChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	ABmrPlayerController* PlayerController = GetPlayerController();
	if (!MainBmrPlayerPawn
	    || !PlayerController
	    || !PlayerController->HasAuthority())
	{
		return;
	}

	// --- if pawn is empty possess back to BmrPawn
	APawn* CurrentPossessedPawn = PlayerController->GetPawn();
	if (!CurrentPossessedPawn)
	{
		// --- Always possess to player character when ghost character is no longer in control
		const bool bInDestroy = PlayerController->IsActorBeingDestroyed();
		if (!bInDestroy)
		{
			PlayerController->Possess(MainBmrPlayerPawn);
			ABmrPawn* NewPossessedPawn = CastChecked<ABmrPawn>(PlayerController->GetPawn());
			UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs pawn is empty. Possessed back to %s Expected: %s  "), __LINE__, __FUNCTION__, *GetNameSafe(NewPossessedPawn), *GetNameSafe(MainBmrPlayerPawn));
		}
	}
	else
	{
		// --- if pawn is ghost unpossess back to BmrPawn
		AGrsPawn* GhostPawn = Cast<AGrsPawn>(CurrentPossessedPawn);
		if (GhostPawn)
		{
			APlayerState* PlayerState = GhostPawn->GetPlayerState();
			checkf(PlayerState, TEXT("[%i] %hs:: 'PlayerState' failed to successful check obtain playerstate"), __LINE__, __FUNCTION__);

			UGrsPlayerStateComponent* GrsPlayerStateComponent = PlayerState->FindComponentByClass<UGrsPlayerStateComponent>();
			checkf(GrsPlayerStateComponent, TEXT("[%i] %hs:: 'GrsPlayerStateComponent' failed to successful check obtained component"), __LINE__, __FUNCTION__);
			/* @PR JanSeliv [Architecture] - UnpossessGhostPawn reaches into sibling actor GrsPlayerStateComponent and writes its non-replicated PreviousGrsPawn, revive decision then depends on externally accessed member (reset in 3 places) not actual kill signal.
			 * GrsPlayerStateComponent derives killer-ghost itself in OnOpponentsKilledNumChanged from own PlayerId, drop AssignPreviousGrsPawn and this cross-actor write */
			GrsPlayerStateComponent->AssignPreviousGrsPawn(GhostPawn);

			PlayerController->UnPossess();
			PlayerController->Possess(MainBmrPlayerPawn);
			ABmrPawn* NewPossessedPawn = Cast<ABmrPawn>(PlayerController->GetPawn());
			checkf(NewPossessedPawn, TEXT("[%i] %hs:: 'NewPossessedPawn' failed to successful possession completion check"), __LINE__, __FUNCTION__);
			UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs  Possessed to %s Expected: %s  "), __LINE__, __FUNCTION__, *GetNameSafe(NewPossessedPawn), *GetNameSafe(MainBmrPlayerPawn));
		}
	}

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

		TArray<UInputAction*> ContextInputActions;
		UInputUtilsLibrary::GetAllActionsInContext(PlayerController, InputContext, EInputActionInContextState::Any, /*out*/ ContextInputActions);
		UInputUtilsLibrary::UnbindInputActionsInContext(PlayerController, InputContext);
		UInputUtilsLibrary::SetInputContextEnabled(PlayerController, false, InputContext);
		UGfpmUtils::UnloadAssets(ContextInputActions);
	}
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/

// Enables or disable input  context (enhanced input) depends on possession state. Called when possessed pawn changed
void UGrsPlayerControllerComponent::OnPossessedPawnChanged_Implementation(APawn* OldPawn, APawn* NewPawn)
{
	// --- case 1: possessed to ghost character (condition: NewPawn is a ghost character
	if (!NewPawn)
	{
		return;
	}

	AGrsPawn* GhostCharacter = Cast<AGrsPawn>(NewPawn);
	if (!GhostCharacter)
	{
		return;
	}

	SetManagedInputContextEnabled(GetPlayerController(), true);
}

// Enables or disables the input context
void UGrsPlayerControllerComponent::SetManagedInputContextEnabled(AController* PlayerController, bool bEnable)
{
	const bool bIsLocalController = PlayerController->IsLocalController();
	if (!PlayerController || !bIsLocalController)
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: --- PlayerController is IsLocalController() %s "), __LINE__, __FUNCTION__, bIsLocalController ? TEXT("TRUE") : TEXT("FALSE"));
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
		HighestContextPriority = FMath::Max(HighestContextPriority, BmrInputContext->GetContextPriority());
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
		MyPlayerController->BindInputActionsInContext(InputContext);
		UInputUtilsLibrary::SetInputContextEnabled(this, bEnable, InputContext, HighestContextPriority);
	}
}

/*********************************************************************************************
 * Ghost Pawn Controller (Aiming, Throwing, Spawning bomb)
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

	// Get right vector
	const FVector RightDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::Y);

	APawn* GrsPawn = GetPlayerControllerChecked().GetPawn();
	GrsPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
	GrsPawn->AddMovementInput(RightDirection, MovementVector.X);
}

// Hold button to increase trajectory on button release trow bomb
void UGrsPlayerControllerComponent::ChargeBomb(const FInputActionValue& ActionValue)
{
	ShowVisualTrajectory();

	const UGRSDataAsset& GrsDataAsset = UGRSDataAsset::Get();
	if (CurrentHoldTime < GrsDataAsset.GetMaxChargingTime())
	{
		CurrentHoldTime = CurrentHoldTime + GetWorld()->GetDeltaSeconds();
	}
	else
	{
		if (GrsDataAsset.ShouldSpawnBombOnMaxChargeTime())
		{
			ThrowProjectile();
		}
		CurrentHoldTime = 0.0f;
	}

	// UE_LOG(LogGrs, Verbose, TEXT("GRS: Current hold time value: %f"), CurrentHoldTimeInternal);
}

//  Add and update visual representation of charging (aiming) progress as trajectory
void UGrsPlayerControllerComponent::ShowVisualTrajectory()
{
	AGrsPawn* GrsPawn = Cast<AGrsPawn>(GetPlayerControllerChecked().GetPawn());
	if (!GrsPawn)
	{
		return;
	}

	FPredictProjectilePathResult Result;

	// Configure PredictProjectilePath settings and get result
	PredictProjectilePath(Result);
	if (Result.PathData.IsEmpty())
	{
		// nothing is predicted, e.g. ghost side is not replicated yet, so aiming area would be placed to the world origin
		return;
	}

	// Aiming area - show visual element in the of predicted end
	UStaticMeshComponent* AimingStaticMeshComponent = GrsPawn->GetAimingSphereComponent();
	if (ensureMsgf(AimingStaticMeshComponent, TEXT("ASSERT: [%i] %hs:\n'AimingStaticMeshComponent' is not present on GrsPawn!"), __LINE__, __FUNCTION__))
	{
		AimingStaticMeshComponent->SetVisibility(true);
		AimingStaticMeshComponent->SetWorldLocation(Result.LastTraceDestination.Location);
	}

	// show trajectory visual
	if (UGRSDataAsset::Get().ShouldDisplayTrajectory())
	{
		AddSplinePoints(Result);
		AddSplineMesh(Result);
	}
}

// Add spline points to the aiming spline component
void UGrsPlayerControllerComponent::AddSplinePoints(FPredictProjectilePathResult& OutResult)
{
	AGrsPawn* GrsPawn = Cast<AGrsPawn>(GetCurrentPawn());
	if (!ensureMsgf(GrsPawn, TEXT("ASSERT: [%i] %hs:\n'GrsPawn' is not currently possess by this controller!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	USplineComponent* AimingSplineComponent = GrsPawn->GetAimingSplineComponent();
	if (!ensureMsgf(AimingSplineComponent, TEXT("ASSERT: [%i] %hs:\n'AimingStaticMeshComponent' is not present on GrsPawn!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// --- points are rebuilt on each aiming frame, so previous ones are removed first
	AimingSplineComponent->ClearSplinePoints(/*bUpdateSpline*/ false);

	for (int32 Index = 0; Index < OutResult.PathData.Num(); Index++)
	{
		FVector SplinePoint = OutResult.PathData[Index].Location;
		AimingSplineComponent->AddSplinePointAtIndex(SplinePoint, Index, ESplineCoordinateSpace::World);
	}

	AimingSplineComponent->SetSplinePointType(OutResult.PathData.Num() - 1, ESplinePointType::CurveClamped, true);
	AimingSplineComponent->UpdateSpline();
}

// Add spline mesh to spline points
void UGrsPlayerControllerComponent::AddSplineMesh(FPredictProjectilePathResult& OutResult)
{
	AGrsPawn* GrsPawn = Cast<AGrsPawn>(GetCurrentPawn());
	if (!ensureMsgf(GrsPawn, TEXT("ASSERT: [%i] %hs:\n'GrsPawn' is not currently possess by this controller!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	USplineComponent* AimingSplineComponent = GrsPawn->GetAimingSplineComponent();
	if (!ensureMsgf(AimingSplineComponent, TEXT("ASSERT: [%i] %hs:\n'AimingStaticMeshComponent' is not present on GrsPawn!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Spline points are not changed within the loop, only spline meshes are placed along them
	const int32 SplinePointsNum = AimingSplineComponent->GetNumberOfSplinePoints();
	const int32 SplineMeshesNum = FMath::Max(SplinePointsNum - 2, 0); // avoiding negative number here for short predicted trajectory  less than 2 points
	for (int32 Index = 0; Index < SplineMeshesNum; Index++)
	{
		// --- mesh is pooled on the pawn, so aiming each frame only updates existing meshes instead of creating new components
		USplineMeshComponent* SplineMesh = GrsPawn->GetOrCreateAimingSplineMesh(Index);
		if (!SplineMesh)
		{
			// Pool refused the index, so the rest of the trajectory can't be shown
			break;
		}

		// --- spline mesh takes local space: it's attached to the spline with no offset, so spline local space is used, which also follows the pawn
		const FVector LocationStart = AimingSplineComponent->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		const FVector LocationEnd = AimingSplineComponent->GetLocationAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local);
		const FVector TangentStart = AimingSplineComponent->GetTangentAtSplinePoint(Index, ESplineCoordinateSpace::Local);
		const FVector TangentEnd = AimingSplineComponent->GetTangentAtSplinePoint(Index + 1, ESplineCoordinateSpace::Local);
		SplineMesh->SetStartAndEnd(LocationStart, TangentStart, LocationEnd, TangentEnd);
		SplineMesh->SetVisibility(true);
	}

	// --- hide meshes left from a longer trajectory of a previous aiming frame
	GrsPawn->HideAimingSplineMeshes(SplineMeshesNum);
}

// Configure PredictProjectilePath settings and get result
void UGrsPlayerControllerComponent::PredictProjectilePath(FPredictProjectilePathResult& PredictResult)
{
	// Set launch velocity (forward direction with some upward angle)
	FVector LaunchVelocity = UGRSDataAsset::Get().GetVelocityParams();

	APawn& CurrentPawn = GetCurrentPawnChecked();

	// 45-degree vector between up and right
	FVector UpRight45 = (CurrentPawn.GetActorForwardVector() + CurrentPawn.GetActorUpVector()).GetSafeNormal();

	// Predict and draw the trajectory
	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = CurrentPawn.GetActorLocation();

	// --- pick a direction based on the side of the map (left or right) the server allocated for this ghost
	const AGrsPawn* GrsPawn = Cast<AGrsPawn>(&CurrentPawn);
	const UGrsPlayerStateComponent* GrsPlayerStateComponent = GrsPawn ? GrsPawn->GetGrsPlayerStateComponent() : nullptr;
	const EGRSCharacterSide GhostSide = GrsPlayerStateComponent ? GrsPlayerStateComponent->GetGhostSide() : EGRSCharacterSide::None;
	if (GhostSide == EGRSCharacterSide::None)
	{
		return;
	}

	const float SideSign = GhostSide == EGRSCharacterSide::Left ? 1.0f : -1.0f;

	Params.LaunchVelocity = FVector(UpRight45.X + SideSign * (LaunchVelocity.X * CurrentHoldTime), LaunchVelocity.Y, UpRight45.Z + LaunchVelocity.Z);
	Params.ActorsToIgnore.Add(&CurrentPawn);

	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, PredictResult);
}

// Throw projectile event, bound to onetime button press
void UGrsPlayerControllerComponent::ThrowProjectile()
{
	AGrsPawn* GrsPawn = Cast<AGrsPawn>(GetPlayerControllerChecked().GetPawn());
	if (!GrsPawn)
	{
		return;
	}

	UStaticMeshComponent* AimingStaticMeshComponent = GrsPawn->GetAimingSphereComponent();
	if (!ensureMsgf(AimingStaticMeshComponent, TEXT("ASSERT: [%i] %hs:\n'AimingStaticMeshComponent' is not present on GrsPawn!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	//--- Calculate Cell to spawn bomb
	FBmrCell TargetCell;
	TargetCell.Location = AimingStaticMeshComponent->GetComponentLocation();
	SpawnBomb(TargetCell);

	FVector ThrowDirection = GrsPawn->GetActorForwardVector() + FVector(5.0f, 5.0f, 0.0f);
	ThrowDirection.Normalize();

	CurrentHoldTime = 0.0f;
	GrsPawn->ClearTrajectorySplines();

	//--- hide aiming static mesh
	AimingStaticMeshComponent->SetVisibility(false);
	AimingStaticMeshComponent->SetWorldLocation(GrsPawn->GetActorLocation());
}

// Spawn bomb at aiming mesh location
void UGrsPlayerControllerComponent::SpawnBomb(const FBmrCell& TargetCell)
{
	AGrsPawn* GrsPawn = Cast<AGrsPawn>(GetCurrentPawn());
	if (!ensureMsgf(GrsPawn, TEXT("ASSERT: [%i] %hs:\n'GrsPawn' is not currently possess by this controller!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const FBmrCell& SpawnBombCell = UBmrCellUtilsLibrary::GetNearestFreeCell(TargetCell);

	// Activate bomb ability
	FGameplayEventData EventData;
	EventData.EventTag = UGRSDataAsset::Get().GetTriggerBombTag();
	EventData.Instigator = GrsPawn;
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(SpawnBombCell);
	UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
}
