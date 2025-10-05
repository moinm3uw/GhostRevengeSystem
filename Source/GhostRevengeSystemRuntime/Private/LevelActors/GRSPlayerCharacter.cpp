// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GRSPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GRSGhostCharacterManagerComponent.h"
#include "Components/MapComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Controllers/MyPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/BombDataAsset.h"
#include "DataAssets/PlayerDataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "GameplayAbilitySpec.h"
#include "GeneratedMap.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "LevelActors/BombActor.h"
#include "LevelActors/GRSBombProjectile.h"
#include "LevelActors/PlayerCharacter.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "TimerManager.h"
#include "UI/Widgets/PlayerNameWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

class UPlayerRow;

/*********************************************************************************************
 * Mesh and Initialization
 **********************************************************************************************/

// Sets default values for this character's properties
AGRSPlayerCharacter::AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UMySkeletalMeshComponent>(MeshComponentName)) // Init UMySkeletalMeshComponent instead of USkeletalMeshComponent
{
	// Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
	SetDefaultParams();

	// Initialize skeletal mesh of the character
	InitializeSkeletalMesh();

	// Configure the movement component
	MovementComponentConfiguration();

	// Setup capsule component
	SetupCapsuleComponent();

	// setup spline component
	ProjectileSplineComponentInternal = CreateDefaultSubobject<USplineComponent>(TEXT("ProjectileSplineComponent"));
	ProjectileSplineComponentInternal->AttachToComponent(MeshComponentInternal, FAttachmentTransformRules::KeepRelativeTransform);

	AimingSphereComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereComp"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		AimingSphereComponent->SetStaticMesh(SphereMesh.Object);
	}

	// Initialize 3D widget component for the player name
	PlayerName3DWidgetComponentInternal = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponentInternal->SetupAttachment(RootComponent);

	ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::OnControllerChanged);
}

// Get the character side
EGRSCharacterSide AGRSPlayerCharacter::GetCharacterSide() const
{
	const FCell ArenaCenter = UCellsUtilsLibrary::GetCenterCellOnLevel();
	const FVector ToArenaDirection = ArenaCenter.Location - GetActorLocation().GetSafeNormal();
	return ToArenaDirection.X > 0 ? EGRSCharacterSide::Left : EGRSCharacterSide::Right;
}

// Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
void AGRSPlayerCharacter::SetDefaultParams()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Replicate an acto
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	// Do not rotate player by camera
	bUseControllerRotationYaw = false;
}

// Initialize skeletal mesh of the character
void AGRSPlayerCharacter::InitializeSkeletalMesh()
{
	// Initialize skeletal mesh
	USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
	checkf(SkeletalMeshComponent, TEXT("ERROR: [%i] %hs:\n'SkeletalMeshComponent' is null!"), __LINE__, __FUNCTION__);
	static const FVector MeshRelativeLocation(0, 0, -90.f);
	SkeletalMeshComponent->SetRelativeLocation_Direct(MeshRelativeLocation);
	static const FRotator MeshRelativeRotation(0, -90.f, 0);
	SkeletalMeshComponent->SetRelativeRotation_Direct(MeshRelativeRotation);
	SkeletalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Enable all lighting channels, so it's clearly visible in the dark
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/ true, /*bChannel1*/ true, /*bChannel2*/ true);
}

// Configure the movement component of the character
void AGRSPlayerCharacter::MovementComponentConfiguration()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Rotate player by movement
		MovementComponent->bOrientRotationToMovement = true;
		static const FRotator RotationRate(0.f, 540.f, 0.f);
		MovementComponent->RotationRate = RotationRate;

		// Do not push out clients from collision
		MovementComponent->MaxDepenetrationWithGeometryAsProxy = 0.f;
	}
}

// Set up the capsule component of the character
void AGRSPlayerCharacter::SetupCapsuleComponent()
{
	if (UCapsuleComponent* RootCapsuleComponent = GetCapsuleComponent())
	{
		// Setup collision to allow overlap players with each other, but block all other actors
		RootCapsuleComponent->CanCharacterStepUpOn = ECB_Yes;
		RootCapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		RootCapsuleComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player0, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player1, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player2, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player3, ECR_Overlap);

		RootCapsuleComponent->SetIsReplicated(true);
	}
}

// Returns the Skeletal Mesh of ghost revenge character
UMySkeletalMeshComponent& AGRSPlayerCharacter::GetMeshChecked() const
{
	return *CastChecked<UMySkeletalMeshComponent>(GetMesh());
}

// Set visibility of the player character
void AGRSPlayerCharacter::SetVisibility(bool Visibility)
{
	GetMesh()->SetVisibility(Visibility, true);
}

