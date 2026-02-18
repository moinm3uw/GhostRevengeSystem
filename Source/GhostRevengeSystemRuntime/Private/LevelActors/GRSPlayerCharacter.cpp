// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GRSPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrBombAbilityActor.h"
#include "Actors/BmrPawn.h"
#include "Animation/AnimInstance.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GRSGhostCharacterManagerComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSBombProjectile.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UI/Widgets/BmrPlayerNameWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

class UBmrPlayerRow;

/*********************************************************************************************
 * Nickname component
 **********************************************************************************************/

//  Set/Update player name
void AGRSPlayerCharacter::SetPlayerName(const ABmrPawn* MainCharacter)
{
	if (!MainCharacter)
	{
		return;
	}
	ABmrPlayerState* MyPlayerState = Cast<ABmrPlayerState>(MainCharacter->GetPlayerState());
	if (!MyPlayerState)
	{
		return;
	}

	checkf(PlayerName3DWidgetComponentInternal, TEXT("ERROR: [%i] %hs:\n'PlayerName3DWidgetComponent' is null!"), __LINE__, __FUNCTION__);
	PlayerName3DWidgetComponentInternal->Init(MyPlayerState);

	UEnum* EnumPtr = StaticEnum<EGRSCharacterSide>();
	const FString SideName = EnumPtr->GetNameStringByValue(TO_FLAG(UGRSWorldSubSystem::Get().GetCharacterSideFromActor(Cast<ACharacter>(this))));

	UUserWidget* Widget = PlayerName3DWidgetComponentInternal->GetWidget();
	UBmrPlayerNameWidget* NickName = Cast<UBmrPlayerNameWidget>(Widget);
	if (!ensureMsgf(NickName, TEXT("ASSERT: [%i] %hs:\n'NickName' condition is FALSE"), __LINE__, __FUNCTION__))
	{
		return;
	}

	NickName->SetPlayerName(FText::FromString(SideName));
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
	PlayerName3DWidgetComponentInternal = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponentInternal->SetupAttachment(RootComponent);

	// setup spline component
	ProjectileSplineComponentInternal = CreateDefaultSubobject<USplineComponent>(TEXT("ProjectileSplineComponent"));
	ProjectileSplineComponentInternal->AttachToComponent(MeshComponentInternal, FAttachmentTransformRules::KeepRelativeTransform);

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
	PrimaryActorTick.bCanEverTick = false;
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

// Called when the game starts or when spawned (on spawned on the level)
void AGRSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("AGRSPlayerCharacter::OnInitialize ghost character  --- %s - %s"), *this->GetName(), this->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem::Get().RegisterGhostCharacter(this);

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	// --- Activate aiming point
	AimingSphereComponent->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	AimingSphereComponent->SetVisibility(true);

	// --- ghost added to level
	OnGhostAddedToLevel.Broadcast();

	UGRSGhostCharacterManagerComponent* CharacterManagerComponent = UGRSWorldSubSystem::Get().GetGRSCharacterManagerComponent();
	if (CharacterManagerComponent)
	{
		CharacterManagerComponent->OnRefreshGhostCharacters.AddUniqueDynamic(this, &ThisClass::OnRefreshGhostCharacters);
		CharacterManagerComponent->OnActivateGhostCharacter.AddUniqueDynamic(this, &ThisClass::ActivateGhostCharacter);
		CharacterManagerComponent->OnPlayerCharacterPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);
		CharacterManagerComponent->OnRemoveGhostCharacterFromMap.AddUniqueDynamic(this, &ThisClass::OnRemoveGhostCharacterFromMap);
	}

	// --- handle initiation of ghosts player if MGF loaded during start of the game or in active game
	const ABmrGameState& GameState = ABmrGameState::Get();
	if (GameState.HasMatchingGameplayTag(FBmrGameStateTag::GameStarting) || GameState.HasMatchingGameplayTag(FBmrGameStateTag::InGame))
	{
		OnRefreshGhostCharacters();
	}
}

void AGRSPlayerCharacter::Destroyed()
{
	Super::Destroyed();
	RemoveActiveGameplayEffect();
	PerformCleanUp();
}

