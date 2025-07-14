// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "GhostRevengeSystemComponent.h"
#include "GeneratedMap.h"
#include "PoolManagerSubsystem.h"
#include "Controllers/MyPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
#include "LevelActors/BombActor.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GhostRevengeSystemComponent)

// Sets default values for this component's properties
UGhostRevengeSystemComponent::UGhostRevengeSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Returns Player Controller of this component
APlayerController* UGhostRevengeSystemComponent::GetPlayerController() const
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	APlayerController* PlayerController = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	return PlayerController;
}

// Returns Player Controller of this component and check (crash if not valid)
APlayerController& UGhostRevengeSystemComponent::GetPlayerControllerChecked() const
{
	APlayerController* PlayerController = GetPlayerController();
	checkf(PlayerController, TEXT("%s: 'PlayerController' is null"), *FString(__FUNCTION__));
	return *PlayerController;
}

// Called when the game starts
void UGhostRevengeSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
	UMapComponent* MapComponent = UMapComponent::GetMapComponent(GetOwner());

	if (!ensureMsgf(MapComponent, TEXT("ASSERT: [%i] %hs:\n'MapComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	MapComponent->OnAddedToLevel.AddUniqueDynamic(this, &ThisClass::OnAddedToLevel);
	MapComponent->OnPostRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPostRemovedFromLevel);

	PreviousPlayerCharacterInternal = Cast<APlayerCharacter>(GetOwner());
	if (PreviousPlayerCharacterInternal)
	{
		PreviousPlayerCharacterLocationInternal = PreviousPlayerCharacterInternal->GetActorLocation();
	}
}

// Called when this level actor is reconstructed or added on the Generated Map, on both server and clients.
void UGhostRevengeSystemComponent::OnAddedToLevel_Implementation(class UMapComponent* MapComponent)
{
	PreviousPlayerCharacterInternal = Cast<APlayerCharacter>(GetOwner());
	if (PreviousPlayerCharacterInternal)
	{
		PreviousPlayerCharacterLocationInternal = PreviousPlayerCharacterInternal->GetActorLocation();
	}
}

// Called right before owner actor going to remove from the Generated Map
void UGhostRevengeSystemComponent::OnPostRemovedFromLevel_Implementation(class UMapComponent* MapComponent, UObject* DestroyCauser)
{
	// --- not in game, no ghost character spawned, no bomb spawned (not a causer)
	if (AMyGameStateBase::GetCurrentGameState() != ECurrentGameState::InGame)
	{
		return;
	}

	if (MapComponent == UMapComponent::GetMapComponent(GetOwner()))
	{
		APlayerController& PlayerController = GetPlayerControllerChecked();
		bool bHasAuth = PlayerController.HasAuthority();
		if (!bHasAuth)
		{
			return;
		}

		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
		if (PlayerCharacter)
		{
			PreviousPlayerControllerInternal = Cast<AMyPlayerController>(PlayerCharacter->Controller);
		}
		AddGhostCharacter();
	}
}

// Listen game states to switch character skin.
void UGhostRevengeSystemComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::Menu:
		{
			UMapComponent* MapComponent = UMapComponent::GetMapComponent(GetOwner());
			if (!ensureMsgf(MapComponent, TEXT("ASSERT: [%i] %hs:\n'MapComponent' is not valid!"), __LINE__, __FUNCTION__))
			{
				return;
			}

			RemoveGhostCharacterFromMap();
		}
		break;
	default: break;
	}
}

// Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.) 
void UGhostRevengeSystemComponent::OnEndGameStateChanged_Implementation(EEndGameState EndGameState)
{
	class UHUDWidget* HUD = nullptr;

	switch (EndGameState)
	{
	case EEndGameState::Lose:
		{
			/*
		//HUD = UWidgetsSubsystem::Get().GetWidgetByTag();
		if (!ensureMsgf(HUD, TEXT("ASSERT: [%i] %hs:\n'HUD' is not valid!"), __LINE__, __FUNCTION__))
		{
			break;
		}
		HUD->SetVisibility(ESlateVisibility::Collapsed);
		PlayerStateInternal->SetCharacterDead(false);
		PlayerStateInternal->SetOpponentKilledNum(0);
		PlayerStateInternal->SetEndGameState(EEndGameState::None);
		*/
		}


		break;

	default: break;
	}
}