// Clean up the character
void AGRSPlayerCharacter::PerformCleanUp()
{
	ClearTrajectorySplines();

	if (Controller)
	{
		AMyPlayerController& PC = *UMyBlueprintFunctionLibrary::GetLocalPlayerController();

		// Remove all previous input contexts managed by Controller
		TArray<const UMyInputMappingContext*> InputContexts;
		UMyInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
		InputContexts.AddUnique(InputContext);
		PC.RemoveInputContexts(InputContexts);

		Controller->Possess(UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter());
	}

	if (UMovementComponent* MovementComponent = GetMovementComponent())
	{
		MovementComponent->DestroyComponent();
	}

	if (UCapsuleComponent* RootCapsuleComponent = GetCapsuleComponent())
	{
		RootCapsuleComponent->DestroyComponent();
	}

	if (MeshComponentInternal)
	{
		MeshComponentInternal->DestroyComponent();
	}

	if (ProjectileSplineComponentInternal)
	{
		ProjectileSplineComponentInternal->DestroyComponent();
	}
	if (BombProjectileInternal)
	{
		BombProjectileInternal->Destroy();
	}

	if (AimingSphereComponent)
	{
		AimingSphereComponent->DestroyComponent();
	}
}

// Returns own character ID, e.g: 0, 1, 2, 3
int32 AGRSPlayerCharacter::GetPlayerId() const
{
	if (const AMyPlayerState* MyPlayerState = GetPlayerState<AMyPlayerState>())
	{
		return MyPlayerState->GetPlayerId();
	}

	return 0;
}

//  Possess a player controller
void AGRSPlayerCharacter::TryPossessController(AController* PlayerController)
{
	if (!PlayerController || !PlayerController->HasAuthority())
	{
		return;
	}

	PlayerController->ResetIgnoreMoveInput();

	if (PlayerController)
	{
		// Unpossess current pawn first
		if (PlayerController->GetPawn())
		{
			PlayerController->UnPossess();
		}
	}

	PlayerController->Possess(this);

	RegisterForPlayerDeath();

	// --- enable inputs for ghost character
	SetManagedInputContextEnabled(true);
}

// Called when a controller has been replicated to the client
void AGRSPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	// --- enable inputs for ghost character
	SetManagedInputContextEnabled(true);
}

// Called when our Controller no longer possesses us. Only called on the server (or in standalone).
void AGRSPlayerCharacter::UnPossessed()
{
	Super::UnPossessed();

	//--- disable inputs for ghost character once no more controller available
	SetManagedInputContextEnabled(false);

	UE_LOG(LogTemp, Warning, TEXT("GRSPlayerCharacter::UnPossessed happened"));
}

// Event called after a pawn's controller has changed, on the server and owning client. This will happen at the same time as the delegate on GameInstance
void AGRSPlayerCharacter::OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (OldController && OldController->HasAuthority() || NewController && NewController->HasAuthority())
	{
		return;
	}

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(OldController);
	if (!PlayerController)
	{
		return;
	}

	TArray<const UMyInputMappingContext*> InputContexts;
	UMyInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
	InputContexts.AddUnique(InputContext);

	// --- Remove related input contexts
	PlayerController->RemoveInputContexts(InputContexts);
}

// Enables or disables the input context
void AGRSPlayerCharacter::SetManagedInputContextEnabled(bool bEnable)
{
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	TArray<const UMyInputMappingContext*> InputContexts;
	UMyInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
	InputContexts.AddUnique(InputContext);

	if (!bEnable)
	{
		// --- Remove related input contexts
		PlayerController->RemoveInputContexts(InputContexts);
		return;
	}

	// --- Remove all previous input contexts managed by Controller
	PlayerController->RemoveInputContexts(InputContexts);

	// --- Add gameplay context as auto managed by Game State, so it will be enabled everytime the game is in the in-game state
	if (InputContext
	    && InputContext->GetChosenGameStatesBitmask() > 0)
	{
		PlayerController->SetupInputContexts(InputContexts);
	}
}

// Subscribes to PlayerCharacters death events in order to see if a player died
void AGRSPlayerCharacter::RegisterForPlayerDeath()
{
	TArray<AActor*> PlayerCharacters;

	// -- subscribe to PlayerCharacters death event in order to see if a ghost player killed somebody
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), PlayerCharacters);

	for (AActor* Actor : PlayerCharacters)
	{
		APlayerCharacter* MyActor = Cast<APlayerCharacter>(Actor);
		if (MyActor)
		{
			if (MyActor->IsBotControlled())
			{
				continue;
			}

			UMapComponent* MapComponent = UMapComponent::GetMapComponent(MyActor);
			if (MapComponent)
			{
				MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);
			}
		}
	}
}

