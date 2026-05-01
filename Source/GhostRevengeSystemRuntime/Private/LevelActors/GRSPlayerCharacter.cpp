// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GRSPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrBombAbilityActor.h"
#include "Actors/BmrPawn.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrPlayerArrowStartComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Components/GRSGhostCharacterManagerComponent.h"
#include "Components/GRSPlayerControllerComponent.h"
#include "Components/GrsPawnComponent.h"
#include "Components/GrsPlayerStateComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "Data/GRSDataAsset.h"
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
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"
#include "Utils/GrsPawnHelper.h"

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGRSPlayerCharacter::GetAbilitySystemComponent() const
{
	const ABmrPlayerState* InPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerStateChecked();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

/*********************************************************************************************
 * Initialization
 **********************************************************************************************/

// Sets default values for this character's properties
AGRSPlayerCharacter::AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBmrSkeletalMeshComponent>(MeshComponentName)) // Init UBmrSkeletalMeshComponent instead of USkeletalMeshComponent
{
	// --- Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// --- Replicate an actor
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	// --- Do not rotate player by camera
	bUseControllerRotationYaw = false;

	// --- Initialize skeletal mesh of the character
	UGrsPawnHelper::InitializeSkeletalMesh(this);

	// --- Configure the movement component
	UGrsPawnHelper::MovementComponentConfiguration(this);

	// --- Setup capsule component
	UGrsPawnHelper::SetupCapsuleComponent(this);

	// --- Initialize 3D widget component for the player name
	PlayerName3DWidgetComponent = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponent->SetupAttachment(RootComponent);

	// --- setup spline component
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

// Called on client when player ID is changed
void AGRSPlayerCharacter::OnRep_PlayerID()
{
	// --- Init Grs Pawn logic
	InitPawn(PlayerID);
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

	UE_LOG(LogTemp, Log, TEXT("AGRSPlayerCharacter::OnInitialize ghost character  --- %s - %s"), *this->GetName(), this->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The player character could be replicated faster than MGF(GFP) is loaded on client so the only we have to wait/check for subsystem to initialize as it is central loading point
void AGRSPlayerCharacter::OnInitialize(const struct FGameplayEventData& Payload)
{
	ABmrPlayerState* BmrPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerState();
	if (ensureMsgf(BmrPlayerState, TEXT("ASSERT: [%i] %hs:\n'BmrPlayerState' is not set!"), __LINE__, __FUNCTION__))
	{
		BmrPlayerState->OnOpponentsKilledNumChanged.AddUniqueDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
	}

	// --- Initiate and Activate aiming point
	checkf(AimingSphereComponent, TEXT("ERROR: [%i] %hs:\n'AimingSphereComponent' is null!"), __LINE__, __FUNCTION__);
	AimingSphereComponent->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	AimingSphereComponent->SetVisibility(false);

	// --- bind to  clear ghost data
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	// --- default params required for the fist start to have character prepared
	UGrsPawnHelper::InitPlayerMesh(this); // --- default init of mesh
	UGrsPawnHelper::SetCharacterVisual(this); // --- set character visuals (mesh, animation, skin)
	UGrsPawnHelper::InitializePlayerNameWidget(this); // somehow should be hidden as well.
	UGrsPawnHelper::SetVisibility(this, false); // -- hidden by default
	UGrsPawnHelper::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	OnGhostAddedToLevel.Broadcast(); // --- ghost added to level
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

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	UGrsPawnHelper::RefreshPawn(this);
}

// APawn Interface when this pawn was replicated by a new controller
void AGRSPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	UGrsPawnHelper::RefreshPawn(this);
}

//  APawn Interface when this pawn was replicated by a new player state
void AGRSPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	UGrsPawnHelper::RefreshPawn(this);
}

// APawn Interface when this pawn was unpossessed
void AGRSPlayerCharacter::UnPossessed()
{
	UGrsPawnHelper::SetVisibility(this, false);
	UGrsPawnHelper::SetArrowEnabled(this, false);

	Super::UnPossessed();
}

// Returns properties that are replicated for the lifetime of the actor channel
void AGRSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerID, Params);
}

// Is increased when this player kills an opponent
void AGRSPlayerCharacter::OnOpponentsKilledNumChanged_Implementation(int32 OpponentsKilledNum)
{
	// --- ignore reset cases
	if (OpponentsKilledNum < 1)
	{
		return;
	}

	// --- ghost eliminates a player - remove ghost from map
	ABmrPlayerController* CurrentPlayerController = Cast<ABmrPlayerController>(GetController());
	if (!ensureMsgf(CurrentPlayerController, TEXT("ASSERT: [%i] %hs:\n'CurrentPlayerController' is no longer available for Grs Pawn!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	if (CurrentPlayerController->HasAuthority())
	{
		CurrentPlayerController->UnPossess();
		ABmrPawn* PlayerCharacter = UBmrBlueprintFunctionLibrary::GetPawn(PlayerID);
		if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
		{
			return;
		}
		UGRSWorldSubSystem::Get().GetPlayerStateComponent(GetPlayerID())->RevivePlayerCharacter(PlayerCharacter);
	}

	RemoveGhostCharacterFromMap();
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

	UGrsPawnHelper::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	UGrsPawnHelper::SetVisibility(this, true);

	// --- clients calls:
	// --- update collision settings
	// --- activate arrow

	// --- just refresh visibility of player name needed, to be changed. Player name to be set by default
	// ABmrPlayerState* BmrPlayerState = Cast<ABmrPlayerState>(FromPlayerCharacter->GetPlayerState());
	// UpdatePlayerName(BmrPlayerState);

	// --- authority calls:
	TryPossessController(PlayerController);

	// --- set pawn location (side)
	UGrsPawnHelper::SetPawnSide(this);
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
	ABmrPlayerState* BmrPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerState();
	if (ensureMsgf(BmrPlayerState, TEXT("ASSERT: [%i] %hs:\n'BmrPlayerState' fail to obtain from player ID!"), __LINE__, __FUNCTION__))
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

// Add a mesh to the last element of the predict Projectile path results
void AGRSPlayerCharacter::AddMeshToEndProjectilePath(FVector Location)
{
	AimingSphereComponent->SetVisibility(true);
	AimingSphereComponent->SetWorldLocation(Location);
}

// Add spline points to the spline component
void AGRSPlayerCharacter::AddSplinePoints(FPredictProjectilePathResult& Result)
{
	UGrsPawnHelper::ClearTrajectorySplines(this);

	for (int32 i = 0; i < Result.PathData.Num(); i++)
	{
		FVector SplinePoint = Result.PathData[i].Location;
		ProjectileSplineComponentInternal->AddSplinePointAtIndex(SplinePoint, i, ESplineCoordinateSpace::World);
		ProjectileSplineComponentInternal->Mobility = EComponentMobility::Static;
	}

	ProjectileSplineComponentInternal->SetSplinePointType(Result.PathData.Num() - 1, ESplinePointType::CurveClamped, true);
	ProjectileSplineComponentInternal->UpdateSpline();
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

	UGrsPawnHelper::ClearTrajectorySplines(this);
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
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PerformCleanUp Started"), __LINE__, __FUNCTION__);
	RemoveGhostCharacterFromMap();

	if (AimingSphereComponent)
	{
		AimingSphereComponent->EmptyOverrideMaterials();
	}

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