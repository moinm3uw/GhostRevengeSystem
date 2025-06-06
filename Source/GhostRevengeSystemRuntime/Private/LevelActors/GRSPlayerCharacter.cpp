// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "LevelActors/GRSPlayerCharacter.h"

#include "InputActionValue.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Controllers/MyPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/PlayerDataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/MyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "LevelActors/GRSBombProjectile.h"
#include "LevelActors/PlayerCharacter.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UI/Widgets/PlayerNameWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

class UPlayerRow;

// Sets default values for this character's properties
AGRSPlayerCharacter::AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMySkeletalMeshComponent>(MeshComponentName)) // Init UMySkeletalMeshComponent instead of USkeletalMeshComponent
{
	// Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
	SetDefaultParams();

	// Initialize skeletal mesh of the character
	InitializeSkeletalMesh();

	// Initialize the nameplate mesh component
	InitializeNameplateMeshComponent();

	// set a name plate material
	InitializeNamePlateMaterial();

	// Initialize 3D widget component for the player name
	Initialize3DWidgetComponent();

	// Configure the movement component
	MovementComponentConfiguration();

	// Setup capsule component
	SetupCapsuleComponent();
}

// Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
void AGRSPlayerCharacter::SetDefaultParams()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Replicate an actor
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
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/true, /*bChannel1*/true, /*bChannel2*/true);
}