/*********************************************************************************************
 * Begin play (on spawned on the level)
 **********************************************************************************************/

// Called when the game starts or when spawned
void AGRSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("BeginPlay Spawned ghost character  --- %s - %s"), *this->GetName(), this->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem::Get().RegisterGhostCharacter(this);

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	// --- Initialize aiming point
	AimingSphereComponent->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	AimingSphereComponent->SetVisibility(true);

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Perform ghost character activation (possessing controller)
void AGRSPlayerCharacter::ActivateCharacter(const APlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter)
	{
		return;
	}

	AController* PlayerController = PlayerCharacter->GetController();
	TryPossessController(PlayerController);
	InitializePlayerName(PlayerCharacter);
}

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGRSPlayerCharacter::GetAbilitySystemComponent() const
{
	const AMyPlayerState* InPlayerState = GetPlayerState<AMyPlayerState>();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

// Listen game states to remove ghost character from level
void AGRSPlayerCharacter::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
		case ECurrentGameState::GameStarting:
		{
			// --- default params required for the fist start to have character prepared
			SetDefaultPlayerMeshData();
			// --- Init Player name
			// InitializePlayerName(UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter());
			// --- Init character visuals (animations, skin)
			InitializeCharacterVisual(UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter());
			// --- Clear splines
			ClearTrajectorySplines();
			break;
		}

		case ECurrentGameState::Menu:
		{
			RemoveGhostCharacterFromMap();
			break;
		}

		default: break;
	}
}

// Set and apply default skeletal mesh for this player once game is starting therefore all players are connected
void AGRSPlayerCharacter::SetDefaultPlayerMeshData(bool bForcePlayerSkin /* = false*/)
{
	APlayerCharacter* PlayerCharacter = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter();
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	const ELevelType PlayerFlag = UMyBlueprintFunctionLibrary::GetLevelType();
	ELevelType LevelType = PlayerFlag;

	const UPlayerRow* Row = UPlayerDataAsset::Get().GetRowByLevelType<UPlayerRow>(TO_ENUM(ELevelType, LevelType));
	if (!ensureMsgf(Row, TEXT("ASSERT: [%i] %hs:\n'Row' is not found!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const int32 SkinsNum = Row->GetSkinTexturesNum();
	FBmrMeshData MeshData;
	MeshData.Row = Row;
	MeshData.SkinIndex = PlayerCharacter->GetPlayerId() % SkinsNum;

	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(&GetMeshChecked()))
	{
		SkeletalMeshComponent->SetSkeletalMesh(Cast<USkeletalMesh>(MeshData.Row->Mesh));
	}
	else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(&GetMeshChecked()))
	{
		StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(MeshData.Row->Mesh));
	}
}

