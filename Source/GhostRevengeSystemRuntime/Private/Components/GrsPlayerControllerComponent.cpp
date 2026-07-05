// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsPlayerControllerComponent.h"

// Grs
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "GrsUtils.h"
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Controllers/BmrPlayerController.h"
#include "DataAssets/BmrInputMappingContext.h"
#include "DataAssets/BmrPlayerInputDataAsset.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"
#include "Actors/BmrPawn.h"

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

// @PR JanSeliv [Coding Standards] - .cpp with reflection in own .h must enable UE_INLINE_GENERATED_CPP_BY_NAME, uncomment after all includes + 1 blank line
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
	// @PR JanSeliv [Coding Standards] - use %hs with __FUNCTION__, drop *FString() wrap, applies across file
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

	// @PR JanSeliv [Coding Standards] - GetPlayerControllerChecked() called repeatedly here, cache once to local ref and reuse, applies across file
	GetPlayerControllerChecked().OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::OnPossessedPawnChanged);
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	ABmrPlayerState* BmrPlayerState = GetPlayerControllerChecked().GetPlayerState<ABmrPlayerState>();
	// @PR JanSeliv [Coding Standards] - GetOwner() deref without null-check, and SERVER\CLIENT HasAuthority ternary duplicated across log sites, cache HasAuthority to local, applies across file
	UE_CLOG(!BmrPlayerState, LogGrs, Verbose, TEXT("[%i] %hs (%s) 'BmrPlayerState' is null, which is expected (BeginPlay is too early)"), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	if (BmrPlayerState)
	{
		BmrPlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);
	}
}

