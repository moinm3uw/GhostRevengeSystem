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

	APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController());
	return PlayerController;
}

// Returns Player Controller of this component and check (crash if not valid)
APlayerController* UGhostRevengeSystemComponent::GetPlayerControllerChecked() const
{
	APlayerController* PlayerController = GetPlayerController();
	checkf(PlayerController, TEXT("%s: 'PlayerController' is null"), *FString(__FUNCTION__));
	return PlayerController;
}

// Called when the game starts
void UGhostRevengeSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerCharacter* MainPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!MainPlayerCharacter)
	{
		return;
	}

	UGRSWorldSubSystem::Get().RegisterMainPlayerCharacter(MainPlayerCharacter);

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
}

// Listen game states to switch character skin.
void UGhostRevengeSystemComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::Menu:
		{
			// whenever user returns to main menu all ghost character to be removed from level (cleanup)
			RemoveGhostCharacterFromMap();
		}
		break;
	default: break;
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

	// --- Posses to play character
	AController* GhostPlayerController = UGRSWorldSubSystem::Get().GetGhostPlayerCharacter()->Controller;
	APlayerCharacter* MainPlayerCharacter = UGRSWorldSubSystem::Get().GetMainPlayerCharacter();
	if (!GhostPlayerController || !GhostPlayerController->HasAuthority() || !MainPlayerCharacter)
	{
		return;
	}

	GhostPlayerController->Possess(MainPlayerCharacter);

	// --- Return to pool character
	if (!PoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
		PoolActorHandlersInternal.Empty();
	}

	// --- reset ghost character as removed
	UGRSWorldSubSystem::Get().RegisterGhostPlayerCharacter(nullptr);
}

// Called when this level actor is reconstructed or added on the Generated Map, on both server and clients.
void UGhostRevengeSystemComponent::OnAddedToLevel_Implementation(class UMapComponent* MapComponent)
{
	APlayerCharacter* MainPlayerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!MainPlayerCharacter)
	{
		return;
	}
	UGRSWorldSubSystem::Get().RegisterMainPlayerCharacter(MainPlayerCharacter);
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
		APlayerController* PlayerController = GetPlayerControllerChecked();
		bool bHasAuth = PlayerController->HasAuthority();
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

// Add ghost character to the current active game (on level map)
void UGhostRevengeSystemComponent::AddGhostCharacter_Implementation()
{
	APlayerController* PlayerController = GetPlayerControllerChecked();

	// --- Only server can spawn character and posses it
	if (!PlayerController->HasAuthority())
	{
		return;
	}

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

	// --- Setup spawned widget
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGRSPlayerCharacter* Character = &CreatedObject.GetChecked<AGRSPlayerCharacter>();
		SpawnGhost(Character);
	}
}

// Adds ghost character to the level
void UGhostRevengeSystemComponent::SpawnGhost(AGRSPlayerCharacter* GhostPlayerCharacter)
{
	APlayerController* PlayerController = GetPlayerControllerChecked();

	// --- Only server can spawn character and posses it
	if (!PlayerController->HasAuthority())
	{
		return;
	}

	FCell ActorSpawnLocation;
	float CellSize = FCell::CellSize + (FCell::CellSize / 2);

	APlayerCharacter* MainPlayerCharacter = UGRSWorldSubSystem::Get().GetMainPlayerCharacter();
	if (!MainPlayerCharacter)
	{
		return;
	}

	int32 PlayerId = MainPlayerCharacter->GetPlayerId();

	if (PlayerId == 0 || PlayerId == 2)
	{
		//ActorSpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopLeft);
		//ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.X = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().X - CellSize;
		ActorSpawnLocation.Location.Y = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().Y;
		ActorSpawnLocation.Location.Z = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().Z;
	}
	else
	{
		//ActorSpawnLocation = UCellsUtilsLibrary::GetCellByCornerOnLevel(EGridCorner::TopRight);
		//ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.X = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().X + CellSize;
		ActorSpawnLocation.Location.Y = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().Y;
		ActorSpawnLocation.Location.Z = UGRSWorldSubSystem::Get().GetMainPlayerCharacterSpawnLocation().Z;
	}

	// --- Update ghost character 
	FVector SpawnLocation = ActorSpawnLocation;
	GhostPlayerCharacter->SetActorLocation(SpawnLocation);
	GhostPlayerCharacter->SetVisibility(true);
	GhostPlayerCharacter->OnGhostPlayerKilled.AddUniqueDynamic(this, &ThisClass::OnGhostPlayerKilled);
	PlayerController->ResetIgnoreMoveInput();

	// --- Possess the ghost character
	// comment to be removed if all works
	if (PlayerController)
	{
		// Unpossess current pawn first
		if (PlayerController->GetPawn())
		{
			PlayerController->UnPossess();
		}
	}

	PlayerController->Possess(GhostPlayerCharacter);
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

// Called when the ghost player killed
void UGhostRevengeSystemComponent::OnGhostPlayerKilled()
{
	RemoveGhostCharacterFromMap();
}