//  Perform init character once  from a refence character (visuals, animations)
void AGRSPlayerCharacter::InitializeCharacterVisual(const APlayerCharacter* PlayerCharacter)
{
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	const UMySkeletalMeshComponent* MainCharacterMeshComponent = &PlayerCharacter->GetMeshComponentChecked();

	if (!ensureMsgf(MainCharacterMeshComponent, TEXT("ASSERT: [%i] %hs:\n'MainCharacterMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	const int32 CurrentSkinIndex = MainCharacterMeshComponent->GetAppliedSkinIndex();

	UMySkeletalMeshComponent* CurrentMeshComponent = &GetMeshChecked();
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitMySkeletalMesh(MainCharacterMeshComponent->GetMeshData());
	CurrentMeshComponent->ApplySkinByIndex(CurrentSkinIndex);
}

//  Initialize Player Name
void AGRSPlayerCharacter::InitializePlayerName(const APlayerCharacter* MainCharacter) const
{
	if (!MainCharacter)
	{
		return;
	}
	AMyPlayerState* MyPlayerState = Cast<AMyPlayerState>(MainCharacter->GetPlayerState());
	if (!MyPlayerState)
	{
		return;
	}

	checkf(PlayerName3DWidgetComponentInternal, TEXT("ERROR: [%i] %hs:\n'PlayerName3DWidgetComponentInternal' is null!"), __LINE__, __FUNCTION__);
	PlayerName3DWidgetComponentInternal->Init(MyPlayerState);

	UEnum* EnumPtr = StaticEnum<EGRSCharacterSide>();
	const FString SideName = EnumPtr->GetNameStringByValue(TO_FLAG(GetCharacterSide()));

	UUserWidget* Widget = PlayerName3DWidgetComponentInternal->GetWidget();
	UPlayerNameWidget* NickName = Cast<UPlayerNameWidget>(Widget);
	if (!ensureMsgf(NickName, TEXT("ASSERT: [%i] %hs:\n'NickName' condition is FALSE"), __LINE__, __FUNCTION__))
	{
		return;
	}

	NickName->SetPlayerName(FText::FromString(SideName));
}

// Remove ghost character from the level
void AGRSPlayerCharacter::RemoveGhostCharacterFromMap()
{
	AimingSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimingSphereComponent->SetVisibility(false);

	// --- not ghost character controlled, could be null
	if (GetController() && GetController()->HasAuthority())
	{
		GetController()->UnPossess();
	}
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients.
void AGRSPlayerCharacter::OnPreRemovedFromLevel_Implementation(class UMapComponent* MapComponent, class UObject* DestroyCauser)
{
	APlayerCharacter* PlayerCharacter = MapComponent->GetOwner<APlayerCharacter>();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const APawn* Causer = Cast<APawn>(DestroyCauser);
	if (Causer)
	{
		const AActor* CauserActor = Cast<AActor>(DestroyCauser);
		Causer = CauserActor ? CauserActor->GetInstigator() : nullptr;
		const AGRSPlayerCharacter* GRSCharacter = Cast<AGRSPlayerCharacter>(Causer);
		if (GRSCharacter == this)
		{
			UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- Spawner is an instigator"), __LINE__, __FUNCTION__);
			OnGhostEliminatesPlayer.Broadcast(PlayerCharacter, this);
			RemoveGhostCharacterFromMap();
		}
	}
}

/*********************************************************************************************
 * Hold To Charge aim, spawn bomb (enhanced input)
 **********************************************************************************************/

// Move the player character
void AGRSPlayerCharacter::MovePlayer(const FInputActionValue& ActionValue)
{
	const AController* OwnedController = GetController();
	if (!OwnedController
	    || OwnedController->IsMoveInputIgnored())
	{
		return;
	}

	// input is a Vector2D
	const FVector2D MovementVector = ActionValue.Get<FVector2D>();

	// Find out which way is forward
	const FRotator ForwardRotation = UCellsUtilsLibrary::GetLevelGridRotation();

	// Get forward vector
	const FVector ForwardDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::X);
	// const FVector ForwardDirection = FVector().ZeroVector;

	// Get right vector
	const FVector RightDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::Y);
	// const FVector RightDirection = FVector().ZeroVector;;

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

// Hold button to increase trajectory on button release trow bomb
void AGRSPlayerCharacter::ChargeBomb(const FInputActionValue& ActionValue)
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
			ThrowProjectile();
		}
		CurrentHoldTimeInternal = 0;
	}

	UE_LOG(LogTemp, Warning, TEXT("GRS: Current hold time value: %f"), CurrentHoldTimeInternal);
}

//  Add and update visual representation of charging (aiming) progress as trajectory
void AGRSPlayerCharacter::ShowVisualTrajectory()
{
	FPredictProjectilePathResult Result;

	// Configure PredictProjectilePath settings and get result
	PredictProjectilePath(Result);

	// Aiming area - show visual element in the of predicted end
	AddMeshToEndProjectilePath(Result);

	// show trajectory visual
	if (UGRSDataAsset::Get().ShouldDisplayTrajectory() && Result.PathData.Num() > 0)
	{
		AddSplinePoints(Result);
		AddSplineMesh(Result);
	}
}

// Configure PredictProjectilePath settings and get result
void AGRSPlayerCharacter::PredictProjectilePath(FPredictProjectilePathResult& PredictResult)
{
	// Set launch velocity (forward direction with some upward angle)
	FVector LaunchVelocity = UGRSDataAsset::Get().GetVelocityParams();
	// 45-degree vector between up and right
	FVector UpRight45 = (GetActorForwardVector() + GetActorUpVector()).GetSafeNormal();

	// Predict and draw the trajectory
	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = GetActorLocation();

	// --- pick a direction based on the side of the map (left or right)
	const float SideSign = GetCharacterSide() == EGRSCharacterSide::Left ? 1.0f : -1.0f;

	Params.LaunchVelocity = FVector(
	    UpRight45.X + SideSign * (LaunchVelocity.X * CurrentHoldTimeInternal),
	    LaunchVelocity.Y,
	    UpRight45.Z + LaunchVelocity.Z);

	Params.ActorsToIgnore.Add(this);

	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, PredictResult);
}