// @PR JanSeliv [Coding Standards] - listeners added in BeginPlay (OnPossessedPawnChanged, GameState_Changed, OnEndGameStateChanged) and OnOpponentsKilledNumChanged lack matching Remove here, add RemoveDynamic + StopListeningForAllGlobalMessages like neighbor GrsPlayerStateComponent cleanup
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
	// @PR JanSeliv [Coding Standards] - empty if body, remove dead branch or implement
	if (EndGameState == EBmrEndGameState::Lose)
	{
	}
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

		// @PR JanSeliv [Coding Standards] - CurrentOwner unused local, remove dead variable
		AActor* CurrentOwner = GetOwner();
		// @PR JanSeliv [Coding Standards] - read-only local pointer, make const-pointee const APawn*, applies across file
		APawn* CurrentPossessedPawn = GetCurrentPawn();
		ABmrPawn* CurrentPawn = Cast<ABmrPawn>(CurrentPossessedPawn);
		if (!ensureMsgf(CurrentPawn, TEXT("ASSERT: [%i] %hs:\n'CurrentPawn' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}

		// @PR JanSeliv [Coding Standards] - redundant guard, assign MainBmrPlayerPawn = CurrentPawn directly, same result
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
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs (%s) Started \n "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	ABmrPlayerController* PlayerController = GetPlayerController();
	if (!MainBmrPlayerPawn
	    || !PlayerController
	    || !PlayerController->HasAuthority())
	{
		return;
	}

	// --- if pawn is empty possess back to BmrPawn
	// @PR JanSeliv [Coding Standards] - TObjectPtr is for .h UObject members, local uses raw APawn* (const-pointee, read-only)
	TObjectPtr<APawn> CurrentPossessedPawn = PlayerController->GetPawn();
	if (!CurrentPossessedPawn)
	{
		// --- Always possess to player character when ghost character is no longer in control
		// @PR JanSeliv [Coding Standards] - read-only value local needs const, const bool bInDestroy, applies across file (FVector SplinePoint, TangentStart, TangentEnd)
		bool bInDestroy = PlayerController->IsActorBeingDestroyed();
		if (!bInDestroy)
		{
			PlayerController->Possess(MainBmrPlayerPawn);
			// @PR JanSeliv [Coding Standards] - Cast + checkf, use CastChecked<ABmrPawn>, applies across file
			ABmrPawn* NewPossessedPawn = Cast<ABmrPawn>(PlayerController->GetPawn());
			checkf(NewPossessedPawn, TEXT("%s: 'NewPossessedPawn' failed to check possession completion"), *FString(__FUNCTION__));
			UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs pawn is empty. Possessed back to %s Expected: %s  "), __LINE__, __FUNCTION__, *GetNameSafe(NewPossessedPawn), *GetNameSafe(MainBmrPlayerPawn));
		}
	}
	else
	{
		// --- if pawn is ghost unpossess back to BmrPawn
		AGrsPawn* GhostPawn = Cast<AGrsPawn>(CurrentPossessedPawn);
		if (GhostPawn)
		{
			// @PR JanSeliv [Potential Bug] - GetPlayerState() can be null during unpossess/teardown, deref via ->FindComponentByClass crashes, cache to local and null-guard first
			UGrsPlayerStateComponent* GrsPlayerStateComponent = GhostPawn->GetPlayerState()->FindComponentByClass<UGrsPlayerStateComponent>();
			checkf(GrsPlayerStateComponent, TEXT("%s: 'GrsPlayerStateComponent' failed to check obtain component"), *FString(__FUNCTION__));
			/* @PR JanSeliv [Architecture] - UnpossessGhostPawn reaches into sibling actor GrsPlayerStateComponent and writes its non-replicated PreviousGrsPawn, revive decision then depends on externally accessed member (reset in 3 places) not actual kill signal.
			 * GrsPlayerStateComponent derives killer-ghost itself in OnOpponentsKilledNumChanged from own PlayerId, drop AssignPreviousGrsPawn and this cross-actor write */
			GrsPlayerStateComponent->AssignPreviousGrsPawn(GhostPawn);

			PlayerController->UnPossess();
			PlayerController->Possess(MainBmrPlayerPawn);
			ABmrPawn* NewPossessedPawn = Cast<ABmrPawn>(PlayerController->GetPawn());
			checkf(NewPossessedPawn, TEXT("%s: 'NewPossessedPawn' failed to check possession completion"), *FString(__FUNCTION__));
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
	// --- case 1: possessed to ghost character (condition: NewPawn is a ghost character)
	// @PR JanSeliv [Coding Standards] - flatten nested if with guard clauses, early return `if (!NewPawn) return;` then Cast then `if (!GhostCharacter) return;`, matches early-return style used across file
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

	// @PR JanSeliv [Coding Standards] - IsLocalController() re-called here, guard above already returned when false so log always prints TRUE, drop redundant call
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: --- PlayerController is IsLocalController() %s "), __LINE__, __FUNCTION__, PlayerController->IsLocalController() ? TEXT("TRUE") : TEXT("FALSE"));
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
		// @PR JanSeliv [Coding Standards] - hand-written max with double GetContextPriority() call, use HighestContextPriority = FMath::Max(HighestContextPriority, BmrInputContext->GetContextPriority())
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
		// @PR JanSeliv [Coding Standards] - redundant InputContext null-check, outer if already guards it, drop inner if to reduce nesting
		if (InputContext)
		{
			MyPlayerController->BindInputActionsInContext(InputContext);
			UInputUtilsLibrary::SetInputContextEnabled(this, bEnable, InputContext, HighestContextPriority);
		}
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
	// @PR JanSeliv [Coding Standards] - remove commented-out dead code, applies across file
	// const FVector ForwardDirection = FVector().ZeroVector;

	// Get right vector
	const FVector RightDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::Y);
	// const FVector RightDirection = FVector().ZeroVector;;

	APawn* GrsPawn = GetPlayerControllerChecked().GetPawn();
	if (!ensureMsgf(GrsPawn, TEXT("ASSERT: [%i] %hs:\n'GrsPawn' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	GrsPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
	GrsPawn->AddMovementInput(RightDirection, MovementVector.X);
}

// Hold button to increase trajectory on button release trow bomb
void UGrsPlayerControllerComponent::ChargeBomb(const FInputActionValue& ActionValue)
{
	ShowVisualTrajectory();

	// @PR JanSeliv [Coding Standards] - magic 1.0f max charge time, extract to constexpr or DataAsset config value
	if (CurrentHoldTime < 1.0f)
	{
		CurrentHoldTime = CurrentHoldTime + GetWorld()->GetDeltaSeconds();
	}
	else
	{
		if (UGRSDataAsset::Get().ShouldSpawnBombOnMaxChargeTime())
		{
			ThrowProjectile();
		}
		// @PR JanSeliv [Coding Standards] - int 0 assigned to float, use 0.0f
		CurrentHoldTime = 0;
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

	// Aiming area - show visual element in the of predicted end
	UStaticMeshComponent* AimingStaticMeshComponent = GrsPawn->GetAimingSphereComponent();
	if (ensureMsgf(AimingStaticMeshComponent, TEXT("ASSERT: [%i] %hs:\n'AimingStaticMeshComponent' is not present on GrsPawn!"), __LINE__, __FUNCTION__))
	{
		AimingStaticMeshComponent->SetVisibility(true);
		AimingStaticMeshComponent->SetWorldLocation(Result.LastTraceDestination.Location);
	}

	// show trajectory visual
	// @PR JanSeliv [Coding Standards] - Num() > 0, use !Result.PathData.IsEmpty()
	if (UGRSDataAsset::Get().ShouldDisplayTrajectory() && Result.PathData.Num() > 0)
	{
		GrsPawn->ClearTrajectorySplines();
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

	// @PR JanSeliv [Coding Standards] - 1-char loop var, use Index, applies across file
	for (int32 i = 0; i < OutResult.PathData.Num(); i++)
	{
		FVector SplinePoint = OutResult.PathData[i].Location;
		AimingSplineComponent->AddSplinePointAtIndex(SplinePoint, i, ESplineCoordinateSpace::World);
		AimingSplineComponent->Mobility = EComponentMobility::Static;
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

	// @PR JanSeliv [Coding Standards] - GetNumberOfSplinePoints() re-evaluated every iteration, count invariant in loop, cache once to local before loop and reuse
	for (int32 i = 0; i < AimingSplineComponent->GetNumberOfSplinePoints() - 2; i++)
	{
		/* @PR JanSeliv [Architecture] - ChargeBomb runs every aim-held frame, each frame ClearTrajectorySplines destroys then this loop NewObject + RegisterComponent N spline components, defeats PoolManager this GFP depends on.
		 * Keep grow-only pool of spline meshes sized to max points, per frame only update existing (SetStartAndEnd, visibility), set Mobility once outside loop */
		// Create and attach the spline mesh component
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(GrsPawn);
		SplineMesh->AttachToComponent(AimingSplineComponent, FAttachmentTransformRules::KeepRelativeTransform);
		SplineMesh->ForwardAxis = ESplineMeshAxis::Z;
		SplineMesh->Mobility = EComponentMobility::Static;
		// @PR JanSeliv [Coding Standards] - GetTrajectoryMeshScale() retrieved twice for same value, cache to local and reuse for start and end
		// @PR JanSeliv [Coding Standards] - UGRSDataAsset::Get() called repeatedly, cache once to `const UGRSDataAsset& DataAsset = UGRSDataAsset::Get();` and reuse like neighbor components, applies across file
		SplineMesh->SetStartScale(UGRSDataAsset::Get().GetTrajectoryMeshScale());
		SplineMesh->SetEndScale(UGRSDataAsset::Get().GetTrajectoryMeshScale());

		// Set mesh and material
		SplineMesh->SetStaticMesh(UGRSDataAsset::Get().GetChargeMesh());
		SplineMesh->SetMaterial(0, UGRSDataAsset::Get().GetTrajectoryMaterial());
		FVector TangentStart = AimingSplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		FVector TangentEnd = AimingSplineComponent->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

		// Set start and end
		SplineMesh->SetStartAndEnd(OutResult.PathData[i].Location, TangentStart, OutResult.PathData[i + 1].Location, TangentEnd);
		// Register the component so it appears in the game
		SplineMesh->RegisterComponent();

		GrsPawn->AddAimingSplineMeshComponent(SplineMesh);
	}
}

// Configure PredictProjectilePath settings and get result
void UGrsPlayerControllerComponent::PredictProjectilePath(FPredictProjectilePathResult& PredictResult)
{
	// Set launch velocity (forward direction with some upward angle)
	FVector LaunchVelocity = UGRSDataAsset::Get().GetVelocityParams();
	// 45-degree vector between up and right
	// @PR JanSeliv [Coding Standards] - GetCurrentPawnChecked() called 3x in this func, cache once to local ref and reuse
	FVector UpRight45 = (GetCurrentPawnChecked().GetActorForwardVector() + GetCurrentPawnChecked().GetActorUpVector()).GetSafeNormal();

	// Predict and draw the trajectory
	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = GetCurrentPawnChecked().GetActorLocation();

	// --- pick a direction based on the side of the map (left or right)
	// @PR JanSeliv [Coding Standards] - redundant Cast<AActor>, APawn already IS-A AActor, pass &GetCurrentPawnChecked() directly as implicit upcast
	/* @PR JanSeliv [Architecture] - ghost side defined twice, RegisterGhostCharacter stores it by slot and PredictProjectilePath here re-derives from world position via GetCharacterSideFromActor, both can disagree since slot allocation independent of post-spawn X.
	 * Store allocated side on AGrsPawn (replicate enum), aiming reads stored side, drop positional re-derivation and unused EGRSSpotType enum */
	const float SideSign = UGrsUtils::GetCharacterSideFromActor(Cast<AActor>(&GetCurrentPawnChecked())) == EGRSCharacterSide::Left ? 1.0f : -1.0f;

	Params.LaunchVelocity = FVector(UpRight45.X + SideSign * (LaunchVelocity.X * CurrentHoldTime), LaunchVelocity.Y, UpRight45.Z + LaunchVelocity.Z);
	Params.ActorsToIgnore.Add(GetCurrentPawn());

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

	// @PR JanSeliv [Coding Standards] - int literals for float math, use .f (5.f, 100.f), applies across file
	FVector ThrowDirection = GrsPawn->GetActorForwardVector() + FVector(5, 5, 0.0f);
	ThrowDirection.Normalize();
	// @PR JanSeliv [Coding Standards] - LaunchVelocity never read, dead local, remove it and ThrowDirection compute that only feeds it
	FVector LaunchVelocity = ThrowDirection * 100;

	// @PR JanSeliv [Potential Bug] - CurrentHoldTimeInternal not reset on release-throw path, only ChargeBomb max-charge branch resets, charges silently leak across throws, reset it here
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
