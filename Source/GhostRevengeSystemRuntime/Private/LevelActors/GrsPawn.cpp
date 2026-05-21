// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GrsPawn.h"

// Grs
#include "Components/GrsCharacterManagerComponent.h"
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawnSubobjects/GrsPawnVisualizer.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Utils/GrsPawnHelper.h"

// Bmr
#include "Actors/BmrBombAbilityActor.h"
#include "Actors/BmrPawn.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrPlayerArrowStartComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameplayTags.h"
#include "UI/Widgets/BmrPlayerNameWidget.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"

// Aiming
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GhostRevengeSystemRuntimeModule.h"
#include "GrsUtils.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawn)

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGrsPawn::GetAbilitySystemComponent() const
{
	const ABmrPlayerState* InPlayerState = Cast<ABmrPlayerState>(UGrsPawnHelper::GetPlayerStateForPlayerID(this));
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

// Obtains players state from the cached and replicated PlayerID
class UGrsPlayerStateComponent* AGrsPawn::GetGrsPlayerStateComponent() const
{
	APlayerState* MyPlayerState = UGrsPawnHelper::GetPlayerStateForPlayerID(this);
	if (!ensureMsgf(MyPlayerState, TEXT("ASSERT: [%i] %hs:\n'MyPlayerState' failed to obtain from UGrsPawnHelper::GetPlayerStateForPlayerID!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}
	UGrsPlayerStateComponent* GrsPlayerStateComponent = MyPlayerState->FindComponentByClass<UGrsPlayerStateComponent>();
	if (ensureMsgf(GrsPlayerStateComponent, TEXT("ASSERT: [%i] %hs:\n'GrsPlayerStateComponent' is not found on APlayerState (not attached, initialized or no longer exists"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}
	return GrsPlayerStateComponent;
}

// Obtains players state from the cached and replicated PlayerID
UGrsPlayerStateComponent& AGrsPawn::GetGrsPlayerStateComponentChecked() const
{
	APlayerState* MyPlayerState = UGrsPawnHelper::GetPlayerStateForPlayerID(this);
	checkf(MyPlayerState, TEXT("ASSERT: [%i] %hs:\n'MyPlayerState' is nullptr, can not get PlayerState for '%i' PlayerID."), __LINE__, __FUNCTION__, PlayerID);

	UGrsPlayerStateComponent* GrsPlayerStateComponent = MyPlayerState->FindComponentByClass<UGrsPlayerStateComponent>();
	checkf(GrsPlayerStateComponent, TEXT("ASSERT: [%i] %hs:\n'GrsPlayerStateComponent' is nullptr, can not get PlayerState for '%i' PlayerID."), __LINE__, __FUNCTION__, PlayerID);

	return *GrsPlayerStateComponent;
}

// Sets default values for this character's properties
AGrsPawn::AGrsPawn(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBmrSkeletalMeshComponent>(MeshComponentName)) // Init UBmrSkeletalMeshComponent instead of USkeletalMeshComponent
{
	// --- Has movement that requires tick and also a player 3d arrow widget also playing animation in tick.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// --- Pawn is required to be replicated as it spawned on server and replicated to the clients
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	// --- Do not rotate player by camera
	bUseControllerRotationYaw = false;

	// --- Initialize skeletal mesh of the character
	FGrsPawnVisualizer::InitializeSkeletalMesh(this);

	// --- Configure the movement component
	FGrsPawnVisualizer::MovementComponentConfiguration(this);

	// --- Setup capsule component
	FGrsPawnVisualizer::InitCapsuleComponent(this);

	// --- Setup player nickname 3d Widget (on top of player)
	PlayerNickName3DWidgetComponent = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerNickName3DWidgetComponent->SetupAttachment(RootComponent);

	// --- Initialize 3D player arrow widget component that appears on top of character when a player start to control it
	PlayerArrowStartComponent = CreateDefaultSubobject<UBmrPlayerArrowStartComponent>(TEXT("PlayerArrowStartWidgetComponent"));
	PlayerArrowStartComponent->SetupAttachment(RootComponent);

	// --- Initial setup of spline component and aiming sphere
	// --- setup spline component
	AimingSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("ProjectileSplineComponent"));
	AimingSplineComponent->AttachToComponent(AimingMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	AimingSphereComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereComp"));
}

// Initialize player name widget (on top of character)
void AGrsPawn::InitializePlayerNameWidget()
{
	ABmrPlayerState* MyPlayerState = Cast<ABmrPlayerState>(UGrsPawnHelper::GetPlayerStateForPlayerID(this));
	if (!ensureMsgf(MyPlayerState, TEXT("ASSERT: [%i] %hs:\n'MyPlayerState' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PlayerNickName3DWidgetComponent->Init(MyPlayerState);
}

// Returns properties that are replicated for the lifetime of the actor channel
void AGrsPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerID, Params);
}

// Called on client when player ID is changed
void AGrsPawn::OnRep_PlayerID()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// --- Init Grs Pawn logic
	InitPawn(PlayerID);
}

// Basic initialization of the Pawn
void AGrsPawn::InitPawn(int32 NewPlayerId)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs (%s) PlayerID: %i  "), __LINE__, __FUNCTION__, this->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), NewPlayerId);
	if (!ensureMsgf(NewPlayerId >= 0, TEXT("ASSERT: [%i] %hs:\n'NewPlayerId' invalid. Value is less than 0!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	if (PlayerID != NewPlayerId)
	{
		PlayerID = NewPlayerId;
	}

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The player character could be replicated faster than GFP is loaded on client so the only we have to wait/check for subsystem to initialize as it is central loading point
void AGrsPawn::OnInitialize(const struct FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// --- default params required for the fist start to have character prepared
	FGrsPawnVisualizer::InitPlayerMesh(this); // --- default init of mesh
	FGrsPawnVisualizer::InitCharacterVisual(this); // --- set character visuals (mesh, animation, skin)
	FGrsPawnVisualizer::SetVisibility(this, false); // -- hidden by default
	FGrsPawnVisualizer::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	InitAimingSphere();
	InitializePlayerNameWidget();

	// --- bind to clear ghost data
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	// --- listens to event when BmrPawn controller by player was eliminated on level
	ABmrPawn* MyPawn = UBmrBlueprintFunctionLibrary::GetPawn(PlayerID);
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

// Listen game states to remove ghost character from level
void AGrsPawn::OnGameStateChanged_Implementation(const struct FGameplayEventData& Payload)
{
	HideGhostCharacterFromMap();
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients.
void AGrsPawn::OnPreRemovedFromLevel_Implementation(class UBmrMapComponent* PlayerMapComponent, class UObject* DestroyCauser)
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

// Activates ghost with required initiation
void AGrsPawn::TryActivateGhostCharacter(AGrsPawn* GhostCharacter, ABmrPawn* FromPlayerCharacter)
{
	if (!GhostCharacter
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
	AGrsPawn* CurrentGhostCharacter = Cast<AGrsPawn>(PlayerController->GetPawn());
	if (CurrentGhostCharacter)
	{
		return;
	}

	FGrsPawnVisualizer::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	FGrsPawnVisualizer::SetVisibility(this, true);

	// --- authority calls:
	TryPossessController(PlayerController);

	// --- set pawn location (side)
	UGrsPawnHelper::SetPawnToAvailableSide(this);
}

//  Possess a player controller
void AGrsPawn::TryPossessController(AController* PlayerController)
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

// APawn Interface when this pawn was possessed by a new controller
void AGrsPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	RefreshPawn();
}

// APawn Interface when this pawn was replicated by a new controller
void AGrsPawn::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	RefreshPawn();
}

//  APawn Interface when this pawn was replicated by a new player state
void AGrsPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	RefreshPawn();
}

// Overridable function called whenever this actor is being removed from a level
void AGrsPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);
	PerformCleanUp();
}

// APawn Interface when this pawn was unpossessed
void AGrsPawn::UnPossessed()
{
	Super::UnPossessed();

	HideGhostCharacterFromMap(); // remove on clients and server ghost from map
}

// Refresh and enable this pawn
void AGrsPawn::RefreshPawn()
{
	FGrsPawnVisualizer::SetVisibility(this, true);
	ClearTrajectorySplines();
	AimingSphereComponent->SetVisibility(true);
	PlayerArrowStartComponent->SetArrowEnabled(true);
}

// Remove ghost character from the level
void AGrsPawn::HideGhostCharacterFromMap()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// --- change visibility of this pawn
	// -- change nickname visibility of this pawn
	// --- update collision mod of this pawn if needed

	FGrsPawnVisualizer::SetVisibility(this, false);
	AimingSphereComponent->SetVisibility(false);
	PlayerArrowStartComponent->SetArrowEnabled(false);
	ClearTrajectorySplines();

	UGRSWorldSubSystem::Get().UnregisterGhostCharacter(this);

	if (HasAuthority())
	{
		SetActorLocation(UGrsUtils::MaxPos);
	}
}

//  Clean up the character for the GFP unload
void AGrsPawn::PerformCleanUp()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	HideGhostCharacterFromMap();

	if (AimingSphereComponent)
	{
		AimingSphereComponent->EmptyOverrideMaterials();
	}
	if (AimingMeshComponent)
	{
		AimingMeshComponent->DestroyComponent();
		AimingMeshComponent = nullptr;
	}

	PlayerID = 0;

	// --- perform clean up from subsystem GFP is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem& WorldSubSystem = UGRSWorldSubSystem::Get();
	WorldSubSystem.UnregisterGhostCharacter(this);
	WorldSubSystem.ResetRevivedPlayers();

	if (HasAuthority())
	{
		UPoolManagerSubsystem* PoolManager = UPoolManagerSubsystem::GetPoolManager();
		if (PoolManager)
		{
			FPoolObjectHandle SpawnObjectHandle = PoolManager->FindPoolHandleByObject(this);
			if (SpawnObjectHandle.IsValid())
			{
				PoolManager->ReturnToPool(SpawnObjectHandle);
				SpawnObjectHandle.Invalidate();
			}
		}
	}
}

/*********************************************************************************************
 * Aiming functionality
 **********************************************************************************************/

// Initiate and activate aiming point
void AGrsPawn::InitAimingSphere()
{
	AimingSphereComponent->SetStaticMesh(UGRSDataAsset::Get().GetProjectileMesh());
	AimingSphereComponent->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	AimingSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimingSphereComponent->SetVisibility(false);
}

// Hide spline elements (trajectory)
void AGrsPawn::ClearTrajectorySplines()
{
	for (USplineMeshComponent* SplineMeshComponent : AimingSplineMeshArray)
	{
		SplineMeshComponent->DestroyComponent();
	}

	AimingSplineMeshArray.Empty();
	AimingSplineComponent->ClearSplinePoints();
}