// Add ghost character to the current active game (on level map)
void UGhostRevengeSystemComponent::AddGhostCharacter_Implementation()
{
	APlayerController& PlayerController = GetPlayerControllerChecked();

	// --- Only server can spawn character and posses it
	if (!PlayerController.HasAuthority())
	{
		return;
	}

	bool bHasAuthority = GetOwner()->HasAuthority();
	UE_LOG(LogTemp, Warning, TEXT("AddGhostCharacter Has authority: %i"), bHasAuthority);

	// --- Return to Pool Manager the list of handles which is not needed (if there are any) 
	if (!PoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
		PoolActorHandlersInternal.Empty();
	}

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGhostRevengeSystemComponent* This = WeakThis.Get())
		{
			This->OnTakeActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(PoolActorHandlersInternal, AGRSPlayerCharacter::StaticClass(), 1, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGhostRevengeSystemComponent::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// --- something wrong if there are more than 1 object found
	if (CreatedObjects.Num() > 1)
	{
		return;
	}

	bool bHasAuthority = GetOwner()->HasAuthority();
	UE_LOG(LogTemp, Warning, TEXT("OnTakeActorsFromPoolCompleted Has authority: %i"), bHasAuthority);

	// --- Setup spawned widget
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGRSPlayerCharacter& Character = CreatedObject.GetChecked<AGRSPlayerCharacter>();
		SpawnGhost(&Character);
	}
}

// Adds ghost character to the level
void UGhostRevengeSystemComponent::SpawnGhost(AGRSPlayerCharacter* GhostPlayerCharacter)
{
	APlayerController& PlayerController = GetPlayerControllerChecked();

	// --- Only server can spawn character and posses it
	if (!PlayerController.HasAuthority())
	{
		return;
	}

	// --- needed for input action binding at the moment
	UGRSWorldSubSystem::Get().RegisterGhostPlayerCharacter(GhostPlayerCharacter);

	FCell ActorSpawnLocation;
	float CellSize = FCell::CellSize + (FCell::CellSize / 2);
	int32 PlayerId = PreviousPlayerCharacterInternal->GetPlayerState()->GetPlayerId();

	if (PlayerId == 0 || PlayerId == 2)
	{
		ActorSpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopLeft);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.Y = PreviousPlayerCharacterLocationInternal.Y;
		ActorSpawnLocation.Location.Z = PreviousPlayerCharacterLocationInternal.Z;
	}
	else
	{
		ActorSpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopRight);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.Y = PreviousPlayerCharacterLocationInternal.Y;
		ActorSpawnLocation.Location.Z = PreviousPlayerCharacterLocationInternal.Z;
	}

	// --- Possess the ghost character
	// comment to be removed if all works
	//PlayerController.Possess(GhostPlayerCharacter);

	// --- Enables ghost character input (Input Manage Context)
	//SetManagedInputContextEnabled(true);

	// --- Update ghost character 
	FVector SpawnLocation = ActorSpawnLocation;
	GhostPlayerCharacter->SetActorLocation(SpawnLocation);
	GhostPlayerCharacter->SetVisibility(true);
	GhostPlayerCharacter->Initialize(&PlayerController);
	GhostPlayerCharacter->OnGhostPlayerKilled.AddUniqueDynamic(this, &ThisClass::OnGhostPlayerKilled);
	PlayerController.ResetIgnoreMoveInput();

	bool bHasAuthority = GetOwner()->HasAuthority();
	UE_LOG(LogTemp, Warning, TEXT("SpawnGhost Has authority: %i"), bHasAuthority);
}

// Called when the ghost player killed
void UGhostRevengeSystemComponent::OnGhostPlayerKilled()
{
	RemoveGhostCharacterFromMap();
}

// Enables or disables the input context
void UGhostRevengeSystemComponent::SetManagedInputContextEnabled(bool bEnable)
{
	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(&GetPlayerControllerChecked());

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

// Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern)
void UGhostRevengeSystemComponent::RemoveGhostCharacterFromMap()
{
	// --- no ghost character, could be null
	if (!UGRSWorldSubSystem::Get().GetGhostPlayerCharacter())
	{
		return;
	}

	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// --- Disable ghost character input
	SetManagedInputContextEnabled(false);

	// --- Posses to play character  
	AController* GhostPlayerController = UGRSWorldSubSystem::Get().GetGhostPlayerCharacter()->Controller;
	if (!GhostPlayerController || !GhostPlayerController->HasAuthority() || !PreviousPlayerCharacterInternal)
	{
		return;
	}

	GhostPlayerController->Possess(PreviousPlayerCharacterInternal);

	// --- Hide from level
	//UGRSWorldSubSystem::Get().GetGhostPlayerCharacter()->SetVisibility(false);

	// --- Return to pool character
	if (!PoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
		PoolActorHandlersInternal.Empty();
	}

	// --- reset ghost character as removed
	UGRSWorldSubSystem::Get().RegisterGhostPlayerCharacter(nullptr);
}
