// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "LevelActors/GRSPlayerCharacter.h"

#include "GeneratedMap.h"
#include "GhostRevengeSystemComponent.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "Components/MapComponent.h"
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
#include "GameFramework/MyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "LevelActors/BombActor.h"
#include "LevelActors/GRSBombProjectile.h"
#include "LevelActors/PlayerCharacter.h"
#include "SubSystems/GRSWorldSubSystem.h"
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

	SphereComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereComp"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		SphereComp->SetStaticMesh(SphereMesh.Object);
	}

	// Initialize 3D widget component for the player name
	PlayerName3DWidgetComponentInternal = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponentInternal->SetupAttachment(RootComponent);
}

// Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
void AGRSPlayerCharacter::SetDefaultParams()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Replicate an actor
	bReplicates = true;
	SetReplicates(true);
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
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/true, /*bChannel1*/true, /*bChannel2*/true);
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

	if (SphereComp)
	{
		SphereComp->DestroyComponent();
	}
}

// Returns own character ID, e.g: 0, 1, 2, 3
int32 AGRSPlayerCharacter::GetPlayerId() const
{
	if (const AMyPlayerState* MyPlayerState = UMyBlueprintFunctionLibrary::GetLocalPlayerState())
	{
		return MyPlayerState->GetPlayerId();
	}

	return 0;
}

//  Possess a player controller
void AGRSPlayerCharacter::TryPossessController(APlayerController* PlayerController)
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
}

// UnPossess a player controller
void AGRSPlayerCharacter::UnPossessController()
{
	// --- Posses to play character
	AController* GhostPlayerController = UGRSWorldSubSystem::Get().GetGhostPlayerCharacter()->Controller;
	APlayerCharacter* MainPlayerCharacter = UGRSWorldSubSystem::Get().GetMainPlayerCharacter();
	if (!GhostPlayerController || !GhostPlayerController->HasAuthority() || !MainPlayerCharacter)
	{
		return;
	}

	GhostPlayerController->Possess(MainPlayerCharacter);
}

// Called when a controller has been replicated to the client 
void AGRSPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	SetManagedInputContextEnabled(true);
	UE_LOG(LogTemp, Warning, TEXT("GRSPlayerCharacter::OnRep_Controller. "));
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

/*********************************************************************************************
 * Begin play (on spawned on the level)
 **********************************************************************************************/

// Called when the game starts or when spawned
void AGRSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SetDefaultPlayerMeshData();
}

// Set and apply default skeletal mesh for this player
void AGRSPlayerCharacter::SetDefaultPlayerMeshData(bool bForcePlayerSkin/* = false*/)
{
	APlayerCharacter* PlayerCharacter = UGRSWorldSubSystem::Get().GetMainPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}
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

//  Perform init character once added to the level
void AGRSPlayerCharacter::Initialize()
{
	APlayerCharacter* MainCharacter = UGRSWorldSubSystem::Get().GetMainPlayerCharacter();
	if (!MainCharacter)
	{
		return;
	}


	APlayerController* MainPlayerController = Cast<APlayerController>(MainCharacter->Controller);
	if (!MainPlayerController)
	{
		return;
	}
	// -- Set actor spawn location 
	SetLocation(MainCharacter);

	// --- Init Player name
	InitializePlayerName(MainCharacter);

	// --- Set the animation blueprint on very first character spawn
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	UMySkeletalMeshComponent* MainCharacterMeshComponent = &MainCharacter->GetMeshChecked();

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
	//CurrentMeshComponent.ApplySkinByIndex(CurrentSkinIndex);

	// --- Clear splines if spawned
	ClearTrajectorySplines();

	// --- Initialize aiming point
	SphereComp->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	SphereComp->SetVisibility(true);

	// -- subscribe to PlayerCharacters death event in order to see if a ghost player killed somebody 
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), PlayerCharactersInternal);

	for (AActor* Actor : PlayerCharactersInternal)
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

	// --- Possess the ghost character
	TryPossessController(MainPlayerController);
}