// Add a mesh to the last element of the predict Projectile path results
void AGRSPlayerCharacter::AddMeshToEndProjectilePath(FPredictProjectilePathResult& Result)
{
	AimingSphereComponent->SetVisibility(true);
	AimingSphereComponent->SetWorldLocation(Result.LastTraceDestination.Location);
}

// Add spline points to the spline component
void AGRSPlayerCharacter::AddSplinePoints(FPredictProjectilePathResult& Result)
{
	ClearTrajectorySplines();
	for (int32 i = 0; i < Result.PathData.Num(); i++)
	{
		FVector SplinePoint = Result.PathData[i].Location;
		ProjectileSplineComponentInternal->AddSplinePointAtIndex(SplinePoint, i, ESplineCoordinateSpace::World);
		ProjectileSplineComponentInternal->Mobility = EComponentMobility::Static;
	}

	ProjectileSplineComponentInternal->SetSplinePointType(Result.PathData.Num() - 1, ESplinePointType::CurveClamped, true);
	ProjectileSplineComponentInternal->UpdateSpline();
}

// Add spline mesh to spline points
void AGRSPlayerCharacter::AddSplineMesh(FPredictProjectilePathResult& Result)
{
	for (int32 i = 0; i < ProjectileSplineComponentInternal->GetNumberOfSplinePoints() - 2; i++)
	{
		// Create and attach the spline mesh component
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this); // 'this' is usually your actor
		SplineMesh->AttachToComponent(ProjectileSplineComponentInternal, FAttachmentTransformRules::KeepRelativeTransform);
		SplineMesh->ForwardAxis = ESplineMeshAxis::Z;
		SplineMesh->Mobility = EComponentMobility::Static;
		SplineMesh->SetStartScale(UGRSDataAsset::Get().GetTrajectoryMeshScale());
		SplineMesh->SetEndScale(UGRSDataAsset::Get().GetTrajectoryMeshScale());

		// Set mesh and material
		SplineMesh->SetStaticMesh(UGRSDataAsset::Get().GetChargeMesh());
		SplineMesh->SetMaterial(0, UGRSDataAsset::Get().GetTrajectoryMaterial());
		FVector TangentStart = ProjectileSplineComponentInternal->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		FVector TangentEnd = ProjectileSplineComponentInternal->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

		// Set start and end
		SplineMesh->SetStartAndEnd(Result.PathData[i].Location, TangentStart, Result.PathData[i + 1].Location, TangentEnd);
		// Register the component so it appears in the game
		SplineMesh->RegisterComponent();

		SplineMeshArrayInternal.AddUnique(SplineMesh);
	}
}

// Spawn and send projectile
void AGRSPlayerCharacter::ThrowProjectile()
{
	//--- Calculate Cell to spawn bomb
	FCell CurrentCell;
	CurrentCell.Location = AimingSphereComponent->GetComponentLocation();

	//--- hide aiming sphere from ui
	AimingSphereComponent->SetVisibility(false);

	//--- spawn bomb on the server
	SpawnBomb(CurrentCell);

	FVector ThrowDirection = GetActorForwardVector() + FVector(5, 5, 0.0f);
	ThrowDirection.Normalize();
	FVector LaunchVelocity = ThrowDirection * 100;

	ClearTrajectorySplines();

	// Spawn projectile and launch projectile
	if (BombProjectileInternal == nullptr)
	{
		// BombProjectileInternal = GetWorld()->SpawnActor<AGRSBombProjectile>(UGRSDataAsset::Get().GetProjectileClass(), SpawnLocation, GetActorRotation());
	}

	if (BombProjectileInternal)
	{
		// BombProjectileInternal->Launch(LaunchVelocity);
	}
}

// Spawn bomb on aiming sphere position
void AGRSPlayerCharacter::SpawnBomb(FCell TargetCell)
{
	const FCell& SpawnBombCell = UCellsUtilsLibrary::GetNearestFreeCell(TargetCell);

	// Activate bomb ability
	FGameplayEventData EventData;
	EventData.Instigator = this;
	EventData.EventMagnitude = UCellsUtilsLibrary::GetIndexByCellLevel(SpawnBombCell);
	GetAbilitySystemComponent()->HandleGameplayEvent(BmrGameplayTags::Event::Bomb_Placed, &EventData);
}

// Hide spline elements (trajectory)
void AGRSPlayerCharacter::ClearTrajectorySplines()
{
	for (USplineMeshComponent* SplineMeshComponent : SplineMeshArrayInternal)
	{
		SplineMeshComponent->DestroyComponent();
	}

	SplineMeshArrayInternal.Empty();
	ProjectileSplineComponentInternal->ClearSplinePoints();
}