// Refresh ghost players required elements. Happens only when game is starting or active because requires to have all players (humans) to be connected
void AGRSPlayerCharacter::OnRefreshGhostCharacters_Implementation()
{
	// --- default params required for the fist start to have character prepared
	SetPlayerMeshData();
	// --- Init Player name
	// InitializePlayerName(UBmrBlueprintFunctionLibrary::GetLocalPawn());
	// --- Init character visuals (animations, skin)
	SetCharacterVisual(UBmrBlueprintFunctionLibrary::GetLocalPawn());
}

// Server-only logic Perform ghost character activation (possessing controller)
void AGRSPlayerCharacter::ActivateGhostCharacter_Implementation(AGRSPlayerCharacter* GhostCharacter, const ABmrPawn* PlayerCharacter)
{
	if (!GhostCharacter || GhostCharacter != this || !PlayerCharacter)
	{
		return;
	}

	AController* PlayerController = PlayerCharacter->GetController();
	if (!PlayerController || !PlayerController->HasAuthority())
	{
		return;
	}

	TryPossessController(PlayerController);
	SetPlayerName(PlayerCharacter);
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients.
void AGRSPlayerCharacter::OnPreRemovedFromLevel_Implementation(class UBmrMapComponent* MapComponent, class UObject* DestroyCauser)
{
	ABmrPawn* PlayerCharacter = MapComponent->GetOwner<ABmrPawn>();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	const ABmrPlayerState* DestroyCauserPlayerState = Cast<ABmrPlayerState>(DestroyCauser);
	if (!DestroyCauserPlayerState)
	{
		return;
	}

	const APawn* Causer = DestroyCauserPlayerState->GetPawn();
	if (Causer)
	{
		const AGRSPlayerCharacter* GRSCharacter = Cast<AGRSPlayerCharacter>(Causer);
		if (GRSCharacter && GRSCharacter == this)
		{
			OnGhostEliminatesPlayer.Broadcast(PlayerCharacter->GetActorLocation(), this);
			UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- OnGhostEliminatesPlayer.Broadcast"), __LINE__, __FUNCTION__);
			OnRemoveGhostCharacterFromMap(this);
		}
	}
}

// Remove ghost character from the level
void AGRSPlayerCharacter::OnRemoveGhostCharacterFromMap_Implementation(AGRSPlayerCharacter* GhostCharacter)
{
	if (!GhostCharacter || GhostCharacter != this || !GetController())
	{
		return;
	}

	// --- Disable aiming point
	AimingSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimingSphereComponent->SetVisibility(false);

	OnGhostRemovedFromLevel.Broadcast(GetController(), this);
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- OnGhostRemovedFromLevel.Broadcast"), __LINE__, __FUNCTION__);

	// --- not ghost character controlled, could be null
	if (GetController() && GetController()->HasAuthority())
	{
		GetController()->UnPossess();
	}
}

/*********************************************************************************************
 * GAS
 **********************************************************************************************/
// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGRSPlayerCharacter::GetAbilitySystemComponent() const
{
	const ABmrPlayerState* InPlayerState = GetPlayerState<ABmrPlayerState>();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

// To Remove current active applied gameplay effect
void AGRSPlayerCharacter::RemoveActiveGameplayEffect()
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
	if (!ASC)
	{
		return;
	}

	ASC->RemoveActiveGameplayEffect(GASEffectHandle);
	GASEffectHandle.Invalidate();
}

// To apply explosion gameplay effect
void AGRSPlayerCharacter::ApplyExplosionGameplayEffect()
{
	if (!HasAuthority())
	{
		return;
	}

	// Actor has ASC: apply damage effect through GAS
	if (GASEffectHandle.IsValid())
	{
		return;
	}
	const ABmrPlayerState* InPlayerState = GetPlayerState<ABmrPlayerState>();
	if (!ensureMsgf(InPlayerState, TEXT("ASSERT: [%i] %hs:\n'InPlayerState' is not null!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InPlayerState);
	TSubclassOf<UGameplayEffect> ExplosionDamageEffect = UGRSDataAsset::Get().GetExplosionDamageEffect();
	if (ensureMsgf(ExplosionDamageEffect, TEXT("ASSERT: [%i] %hs:\n'ExplosionDamageEffect' is not set!"), __LINE__, __FUNCTION__))
	{
		GASEffectHandle = ASC->ApplyGameplayEffectToSelf(ExplosionDamageEffect.GetDefaultObject(), /*Level*/ 1.f, ASC->MakeEffectContext());
	}
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

// Returns own character ID, e.g: 0, 1, 2, 3
int32 AGRSPlayerCharacter::GetPlayerId() const
{
	if (const ABmrPlayerState* MyPlayerState = GetPlayerState<ABmrPlayerState>())
	{
		return MyPlayerState->GetPlayerId();
	}

	return 0;
}

//  Set character visual once added to the level from a refence character (visuals, animations)
void AGRSPlayerCharacter::SetCharacterVisual(const ABmrPawn* PlayerCharacter)
{
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
	const int32 CurrentSkinIndex = MainCharacterMeshComponent->GetAppliedSkinIndex();

	UBmrSkeletalMeshComponent* CurrentMeshComponent = &GetMeshChecked();
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitSkeletalMesh(MainCharacterMeshComponent->GetMeshData());
	CurrentMeshComponent->ApplySkinByIndex(CurrentSkinIndex);
}

// Set and apply skeletal mesh for ghost player. Copy mesh from current player
void AGRSPlayerCharacter::SetPlayerMeshData(bool bForcePlayerSkin /* = false*/)
{
	ABmrPawn* PlayerCharacter = UBmrBlueprintFunctionLibrary::GetLocalPawn();
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	const EBmrLevelType PlayerFlag = UBmrBlueprintFunctionLibrary::GetLevelType();
	EBmrLevelType LevelType = PlayerFlag;

	const UBmrPlayerRow* Row = UBmrPlayerDataAsset::Get().GetRowByLevelType<UBmrPlayerRow>(TO_ENUM(EBmrLevelType, LevelType));
	if (!ensureMsgf(Row, TEXT("ASSERT: [%i] %hs:\n'Row' is not found!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const int32 SkinsNum = Row->GetSkinTexturesNum();
	FBmrMeshData MeshData;
	MeshData.Row = Row;
	MeshData.SkinIndex = PlayerCharacter->GetPlayerId() % SkinsNum;
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

// Add a mesh to the last element of the predict Projectile path results
void AGRSPlayerCharacter::AddMeshToEndProjectilePath(FVector Location)
{
	AimingSphereComponent->SetVisibility(true);
	AimingSphereComponent->SetWorldLocation(Location);
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

// Spawn bomb on aiming sphere position.
void AGRSPlayerCharacter::SpawnBomb(FBmrCell TargetCell)
{
	const FBmrCell& SpawnBombCell = UBmrCellUtilsLibrary::GetNearestFreeCell(TargetCell);
	FGameplayEventData EventData;
	EventData.Instigator = GetInstigator();
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(SpawnBombCell);
	GetAbilitySystemComponent()->HandleGameplayEvent(UGRSDataAsset::Get().GetTriggerBombTag(), &EventData);
}

//  Clean up the character for the MGF unload
void AGRSPlayerCharacter::PerformCleanUp()
{
	if (UCapsuleComponent* RootCapsuleComponent = GetCapsuleComponent())
	{
		RootCapsuleComponent->DestroyComponent();
	}

	if (UMovementComponent* MovementComponent = GetMovementComponent())
	{
		MovementComponent->DestroyComponent();
	}

	if (ProjectileSplineComponentInternal)
	{
		ProjectileSplineComponentInternal->DestroyComponent();
		ProjectileSplineComponentInternal = nullptr;
	}

	if (MeshComponentInternal)
	{
		MeshComponentInternal->DestroyComponent();
		MeshComponentInternal = nullptr;
	}

	if (BombProjectileInternal)
	{
		BombProjectileInternal->Destroy();
		BombProjectileInternal = nullptr;
	}

	if (AimingSphereComponent)
	{
		AimingSphereComponent->DestroyComponent();
		AimingSphereComponent = nullptr;
	}

	// --- perform clean up from subsystem MGF is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get().UnregisterGhostCharacter(this);
}