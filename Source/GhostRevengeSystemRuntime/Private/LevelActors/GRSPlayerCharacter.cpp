// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GRSPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrBombAbilityActor.h"
#include "Actors/BmrPawn.h"
#include "Animation/AnimInstance.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrPlayerArrowStartComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GRSGhostCharacterManagerComponent.h"
#include "Components/GRSPlayerControllerComponent.h"
#include "Components/GrsPawnComponent.h"
#include "Components/GrsPlayerStateComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "DataRegistries/BmrPlayerRow.h"
#include "DataRegistries/BmrPlayerSkinRow.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/BmrPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GrsGameplayTags.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UI/Widgets/BmrPlayerNameWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

/*********************************************************************************************
 * Nickname component
 **********************************************************************************************/

//  Initialize player name widget (on top of character)
void AGRSPlayerCharacter::InitializePlayerNameWidget()
{
	ABmrPlayerState* MyPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerStateChecked();
	if (!ensureMsgf(MyPlayerState, TEXT("ASSERT: [%i] %hs:\n'MyPlayerState' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(PlayerName3DWidgetComponent, TEXT("ASSERT: [%i] %hs:\n'PlayerName3DWidgetComponentInternal' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PlayerName3DWidgetComponent->Init(MyPlayerState);
}

/*********************************************************************************************
 * Initialization
 **********************************************************************************************/

// Sets default values for this character's properties
AGRSPlayerCharacter::AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBmrSkeletalMeshComponent>(MeshComponentName)) // Init UBmrSkeletalMeshComponent instead of USkeletalMeshComponent
{
	// Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
	SetDefaultParams();

	// Initialize skeletal mesh of the character
	InitializeSkeletalMesh();

	// Configure the movement component
	MovementComponentConfiguration();

	// Setup capsule component
	SetupCapsuleComponent();

	// Initialize 3D widget component for the player name
	PlayerName3DWidgetComponent = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponent->SetupAttachment(RootComponent);

	// setup spline component
	ProjectileSplineComponentInternal = CreateDefaultSubobject<USplineComponent>(TEXT("ProjectileSplineComponent"));
	ProjectileSplineComponentInternal->AttachToComponent(MeshComponentInternal, FAttachmentTransformRules::KeepRelativeTransform);

	PlayerArrowStartComponent = CreateDefaultSubobject<UBmrPlayerArrowStartComponent>(TEXT("PlayerArrowStartWidgetComponent"));
	PlayerArrowStartComponent->SetupAttachment(RootComponent);

	AimingSphereComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereComp"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		AimingSphereComponent->SetStaticMesh(SphereMesh.Object);
	}
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
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PerformCleanUp"), __LINE__, __FUNCTION__);
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

/*********************************************************************************************
 * Main functionality (core loop)
 **********************************************************************************************/

// Basic initialization of the Pawn
void AGRSPlayerCharacter::InitPawn(int32 NewPlayerId)
{
	if (PlayerID != NewPlayerId)
	{
		PlayerID = NewPlayerId;
	}
}

//  Register owning pawn component
void AGRSPlayerCharacter::RegisterPawnComponent(UGrsPawnComponent* NewPawnComponent)
{
	if (NewPawnComponent || OwningPawnComponent != NewPawnComponent)
	{
		OwningPawnComponent = NewPawnComponent;

		ABmrPawn* MyPawn = &OwningPawnComponent->GetBmrPawnChecked();
		if (MyPawn)
		{
			UBmrMapComponent* MapComponent = UBmrMapComponent::GetMapComponent(MyPawn);
			if (!ensureMsgf(MapComponent, TEXT("ASSERT: [%i] %hs:\n 'MapComponent' is null!"), __LINE__, __FUNCTION__))
			{
				return;
			}

			MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);
		}
	}
}

// Called when the game starts or when spawned (on spawned on the level)
void AGRSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("AGRSPlayerCharacter::OnInitialize ghost character  --- %s - %s"), *this->GetName(), this->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// Overridable function called whenever this actor is being removed from a level
void AGRSPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	PerformCleanUp();
	Super::EndPlay(EndPlayReason);
}

// APawn Interface when this pawn was possessed by a new controller
void AGRSPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!bIsReady())
	{
		return;
	}

	RefreshPawn();
}

// APawn Interface when this pawn was replicated by a new controller
void AGRSPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (!bIsReady())
	{
		return;
	}

	RefreshPawn();
}

