// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "LevelActors/GrsPawn.h"

// Grs
#include "Components/GrsPlayerStateComponent.h"
#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "GrsGameplayTags.h"
#include "GrsUtils.h"
#include "LevelActors/GrsPawnSubobjects/GrsPawnVisualizer.h"
#include "Utils/GrsPawnHelper.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrPlayerArrowStartComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrPawnReadySubsystem.h"
#include "UI/Widgets/BmrPlayerNameWidget.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// PoolManager
#include "PoolManagerSubsystem.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "AbilitySystemComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawn)

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGrsPawn::GetAbilitySystemComponent() const
{
	const UGrsPlayerStateComponent* MyPlayerStateComponent = GetGrsPlayerStateComponent();
	return MyPlayerStateComponent ? MyPlayerStateComponent->GetCurrentPlayerStateChecked().GetAbilitySystemComponent() : nullptr;
}

// Returns cached GRS component of the player state that owns PlayerID
UGrsPlayerStateComponent* AGrsPawn::GetGrsPlayerStateComponent() const
{
	return PlayerStateComponent.Get();
}

// Obtains players state from the cached and replicated PlayerID
UGrsPlayerStateComponent& AGrsPawn::GetGrsPlayerStateComponentChecked() const
{
	UGrsPlayerStateComponent* GrsPlayerStateComponent = GetGrsPlayerStateComponent();
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
	FGrsPawnVisualizer::ConfigureMovementComponent(this);

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
	/* @PR JanSeliv [Potential Bug] - AimingMeshComponent never assigned (always nullptr), AttachToComponent to null parent silently no-ops, spline never attached.
	 * Attach to RootComponent or existing mesh, or create AimingMeshComponent via CreateDefaultSubobject first */
	AimingSplineComponent->AttachToComponent(AimingMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	AimingSphereComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereComp"));
}

// Initialize player name widget (on top of character)
void AGrsPawn::InitializePlayerNameWidget()
{
	const UGrsPlayerStateComponent* MyPlayerStateComponent = PlayerStateComponent.Get();
	if (!ensureMsgf(MyPlayerStateComponent, TEXT("ASSERT: [%i] %hs:\n'PlayerStateComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PlayerNickName3DWidgetComponent->Init(&MyPlayerStateComponent->GetCurrentPlayerStateChecked());
}

// Returns properties that are replicated for the lifetime of the actor channel
void AGrsPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerID, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bIsGhostActive, Params);
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
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs (%s) PlayerID: %i  "), __LINE__, __FUNCTION__, HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), NewPlayerId);
	if (!ensureMsgf(NewPlayerId >= 0, TEXT("ASSERT: [%i] %hs:\n'NewPlayerId' invalid. Value is less than 0!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PlayerID = NewPlayerId;

	// --- pawn can be reinitialized and serve another player, so previously cached references do not belong to it anymore
	if (UGrsPlayerStateComponent* PreviousPlayerStateComponent = PlayerStateComponent.Get())
	{
		PreviousPlayerStateComponent->ResetGhostSide();
	}
	PlayerStateComponent.Reset();
	StopListeningPlayerCharacterRemoval();

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

// The player character could be replicated faster than GFP is loaded on client so the only we have to wait/check for subsystem to initialize as it is central loading point
void AGrsPawn::OnInitialize_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// --- bind to clear ghost data and to re-init it for each match
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	// --- init once player character of this ghost is ready, is replayed for already ready player characters
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_PawnReady, this, &ThisClass::OnPawnReady);
}

// Listen game states to remove ghost character from level and to re-init it for each match
void AGrsPawn::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	// --- hiding also unsubscribes from the player character, so it has to be re-initialized for the match below
	HideGhostCharacterFromMap();

	TryInitGhostCharacter();
}

// Is called when any player character (BmrPawn) is spawned, possessed, and replicated
void AGrsPawn::OnPawnReady_Implementation(const FGameplayEventData& Payload)
{
	const ABmrPawn* ReadyPawn = Cast<ABmrPawn>(Payload.Instigator.Get());
	if (ReadyPawn
	    && ReadyPawn->GetPlayerId() == PlayerID)
	{
		TryInitGhostCharacter();
	}
}

// Inits this ghost for the current player character of PlayerID only when the match is starting or in progress and that player character is ready
void AGrsPawn::TryInitGhostCharacter()
{
	const ABmrGameState& GameState = ABmrGameState::Get();
	const bool bIsMatchStartingOrInProgress = GameState.HasMatchingGameplayTag(FBmrGameStateTag::GameStarting)
	                                          || GameState.HasMatchingGameplayTag(FBmrGameStateTag::InGame);

	const ABmrPawn* PlayerCharacter = UBmrBlueprintFunctionLibrary::GetPawn(PlayerID);
	if (!bIsMatchStartingOrInProgress
	    || !UBmrPawnReadySubsystem::Get().IsReady(PlayerCharacter))
	{
		return;
	}
	
	if (ListenedMapComponent.IsValid()
	    && ListenedMapComponent.Get() == UBmrMapComponent::GetMapComponent(PlayerCharacter))
	{
		return;
	}

	InitGhostCharacter(PlayerCharacter);
}

// Inits this ghost for given player character
void AGrsPawn::InitGhostCharacter(const ABmrPawn* PlayerCharacter)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) PlayerID: %i"), __LINE__, __FUNCTION__, HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), PlayerID);
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// --- default params required for the fist start to have character prepared
	FGrsPawnVisualizer::InitPlayerMesh(this); // --- default init of mesh
	FGrsPawnVisualizer::InitCharacterVisual(this); // --- set character visuals (mesh, animation, skin)
	ApplyGhostActiveVisuals(); // -- hidden unless already active: on client the activation can be replicated before this init
	FGrsPawnVisualizer::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	InitAimingSphere();
	
	const APlayerState* MyPlayerState = UGrsPawnHelper::GetPlayerStateForPlayerID(this);
	PlayerStateComponent = MyPlayerState ? MyPlayerState->FindComponentByClass<UGrsPlayerStateComponent>() : nullptr;
	if (ensureMsgf(PlayerStateComponent.IsValid(), TEXT("ASSERT: [%i] %hs:\n'PlayerStateComponent' is not found on the player state of '%i' PlayerID!"), __LINE__, __FUNCTION__, PlayerID))
	{
		InitializePlayerNameWidget();
	}

	// --- listens to event when player character of this ghost was eliminated on level
	// --- Unsubscribes from previous player character first, since it could be respawned as another one
	StopListeningPlayerCharacterRemoval();
	UBmrMapComponent* MapComponent = UBmrMapComponent::GetMapComponent(PlayerCharacter);
	if (!ensureMsgf(MapComponent, TEXT("ASSERT: [%i] %hs:\n 'MapComponent' is null!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);
	ListenedMapComponent = MapComponent;
}

// Unsubscribes from removal from level of the player character this ghost was initialized for
void AGrsPawn::StopListeningPlayerCharacterRemoval()
{
	if (UBmrMapComponent* MapComponent = ListenedMapComponent.Get())
	{
		MapComponent->OnPreRemovedFromLevel.RemoveDynamic(this, &ThisClass::OnPreRemovedFromLevel);
	}

	ListenedMapComponent.Reset();
}

// Called right before owner actor going to remove from the Generated Map, on both server and clients.
void AGrsPawn::OnPreRemovedFromLevel_Implementation(UBmrMapComponent* PlayerMapComponent, UObject* DestroyCauser)
{
	const ABmrPawn* PlayerCharacter = PlayerMapComponent->GetOwner<ABmrPawn>();
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
void AGrsPawn::TryActivateGhostCharacter(AGrsPawn* GhostCharacter, const ABmrPawn* FromPlayerCharacter)
{
	// --- ghost is activated by the server only, clients follow the replicated bIsGhostActive
	if (!HasAuthority()
	    || !GhostCharacter
	    || !FromPlayerCharacter)
	{
		return;
	}
	
	if (!ensureMsgf(FromPlayerCharacter->GetPlayerId() == GetPlayerID(), TEXT("ASSERT: [%i] %hs:\n'FromPlayerCharacter' belongs to another player than this ghost!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	
	const UGrsPlayerStateComponent* GrsPlayerStateComponent = GetGrsPlayerStateComponent();
	if (!GrsPlayerStateComponent
	    || !GrsPlayerStateComponent->IsRevivable())
	{
		return;
	}

	AController* PlayerController = FromPlayerCharacter->GetController();
	if (!PlayerController)
	{
		return;
	}

	// --- check if the player already possessed by other ghost
	const AGrsPawn* CurrentGhostCharacter = Cast<AGrsPawn>(PlayerController->GetPawn());
	if (CurrentGhostCharacter)
	{
		return;
	}

	SetGhostActive(true);

	// --- authority calls:
	TryPossessController(PlayerController);

	// --- set pawn location (side)
	UGrsPawnHelper::SetPawnToAvailableSide(this);
}

// Possess a player controller
void AGrsPawn::TryPossessController(AController* PlayerController)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	if (!PlayerController || !PlayerController->HasAuthority())
	{
		return;
	}

	// Unpossess current pawn first
	if (PlayerController->GetPawn())
	{
		PlayerController->UnPossess();
	}

	PlayerController->Possess(this);
}

// APawn Interface when this pawn was possessed by a new controller
void AGrsPawn::PossessedBy(AController* NewController)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	Super::PossessedBy(NewController);

	if (!UGrsPawnHelper::IsReady(this))
	{
		return;
	}

	RefreshPawn();
}

// APawn Interface when this pawn was replicated by a new controller
void AGrsPawn::OnRep_Controller()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	Super::OnRep_Controller();

	if (!UGrsPawnHelper::IsReady(this))
	{
		return;
	}

	RefreshPawn();
}

//  APawn Interface when this pawn was replicated by a new player state
void AGrsPawn::OnRep_PlayerState()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	Super::OnRep_PlayerState();

	if (!UGrsPawnHelper::IsReady(this))
	{
		return;
	}

	RefreshPawn();
}

// Overridable function called whenever this actor is being removed from a level
void AGrsPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	Super::EndPlay(EndPlayReason);

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);
	PerformCleanUp();
}

// APawn Interface when this pawn was unpossessed
void AGrsPawn::UnPossessed()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	Super::UnPossessed();

	HideGhostCharacterFromMap(); // remove on clients and server ghost from map
}

// Refresh and enable this pawn
void AGrsPawn::RefreshPawn()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	ClearTrajectorySplines();
	AimingSphereComponent->SetVisibility(true);
	PlayerArrowStartComponent->SetArrowEnabled(true);
}

