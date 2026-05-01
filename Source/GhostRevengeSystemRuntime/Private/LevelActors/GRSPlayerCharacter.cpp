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
#include "Engine/StaticMesh.h"
#include "GameFramework/BmrPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawnSubobjects/GrsPawnSubobject.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UI/Widgets/BmrPlayerNameWidget.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "Utils/GrsPawnHelper.h"

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* AGRSPlayerCharacter::GetAbilitySystemComponent() const
{
	const ABmrPlayerState* InPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->GetCurrentPlayerStateChecked();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

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
	FGrsPawnVisualizer::InitializeSkeletalMesh(this);

	// --- Configure the movement component
	FGrsPawnVisualizer::MovementComponentConfiguration(this);

	// --- Setup capsule component
	FGrsPawnVisualizer::InitCapsuleComponent(this);

	PlayerNickName3DWidgetComponent.SetupWidget(this); // --- Initialize 3D widget component for the player name
	ArrowStartWidgetComponent.InitArrowStartWidgetComponent(this); // --- Initialize 3D player arrow widget component that appears on top of character when player start to control it
	AimingComponent.SetupSplineComponent(this); // --- Initial setup of spline component and aiming sphere
}

// Returns properties that are replicated for the lifetime of the actor channel
void AGRSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PlayerID, Params);
}

// Called on client when player ID is changed
void AGRSPlayerCharacter::OnRep_PlayerID()
{
	// --- Init Grs Pawn logic
	InitPawn(PlayerID);
}

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

	AimingComponent.InitAimingSphere();

	// --- bind to  clear ghost data
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	// --- default params required for the fist start to have character prepared
	FGrsPawnVisualizer::InitPlayerMesh(this); // --- default init of mesh
	FGrsPawnVisualizer::InitCharacterVisual(this); // --- set character visuals (mesh, animation, skin)
	FGrsPawnVisualizer::SetVisibility(this, false); // -- hidden by default
	FGrsPawnVisualizer::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	PlayerNickName3DWidgetComponent.InitializePlayerNameWidget(this); // somehow should be hidden as well.
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
		UGRSWorldSubSystem::Get().GetPlayerStateComponent(PlayerID)->RevivePlayerCharacter(PlayerCharacter);
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

	FGrsPawnVisualizer::GetMeshChecked(this)->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	FGrsPawnVisualizer::SetVisibility(this, true);

	// --- clients calls:
	// --- update collision settings
	// --- activate arrow

	// --- just refresh visibility of player name needed, to be changed. Player name to be set by default
	// ABmrPlayerState* BmrPlayerState = Cast<ABmrPlayerState>(FromPlayerCharacter->GetPlayerState());
	// UpdatePlayerName(BmrPlayerState);

	// --- authority calls:
	TryPossessController(PlayerController);

	// --- set pawn location (side)
	UGrsPawnHelper::SetPawnToAvailableSide(this);
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

// Overridable function called whenever this actor is being removed from a level
void AGRSPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);
	PerformCleanUp();
}

// APawn Interface when this pawn was unpossessed
void AGRSPlayerCharacter::UnPossessed()
{
	Super::UnPossessed();

	FGrsPawnVisualizer::SetVisibility(this, false);
	ArrowStartWidgetComponent.SetArrowEnabled(false);
}

// APawn Interface when this pawn was possessed by a new controller
void AGRSPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	RefreshPawn();
}

// APawn Interface when this pawn was replicated by a new controller
void AGRSPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	RefreshPawn();
}

//  APawn Interface when this pawn was replicated by a new player state
void AGRSPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!UGrsPawnHelper::bIsReady(this))
	{
		return;
	}

	RefreshPawn();
}

// Refresh and enable this pawn
void AGRSPlayerCharacter::RefreshPawn()
{
	GetMesh()->SetVisibility(true, true);
	AimingComponent.ClearTrajectorySplines();
	ArrowStartWidgetComponent.SetArrowEnabled(true);
	AimingComponent.AimingSphereComponent->SetVisibility(true);
}

// Remove ghost character from the level
void AGRSPlayerCharacter::RemoveGhostCharacterFromMap()
{
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- RemoveGhostCharacterFromMap Started"), __LINE__, __FUNCTION__);
	// --- move all functional part such as posses to ability
	// --- possess back to player character for any cases
	// PlayerCharacter->GetMeshComponentChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	// --- disable aiming sphere component
	AimingComponent.AimingSphereComponent->SetVisibility(false);

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

//  Clean up the character for the MGF unload
void AGRSPlayerCharacter::PerformCleanUp()
{
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PerformCleanUp Started"), __LINE__, __FUNCTION__);
	RemoveGhostCharacterFromMap();

	AimingComponent.PerformCleanUp();

	OwningPawnComponent = nullptr;
	PlayerID = 0;

	// --- perform clean up from subsystem MGF is not possible so we have to call directly to clean cached references
	UGRSWorldSubSystem::Get().UnregisterGhostCharacter(this);
	UGRSWorldSubSystem::Get().ResetRevivedPlayers();
}

/*********************************************************************************************
 * Aiming functionality
 **********************************************************************************************/

// Add a mesh to the last element of the predict Projectile path results
void AGRSPlayerCharacter::AddMeshToEndProjectilePath(FVector Location)
{
	AimingComponent.AddMeshToEndOfProjectedPath(Location);
}

// Add spline points to the spline component
void AGRSPlayerCharacter::AddSplinePoints(FPredictProjectilePathResult& Result)
{
	AimingComponent.AddSplinePoints(Result);
}

//  Add spline mesh to spline points
void AGRSPlayerCharacter::AddSplineMesh(FPredictProjectilePathResult& Result)
{
	AimingComponent.AddSplineMesh(Result, this);
}

// Throw projectile event, bound to onetime button press
void AGRSPlayerCharacter::ThrowProjectile()
{
	AimingComponent.ThrowProjectile(this);
}