//  APawn Interface when this pawn was replicated by a new player state
void AGRSPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	ABmrPlayerState* BmrPlayerState = Cast<ABmrPlayerState>(GetPlayerState());
	if (ensureMsgf(BmrPlayerState, TEXT("ASSERT: [%i] %hs:\n'BmrPlayerState' is not set!"), __LINE__, __FUNCTION__))
	{
		BmrPlayerState->OnOpponentsKilledNumChanged.AddUniqueDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
	}

	if (!bIsReady())
	{
		return;
	}

	RefreshPawn();
}

// Refresh the pawn visuals
void AGRSPlayerCharacter::RefreshPawn()
{
	// --- Clear splines
	ClearTrajectorySplines();
	SetVisibility(true);
	SetArrowEnabled(true);
	AimingSphereComponent->SetVisibility(true);
}

// Checks if Pawn is replicated fully (player state and controller present
bool AGRSPlayerCharacter::bIsReady()
{
	if (!GetController())
	{
		return false;
	}

	if (!GetPlayerState())
	{
		// UE_LOG(LogGrs, Verbose, TEXT("GetPlayerState() is not available"), ___FUNCTION___); // ~ Log LogGrs Verbose
		return false;
	}

	return true;
}

// APawn Interface when this pawn was unpossessed
void AGRSPlayerCharacter::UnPossessed()
{
	SetVisibility(false);
	SetArrowEnabled(false);

	Super::UnPossessed();
}

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGRSPlayerCharacter::GetAbilitySystemComponent() const
{
	const ABmrPlayerState* InPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerStateChecked();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

void AGRSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerID, Params);
}

// The player character could be replicated faster than MGF(GFP) is loaded on client so the only we have to wait/check for subsystem to initialize as it is central loading point
void AGRSPlayerCharacter::OnInitialize(const struct FGameplayEventData& Payload)
{
	// --- Activate aiming point
	checkf(AimingSphereComponent, TEXT("ERROR: [%i] %hs:\n'AimingSphereComponent' is null!"), __LINE__, __FUNCTION__);
	AimingSphereComponent->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	AimingSphereComponent->SetVisibility(false);

	// --- bind to  clear ghost data
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	// --- default params required for the fist start to have character prepared
	InitPlayerMesh(); // --- default init of mesh
	SetCharacterVisual(); // --- set character visuals (mesh, animation, skin)

	InitializePlayerNameWidget(); // somehow should be hidden as well.
	SetVisibility(false); // -- hidden by default

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	OnGhostAddedToLevel.Broadcast(); // --- ghost added to level
}