// Remove ghost character from the level when clean up or ghost kills a player
void AGrsPawn::HideGhostCharacterFromMap()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// --- change visibility of this pawn
	// -- change nickname visibility of this pawn
	// --- update collision mod of this pawn if needed
	// deactivated by the server, clients hide the ghost once the state is replicated
	SetGhostActive(false);
	AimingSphereComponent->SetVisibility(false);
	PlayerArrowStartComponent->SetArrowEnabled(false);
	ClearTrajectorySplines();

	// --- free the side of the map, so another ghost can be placed there
	if (UGrsPlayerStateComponent* MyPlayerStateComponent = GetGrsPlayerStateComponent())
	{
		MyPlayerStateComponent->ResetGhostSide();
	}

	StopListeningPlayerCharacterRemoval();

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
	PlayerStateComponent.Reset();

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
 * Ghost activity
 **********************************************************************************************/

// Is called on clients when this ghost was activated or deactivated by the server
void AGrsPawn::OnRep_IsGhostActive()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (CLIENT) %s"), __LINE__, __FUNCTION__, bIsGhostActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
	ApplyGhostActiveVisuals();
}

// Activates or deactivates this ghost
void AGrsPawn::SetGhostActive(bool bNewActive)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (SERVER) %s"), __LINE__, __FUNCTION__, bNewActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
	bIsGhostActive = bNewActive;

	// Rep notify is not called on the server, so visuals are applied here directly
	ApplyGhostActiveVisuals();
}

// Applies visuals shared by all machines for the current bIsGhostActive
void AGrsPawn::ApplyGhostActiveVisuals()
{
	FGrsPawnVisualizer::SetVisibility(this, bIsGhostActive);
}

/*********************************************************************************************
 * Aiming functionality
 **********************************************************************************************/

// Initiate and activate aiming point
void AGrsPawn::InitAimingSphere()
{
	const UGRSDataAsset* const GrsDataAsset = &UGRSDataAsset::Get();
	AimingSphereComponent->SetStaticMesh(GrsDataAsset->GetProjectileMesh());
	AimingSphereComponent->SetMaterial(0, GrsDataAsset->GetAimingMaterial());
	AimingSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimingSphereComponent->SetVisibility(false);
}

// Add a new spline mesh component
void AGrsPawn::AddAimingSplineMeshComponent(USplineMeshComponent* SplineMeshComponent)
{
	AimingSplineMeshArray.AddUnique(SplineMeshComponent);
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