// Set initial location on spawn
void AGRSPlayerCharacter::SetLocation(const APlayerCharacter* MainCharacter)
{
	if (!MainCharacter)
	{
		return;
	}

	FCell ActorSpawnLocation;
	float CellSize = FCell::CellSize + (FCell::CellSize / 2);

	int32 PlayerId = MainCharacter->GetPlayerId();

	ActorSpawnLocation.Location = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation();

	if (PlayerId == 0 || PlayerId == 2)
	{
		//ActorSpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopLeft);
		//ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.X = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().X - CellSize;
	}
	else
	{
		//ActorSpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopRight);
		//ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.X = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().X + CellSize;
	}

	// --- Update ghost character 
	SetActorLocation(ActorSpawnLocation);
	SetVisibility(true);
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
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients.
void AGRSPlayerCharacter::OnPreRemovedFromLevel_Implementation(class UMapComponent* MapComponent, class UObject* DestroyCauser)
{
	if (BombActorInternal == DestroyCauser)
	{
		FVector SpawnLocation = MapComponent->GetOwner()->GetActorLocation();
		//this->SetActorLocation(SpawnLocation);

		FCell CurrentCell = SpawnLocation;
		// const FCell SnappedCell = UCellsUtilsLibrary::GetNearestFreeCell(CurrentCell);
		if (!Controller)
		{
			return;
		}
		Controller->UnPossess();
		this->SetVisibility(false);
		AGeneratedMap::Get().SpawnActorWithMesh(EActorType::Player, CurrentCell, MapComponent->GetReplicatedMeshData());
		OnGhostPlayerKilled.Broadcast();
		FString Name = MapComponent->GetOwner()->GetName();
		UE_LOG(LogTemp, Warning, TEXT("AGRSPlayer character bomb destroyed actor %s"), *Name);
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
	//const FVector ForwardDirection = FVector().ZeroVector;

	// Get right vector
	const FVector RightDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::Y);
	//const FVector RightDirection = FVector().ZeroVector;;

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

// Hold button to increase trajectory on button release trow bomb
void AGRSPlayerCharacter::ChargeBomb(const FInputActionValue& ActionValue)
{
	if (BombCountInternal < 1)
	{
		return;
	}
	ShowVisualTrajectory();

	if (CurrentHoldTimeInternal < 1.0f)
	{
		CurrentHoldTimeInternal = CurrentHoldTimeInternal + GetWorld()->GetDeltaSeconds();
	}
	else
	{
		if (UGRSDataAsset::Get().ShouldSpawnBombOnMaxChargeTime())
		{
			ThrowProjectile(ActionValue);
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
	Params.LaunchVelocity = FVector(UpRight45.X + LaunchVelocity.X * CurrentHoldTimeInternal, 0 + LaunchVelocity.Y, UpRight45.Z + LaunchVelocity.Z);
	Params.ActorsToIgnore.Add(this);

	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, PredictResult);
}

// Add a mesh to the last element of the predict Projectile path results
void AGRSPlayerCharacter::AddMeshToEndProjectilePath(FPredictProjectilePathResult& Result)
{
	SphereComp->SetVisibility(true);
	SphereComp->SetWorldLocation(Result.LastTraceDestination.Location);
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
void AGRSPlayerCharacter::ThrowProjectile(const FInputActionValue& ActionValue)
{
	if (BombCountInternal < 1)
	{
		return;
	}

	// Calculate Cell to spawn bomb:
	FVector SpawnLocation = SphereComp->GetComponentLocation();
	FCell CurrentCell;
	CurrentCell.Location = SpawnLocation;
	SphereComp->SetVisibility(false);
	const FCell& SpawnBombCell = UCellsUtilsLibrary::GetNearestFreeCell(CurrentCell);
	//UGhostRevengeSystemSpotComponent* Spot = UGRSWorldSubSystem::Get().GetSpotComponent();

	const TFunction<void(UMapComponent&)> OnBombSpawned = [WeakThis = TWeakObjectPtr(this), this](UMapComponent& MapComponent)
	{
		AGRSPlayerCharacter* PlayerCharacter = WeakThis.Get();
		if (!PlayerCharacter)
		{
			return;
		}

		ACharacter* Character = Cast<ACharacter>(PlayerCharacter);

		// Init Bomb
		BombActorInternal = CastChecked<ABombActor>(MapComponent.GetOwner());

		// Start listening this bomb
		MapComponent.OnPostRemovedFromLevel.AddUniqueDynamic(PlayerCharacter, &ThisClass::OnBombDestroyed);
	};

	// Spawn bomb
	//const FBmrMeshData& PlayerMeshData = Spot->GetMeshChecked().GetMeshData();
	const struct FBmrMeshData& MeshData = FBmrMeshData(UBombDataAsset::Get().GetRowByIndex(0));
	const auto& OnSpawned = [MeshData](UMapComponent& MapComponent) { MapComponent.SetReplicatedMeshData(MeshData); };
	AGeneratedMap::Get().SpawnActorByType(EAT::Bomb, SpawnBombCell, OnBombSpawned);

	// empty bomb count to spawn
	BombCountInternal = 0;

	FVector ThrowDirection = GetActorForwardVector() + FVector(5, 5, 0.0f);
	ThrowDirection.Normalize();
	FVector LaunchVelocity = ThrowDirection * 100;

	ClearTrajectorySplines();

	// Spawn projectile and launch projectile
	if (BombProjectileInternal == nullptr)
	{
		//BombProjectileInternal = GetWorld()->SpawnActor<AGRSBombProjectile>(UGRSDataAsset::Get().GetProjectileClass(), SpawnLocation, GetActorRotation());
	}

	if (BombProjectileInternal)
	{
		// BombProjectileInternal->Launch(LaunchVelocity);
	}
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

// Event triggered when the bomb has been explicitly destroyed.
void AGRSPlayerCharacter::OnBombDestroyed_Implementation(class UMapComponent* MapComponent, UObject* DestroyCauser)
{
	if (!MapComponent
		|| MapComponent->GetActorType() != EAT::Bomb
		|| BombCountInternal >= 1)
	{
		return;
	}
	// reset count of bombs available
	BombCountInternal = 1;
}