// Initialize the nameplate mesh component (background material of the player name)
void AGRSPlayerCharacter::InitializeNameplateMeshComponent()
{
	NameplateMeshInternal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GRSNameplateMeshComponent"));
	NameplateMeshInternal->SetupAttachment(RootComponent);
	static const FVector NameplateRelativeLocation(0.f, 0.f, 210.f);
	NameplateMeshInternal->SetRelativeLocation_Direct(NameplateRelativeLocation);
	static const FVector NameplateRelativeScale(2.25f, 1.f, 1.f);
	NameplateMeshInternal->SetRelativeScale3D_Direct(NameplateRelativeScale);
	NameplateMeshInternal->SetUsingAbsoluteRotation(true);
	NameplateMeshInternal->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	checkf(PlaneMesh, TEXT("ERROR: [%i] %hs:\n'PlaneMesh' failed to load!"), __LINE__, __FUNCTION__);
	NameplateMeshInternal->SetStaticMesh(PlaneMesh);
	NameplateMeshInternal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Setup name plate material
void AGRSPlayerCharacter::InitializeNamePlateMaterial()
{
	// Set a nameplate material
	if (ensureMsgf(NameplateMeshInternal, TEXT("ASSERT: 'NameplateMeshComponent' is not valid")))
	{
		const UPlayerDataAsset& PlayerDataAsset = UPlayerDataAsset::Get();
		const int32 NameplateMeshesNum = PlayerDataAsset.GetNameplateMaterialsNum();
		if (NameplateMeshesNum > 0)
		{
			int32 CurrentPlayerId = GetPlayerId();
			const int32 MaterialNo = CurrentPlayerId < NameplateMeshesNum ? CurrentPlayerId : CurrentPlayerId % NameplateMeshesNum;
			if (UMaterialInterface* Material = PlayerDataAsset.GetNameplateMaterial(MaterialNo))
			{
				NameplateMeshInternal->SetMaterial(0, Material);
			}
		}
	}
}

// Initialize 3D widget component for the player name
void AGRSPlayerCharacter::Initialize3DWidgetComponent()
{
	PlayerName3DWidgetComponentInternal = CreateDefaultSubobject<UWidgetComponent>(TEXT("GRSPlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponentInternal->SetupAttachment(NameplateMeshInternal);
	static const FVector WidgetRelativeLocation(0.f, 0.f, 10.f);
	PlayerName3DWidgetComponentInternal->SetRelativeLocation_Direct(WidgetRelativeLocation);
	static const FRotator WidgetRelativeRotation(90.f, -90.f, 180.f);
	PlayerName3DWidgetComponentInternal->SetRelativeRotation_Direct(WidgetRelativeRotation);
	PlayerName3DWidgetComponentInternal->SetGenerateOverlapEvents(false);
	static const FVector2D DrawSize(180.f, 50.f);
	PlayerName3DWidgetComponentInternal->SetDrawSize(DrawSize);
	static const FVector2D Pivot(0.5f, 0.4f);
	PlayerName3DWidgetComponentInternal->SetPivot(Pivot);
	PlayerName3DWidgetComponentInternal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGRSPlayerCharacter::InitPlayerNickName()
{
	if (AMyPlayerState* MyPlayerState = UMyBlueprintFunctionLibrary::GetLocalPlayerState())
	{
		MyPlayerState->OnPlayerNameChanged.AddUniqueDynamic(this, &ThisClass::SetNicknameOnNameplate);
		SetNicknameOnNameplate(*MyPlayerState->GetPlayerName());
	}
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

// Update player name on a 3D widget component
void AGRSPlayerCharacter::SetNicknameOnNameplate_Implementation(FName NewName)
{
	const UWidgetsSubsystem* WidgetsSubsystem = UWidgetsSubsystem::GetWidgetsSubsystem();
	UPlayerNameWidget* PlayerNameWidget = WidgetsSubsystem ? WidgetsSubsystem->GetNicknameWidget(GetPlayerId()) : nullptr;
	if (!PlayerNameWidget)
	{
		// Widget is not created yet, might be called before UI Subsystem is initialized
		return;
	}

	PlayerNameWidget->SetPlayerName(FText::FromName(NewName));
	PlayerNameWidget->SetAssociatedPlayerId(GetPlayerId());

	checkf(PlayerName3DWidgetComponentInternal, TEXT("ERROR: [%i] %hs:\n'PlayerName3DWidgetComponentInternal' is null!"), __LINE__, __FUNCTION__);
	const UUserWidget* LastWidget = PlayerName3DWidgetComponentInternal->GetWidget();
	if (LastWidget != PlayerNameWidget)
	{
		PlayerName3DWidgetComponentInternal->SetWidget(PlayerNameWidget);
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

// Called when an instance of this class is placed (in editor) or spawned
void AGRSPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SetDefaultPlayerMeshData();
}

// Called when the game starts or when spawned
void AGRSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Set the animation blueprint on very first character spawn
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	TryPossessController();

	// Update nickname and subscribe to the player name change
	InitPlayerNickName();

	HideSplineTrajectory();
}

// Returns the Skeletal Mesh of ghost revenge character
UMySkeletalMeshComponent& AGRSPlayerCharacter::GetMeshChecked() const
{
	return *CastChecked<UMySkeletalMeshComponent>(GetMesh());
}

// Possess a player 
void AGRSPlayerCharacter::TryPossessController()
{
	if (!HasAuthority()
		|| !IsActorInitialized() // Engine doesn't allow to possess before BeginPlay\PostInitializeComponents
		|| UUtilsLibrary::IsEditorNotPieWorld())
	{
		// Should not possess in PIE
		return;
	}


	AController* ControllerToPossess = nullptr;
	AMyPlayerController* MyPC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	if (MyPC)
	{
		ControllerToPossess = MyPC;
	}

	if (!ControllerToPossess
		|| ControllerToPossess == Controller)
	{
		return;
	}

	if (Controller)
	{
		// At first, unpossess previous controller
		Controller->UnPossess();
	}

	ControllerToPossess->Possess(this);
}

// Set and apply default skeletal mesh for this player
void AGRSPlayerCharacter::SetDefaultPlayerMeshData(bool bForcePlayerSkin/* = false*/)
{
	if (!HasAuthority())
	{
		return;
	}

	APlayerCharacter* PlayerCharacter = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter();

	const ELevelType PlayerFlag = UMyBlueprintFunctionLibrary::GetLevelType();
	ELevelType LevelType = PlayerFlag;

	if (bForcePlayerSkin)
	{
		// Force each bot to look like different player
		LevelType = static_cast<ELevelType>(1 << PlayerCharacter->GetPlayerId());
	}

	const UPlayerRow* Row = UPlayerDataAsset::Get().GetRowByLevelType<UPlayerRow>(TO_ENUM(ELevelType, LevelType));
	if (!ensureMsgf(Row, TEXT("ASSERT: [%i] %hs:\n'Row' is not found!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const int32 SkinsNum = Row->GetSkinTexturesNum();
	FBmrMeshData MeshData = FBmrMeshData::Empty;
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
	UpdateSplineTrajectory();
	
	if (CurrentHoldTimeInternal < 1.0f)
	{
		CurrentHoldTimeInternal = CurrentHoldTimeInternal + GetWorld()->GetDeltaSeconds();
		// UpdateSplineTrajectory();
	}
	else
	{
		ThrowProjectile(ActionValue);
	}

	UE_LOG(LogTemp, Warning, TEXT("GRS: Current hold time value: %f"), CurrentHoldTimeInternal);
}

// Spawn and send projectile
void AGRSPlayerCharacter::ThrowProjectile(const FInputActionValue& ActionValue)
{
	// Calculate throw direction and force
	FVector ThrowDirection = GetActorForwardVector() + FVector(5, 5, 0.0f); // Slight upward angle
	ThrowDirection.Normalize();

	FVector LaunchVelocity = ThrowDirection * 100;

	// Spawn bomb at chest height
	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * FVector(1500, 0, 150.0f);

	if (BombProjectileInternal == nullptr)
	{
		BombProjectileInternal = GetWorld()->SpawnActor<AGRSBombProjectile>(UGRSDataAsset::Get().GetProjectileClass(), SpawnLocation, GetActorRotation());
	}

	if (BombProjectileInternal)
	{
		BombProjectileInternal->Launch(LaunchVelocity);
	}

	CurrentHoldTimeInternal = 0.0f;
	//HideSplineTrajectory();
}

//  Add and update spline elements (trajectory) 
void AGRSPlayerCharacter::UpdateSplineTrajectory()
{
	float bigNumber = CurrentHoldTimeInternal + 350.0f;
	// Spawn bomb at chest height
	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() + CurrentHoldTimeInternal * FVector(100, 100, 100.0f);

	// Get start position (e.g., weapon muzzle)
	FVector StartPos = GetActorLocation() + GetActorForwardVector() * 100.0f;

	// Set launch velocity (forward direction with some upward angle)
	FVector LaunchVel = UGRSDataAsset::Get().GetVelocityParams();
	// 45-degree vector between up and right
	FVector UpRight45 = (GetActorForwardVector() + GetActorUpVector()).GetSafeNormal();

	// Predict and draw the trajectory
	FPredictProjectilePathParams Params = UGRSDataAsset::Get().GetChargePredictParams();
	Params.StartLocation = GetActorLocation();
	Params.LaunchVelocity = FVector(UpRight45.X + LaunchVel.X * CurrentHoldTimeInternal, 0 + LaunchVel.Y, UpRight45.Z + LaunchVel.Z);
	Params.ActorsToIgnore.Add(this);

	// Debug drawing settings
	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, Result);

	if (BombProjectileInternal == nullptr)
	{
		BombProjectileInternal = GetWorld()->SpawnActor<AGRSBombProjectile>(UGRSDataAsset::Get().GetProjectileClass(), Result.LastTraceDestination.Location, GetActorRotation());
	}

	if (BombProjectileInternal)
	{
		BombProjectileInternal->Destroy();
		
		BombProjectileInternal = GetWorld()->SpawnActor<AGRSBombProjectile>(UGRSDataAsset::Get().GetProjectileClass(), Result.LastTraceDestination.Location, GetActorRotation());
	}
}

// Hide spline elements (trajectory)
void AGRSPlayerCharacter::HideSplineTrajectory()
{
}

// Called every frame
void AGRSPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AGRSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