// Is increased when this player kills an opponent
void AGRSPlayerCharacter::OnOpponentsKilledNumChanged_Implementation(int32 OpponentsKilledNum)
{
	// --- ghost eliminates a player - remove ghost from map
	UGRSPlayerControllerComponent* GrsControllerComponent = Cast<UGRSPlayerControllerComponent>(GetController()->FindComponentByClass<UGRSPlayerControllerComponent>());
	if (!ensureMsgf(GrsControllerComponent, TEXT("ASSERT: [%i] %hs:\n'GrsControllerComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	RemoveGhostCharacterFromMap();

	if (GetController()->HasAuthority())
	{
		GetController()->UnPossess();

		ABmrPawn* PlayerCharacter = Cast<ABmrPawn>(GrsControllerComponent->GetMainPlayerPawn());
		if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'BmrPawn' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}
		UGRSWorldSubSystem::Get().GetPlayerStateComponent(GetPlayerID())->RevivePlayerCharacter(PlayerCharacter);
	}
}

// Listen game states to remove ghost character from level
void AGRSPlayerCharacter::OnGameStateChanged_Implementation(const struct FGameplayEventData& Payload)
{
	if (!Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		// -- release (unpossess) all ghosts
		RemoveGhostCharacterFromMap();
	}
}

// Activates ghost with required initiation
void AGRSPlayerCharacter::TryActivateGhostCharacter(AGRSPlayerCharacter* GhostCharacter, ABmrPawn* FromPlayerCharacter)
{
	if (!GhostCharacter
	    || GhostCharacter != this
	    || !FromPlayerCharacter
	    || !UGRSWorldSubSystem::Get().IsRevivable(FromPlayerCharacter))
	{
		return;
	}

	AController* PlayerController = FromPlayerCharacter->GetController();
	if (!PlayerController)
	{
		return;
	}

	// --- check if the player already possessed by other ghost
	AGRSPlayerCharacter* CurrentGhostCharacter = Cast<AGRSPlayerCharacter>(PlayerController->GetPawn());
	if (CurrentGhostCharacter)
	{
		return;
	}

	UGRSPlayerControllerComponent* GrsControllerComponent = Cast<UGRSPlayerControllerComponent>(PlayerController->GetComponentByClass(UGRSPlayerControllerComponent::StaticClass()));
	if (!ensureMsgf(GrsControllerComponent, TEXT("ASSERT: [%i] %hs:\n'GrsControllerComponent' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	GrsControllerComponent->SetPossessedPlayerPawn(FromPlayerCharacter);

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SetVisibility(true);

	// --- clients calls:
	// --- update collision settings
	// --- activate arrow

	// --- just refresh visibility of player name needed, to be changed. Player name to be set by default
	// ABmrPlayerState* BmrPlayerState = Cast<ABmrPlayerState>(FromPlayerCharacter->GetPlayerState());
	// UpdatePlayerName(BmrPlayerState);

	// --- authority calls:
	TryPossessController(PlayerController);

	// --- set pawn location (side)
	SetPawnSide();
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients.
void AGRSPlayerCharacter::OnPreRemovedFromLevel_Implementation(class UBmrMapComponent* PlayerMapComponent, class UObject* DestroyCauser)
{
	ABmrPawn* PlayerCharacter = PlayerMapComponent->GetOwner<ABmrPawn>();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__)
	    || PlayerCharacter->IsBotControlled()
	    || !DestroyCauser)
	{
		return;
	}

	const ABmrPlayerState* DestroyCauserPlayerState = Cast<ABmrPlayerState>(DestroyCauser);
	if (!DestroyCauserPlayerState)
	{
		return;
	}

	APawn* Causer = DestroyCauserPlayerState->GetPawn();
	if (!Causer)
	{
		return;
	}

	// --- a player was eliminated - activate ghost character
	if (PlayerCharacter->GetPlayerId() == PlayerID)
	{
		TryActivateGhostCharacter(this, PlayerCharacter);
	}
}

// Remove ghost character from the level
void AGRSPlayerCharacter::RemoveGhostCharacterFromMap()
{
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- RemoveGhostCharacterFromMap Started"), __LINE__, __FUNCTION__);
	// --- move all functional part such as posses to ability
	// --- possess back to player character for any cases
	// PlayerCharacter->GetMeshComponentChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	// --- Disable aiming point
	if (AimingSphereComponent)
	{
		AimingSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AimingSphereComponent->SetVisibility(false);
	}

	// --- reset bindings
	ABmrPlayerState* BmrPlayerState = Cast<ABmrPlayerState>(GetPlayerState());
	if (BmrPlayerState)
	{
		if (BmrPlayerState->OnOpponentsKilledNumChanged.IsBound())
		{
			BmrPlayerState->OnOpponentsKilledNumChanged.RemoveDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
		}
	}

	UGRSWorldSubSystem::Get().UnregisterGhostCharacter(this);

	// --- change visibility of this pawn
	// -- change nickname visibility of this pawn
	// --- update collision mod of this pawn if needed
}

// Called on client when player ID is changed
void AGRSPlayerCharacter::OnRep_PlayerID()
{
	// --- Init Grs Pawn logic
	InitPawn(PlayerID);
}

/*********************************************************************************************
 * Utils
 **********************************************************************************************/
// Returns the Skeletal Mesh of ghost revenge character
UBmrSkeletalMeshComponent& AGRSPlayerCharacter::GetMeshChecked() const
{
	return *CastChecked<UBmrSkeletalMeshComponent>(GetMesh());
}

// Set visibility of the player character
void AGRSPlayerCharacter::SetVisibility(bool Visibility)
{
	GetMesh()->SetVisibility(Visibility, true);
}

// Set visibility of the arrow on top of player character
void AGRSPlayerCharacter::SetArrowEnabled(bool bVisibility)
{
	PlayerArrowStartComponent->SetArrowEnabled(bVisibility);
}

//  Initialize character visual (SkeletalMesh, Skins)  once added to the level by utilizing player id
void AGRSPlayerCharacter::SetCharacterVisual()
{
	ABmrPawn* PlayerCharacter = &UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerStateChecked()->GetPawnChecked();
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UBmrPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	const UBmrSkeletalMeshComponent* MainCharacterMeshComponent = &PlayerCharacter->GetMeshComponentChecked();
	if (!ensureMsgf(MainCharacterMeshComponent, TEXT("ASSERT: [%i] %hs:\n'MainCharacterMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	const FName CurrentSkinRowName = MainCharacterMeshComponent->GetAppliedSkinRowName();

	UBmrSkeletalMeshComponent* CurrentMeshComponent = &GetMeshChecked();
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitSkeletalMesh(MainCharacterMeshComponent->GetMeshData());
	CurrentMeshComponent->ApplySkinByRowName(CurrentSkinRowName);
}

// Set and apply skeletal mesh for ghost player. Copy mesh from current player
void AGRSPlayerCharacter::InitPlayerMesh()
{
	const ABmrPawn* PlayerCharacter = &UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerStateChecked()->GetPawnChecked();
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	const FBmrPlayerRow* Row = FBmrPlayerRow::GetFirstRow();
	const FName RowName = FBmrPlayerRow::GetFirstRowName();
	if (!ensureMsgf(Row, TEXT("ASSERT: [%i] %hs:\n'Row' is not found!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	FBmrMeshData MeshData = FBmrMeshData::Empty;
	MeshData.RowName = RowName;
	MeshData.SkinRowName = FBmrPlayerSkinRow::GetSkinRowName(Row->PlayerTag, PlayerCharacter->GetPlayerId());
	GetMeshChecked().InitSkeletalMesh(MeshData);
}

//  Possess a player controller
void AGRSPlayerCharacter::TryPossessController(AController* PlayerController)
{
	if (!PlayerController || !PlayerController->HasAuthority())
	{
		return;
	}

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

//  Set side for this pawn (left or right)
void AGRSPlayerCharacter::SetPawnSide()
{
	if (!HasAuthority())
	{
		return;
	}
	EGRSCharacterSide CharacterSide = UGRSWorldSubSystem::Get().RegisterGhostCharacter(this);

	checkf(!(CharacterSide == EGRSCharacterSide::None), TEXT("ERROR: [%i] %hs:\n'CharacterSide' is none!"), __LINE__, __FUNCTION__);

	FBmrCell ActorSpawnLocation;
	float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

	if (CharacterSide == EGRSCharacterSide::Left)
	{
		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
	}
	else if (CharacterSide == EGRSCharacterSide::Right)
	{
		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
	}

	// Match the Z axis to what we have on the level
	ActorSpawnLocation.Location.Z = 100.0f;
	SetActorLocation(ActorSpawnLocation);
}

// Add a mesh to the last element of the predict Projectile path results
void AGRSPlayerCharacter::AddMeshToEndProjectilePath(FVector Location)
{
	AimingSphereComponent->SetVisibility(true);
	AimingSphereComponent->SetWorldLocation(Location);
}

// Applies spawn bomb gameplay effect
void AGRSPlayerCharacter::ApplyExplosionGameplayEffect()
{
	UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->ApplyBombSpawningGameplayEffect();
}

// Removes spawn bomb gameplay effect
void AGRSPlayerCharacter::RemoveExplosionGameplayEffect()
{
	UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->RemoveBombSpawningGameplayEffect();
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

//  Add spline mesh to spline points
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

// Throw projectile event, bound to onetime button press
void AGRSPlayerCharacter::ThrowProjectile()
{
	//--- Calculate Cell to spawn bomb
	FBmrCell CurrentCell;
	CurrentCell.Location = AimingSphereComponent->GetComponentLocation();

	//--- hide aiming sphere from ui
	AimingSphereComponent->SetVisibility(false);

	SpawnBomb(CurrentCell);

	FVector ThrowDirection = GetActorForwardVector() + FVector(5, 5, 0.0f);
	ThrowDirection.Normalize();
	FVector LaunchVelocity = ThrowDirection * 100;

	ClearTrajectorySplines();
}

// Spawn bomb on aiming sphere position.
void AGRSPlayerCharacter::SpawnBomb(FBmrCell TargetCell)
{
	const FBmrCell& SpawnBombCell = UBmrCellUtilsLibrary::GetNearestFreeCell(TargetCell);

	// Activate bomb ability
	FGameplayEventData EventData;
	EventData.EventTag = UGRSDataAsset::Get().GetTriggerBombTag();
	EventData.Instigator = this;
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(SpawnBombCell);
	UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
}

//  Clean up the character for the MGF unload
void AGRSPlayerCharacter::PerformCleanUp()
{
	RemoveGhostCharacterFromMap();

	if (AimingSphereComponent)
	{
		AimingSphereComponent->EmptyOverrideMaterials();
	}
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PerformCleanUp Started"), __LINE__, __FUNCTION__);
	if (MeshComponentInternal)
	{
		MeshComponentInternal->DestroyComponent();
		MeshComponentInternal = nullptr;
	}

	// Components created via CreateDefaultSubobject must NOT cleanup, they are defaults this actor:
	// ProjectileSplineComponentInternal, AimingSphereComponent, PlayerName3DWidgetComponentInternal

	OwningPawnComponent = nullptr;
	PlayerID = 0;

	// --- perform clean up from subsystem MGF is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get().UnregisterGhostCharacter(this);
	UGRSWorldSubSystem::Get().ResetRevivedPlayers();
}