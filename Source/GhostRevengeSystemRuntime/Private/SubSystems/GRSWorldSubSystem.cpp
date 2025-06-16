// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

#include "GeneratedMap.h"
#include "PoolManagerSubsystem.h"
#include "Controllers/MyPlayerController.h"
#include "Data/MyPrimaryDataAsset.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/GeneratedMapDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSWorldSubSystem)

// Returns this Subsystem, is checked and will crash if it can't be obtained
UGRSWorldSubSystem& UGRSWorldSubSystem::Get()
{
	const UWorld* World = UUtilsLibrary::GetPlayWorld();
	checkf(World, TEXT("%s: 'World' is null"), *FString(__FUNCTION__));
	UGRSWorldSubSystem* ThisSubsystem = World->GetSubsystem<ThisClass>();
	checkf(ThisSubsystem, TEXT("%s: 'ProgressionSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

// Returns this Subsystem, is checked and will crash if it can't be obtained
UGRSWorldSubSystem& UGRSWorldSubSystem::Get(const UObject& WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObjectChecked(&WorldContextObject);
	checkf(World, TEXT("%s: 'World' is null"), *FString(__FUNCTION__));
	UGRSWorldSubSystem* ThisSubsystem = World->GetSubsystem<ThisClass>();
	checkf(ThisSubsystem, TEXT("%s: 'ProgressionSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

// Returns the data asset that contains all the assets of Ghost Revenge System game feature
const UGRSDataAsset* UGRSWorldSubSystem::GetGRSDataAsset() const
{
	return UMyPrimaryDataAsset::GetOrLoadOnce(DataAssetInternal);
}

// Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors
void UGRSWorldSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::Menu:
		{
			RemoveGhostCharacterFromMap();
			RemoveMapCollisionOnSide();
		}
		break;
	case ECurrentGameState::GameStarting:
		{
			OnInitialize.Broadcast();
			OnInit();
			break;
		}
	case ECurrentGameState::InGame:
		{
			// --- spawn collisions box on the sides of the map
			SpawnMapCollisionOnSide();
			break;
		}

	default: break;
	}
}

// Called when the ghost revenge system is ready loaded (when game transitions to ingame state) 
void UGRSWorldSubSystem::OnInit_Implementation()
{
	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

// Called when the local player character is spawned, possessed, and replicated.
void UGRSWorldSubSystem::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	// Binds the local player state ready event to the handler
	BIND_ON_LOCAL_PLAYER_STATE_READY(this, ThisClass::OnLocalPlayerStateReady);
	LocalPlayerCharacterInternal = PlayerCharacter;
}

// Subscribes to the end game state change notification on the player state.
void UGRSWorldSubSystem::OnLocalPlayerStateReady_Implementation(class AMyPlayerState* PlayerState, int32 CharacterID)
{
	checkf(PlayerState, TEXT("ERROR: [%i] %hs:\n'PlayerState' is null!"), __LINE__, __FUNCTION__);

	/*
	if (UMapComponent* MapComponent = UMapComponent::GetMapComponent(PlayerCharacterInternal))
	{
		MapComponent->SetReplicatedMeshData(MySkeletalMeshComponentInternal->GetMeshData());
	}
	*/
	PlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);
}

// Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.) 
void UGRSWorldSubSystem::OnEndGameStateChanged_Implementation(EEndGameState EndGameState)
{
	class UHUDWidget* HUD = nullptr;

	switch (EndGameState)
	{
	case EEndGameState::Lose:
		{
			AddGhostCharacter();
		}

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
		break;

	default: break;
	}
}

// Add ghost character to the current active game (on level map)
void UGRSWorldSubSystem::AddGhostCharacter()
{
	// --- spawn collisions box on the sides of the map
	//SpawnMapCollisionOnSide();

	// --- take from pool and spawn character to level
	SpawnGhostCharacter();
}

//  Spawn a collision box the side of the map
void UGRSWorldSubSystem::SpawnMapCollisionOnSide()
{
	// --- Return to Pool Manager the list of handles which is not needed (if there are any) 
	/*
	if (!CollisionPoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
		CollisionPoolActorHandlersInternal.Empty();
	}
	*/

	// --- Prepare spawn request
	const TWeakObjectPtr<ThisClass> WeakThis = this;
	const FOnSpawnAllCallback OnTakeActorsFromPoolCompleted = [WeakThis](const TArray<FPoolObjectData>& CreatedObjects)
	{
		if (UGRSWorldSubSystem* This = WeakThis.Get())
		{
			This->OnTakeCollisionActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(CollisionPoolActorHandlersInternal, UGeneratedMapDataAsset::Get().GetCollisionsAssetClass(), 1, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Remove a collision box the sides of the map
void UGRSWorldSubSystem::RemoveMapCollisionOnSide()
{
	// --- Return to pool character
	if (!CollisionPoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(CollisionPoolActorHandlersInternal);
	}
}

void UGRSWorldSubSystem::OnTakeCollisionActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	APlayerCharacter* PlayerCharacter = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	UMapComponent* MapComponent = UMapComponent::GetMapComponent(PlayerCharacter);
	if (!ensureMsgf(MapComponent, TEXT("ASSERT: [%i] %hs:\n'PlayerMapComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Spawn side collision
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AActor& SpawnCollision = CreatedObject.GetChecked<AActor>();
		int32 playerId = PlayerCharacter->GetPlayerId();

		FCell FirstCell = MapComponent->GetCell();
		if (playerId == 0 || playerId == 3)
		{
			FirstCell.Location.X = FirstCell.Location.X - FCell::CellSize;
			LeftSideCollisionInternal = SpawnCollision;
		}
		if (playerId == 1 || playerId == 2)
		{
			FirstCell.Location.X = FirstCell.Location.X + FCell::CellSize;
			RightSideCollisionInternal = SpawnCollision;
		}

		SpawnCollision.SetActorTransform(UGRSDataAsset::Get().GetCollisionTransform());
		SpawnCollision.SetActorLocation(FVector(FirstCell.Location.X, 0, 0));
	}
}

// Take from pool manager and spawn a ghost character to the current active game (on level map)
void UGRSWorldSubSystem::SpawnGhostCharacter()
{
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
		if (UGRSWorldSubSystem* This = WeakThis.Get())
		{
			This->OnTakeActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(PoolActorHandlersInternal, AGRSPlayerCharacter::StaticClass(), 1, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGRSWorldSubSystem::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	APlayerCharacter* PlayerCharacter = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter();
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	AMyPlayerController* PC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	if (!ensureMsgf(PC, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is null!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// something wrong if there are more than 1 object found
	if (CreatedObjects.Num() > 1)
	{
		return;
	}

	// Setup spawned widget
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		GhostPlayerCharacterInternal = CreatedObject.GetChecked<AGRSPlayerCharacter>();

		int32 playerId = PlayerCharacter->GetPlayerId();
		FVector NewLocation = GhostPlayerCharacterInternal->GetActorLocation();
		if (playerId == 0 || playerId == 3)
		{
			NewLocation.X = LeftSideCollisionInternal->GetActorLocation().X;
		}
		if (playerId == 1 || playerId == 2)
		{
			NewLocation.X = RightSideCollisionInternal->GetActorLocation().X;
		}

		// --- Possess the ghost character
		if (GhostPlayerCharacterInternal->HasAuthority())
		{
			PC->Possess(GhostPlayerCharacterInternal);
		}
		else
		{
			GhostPlayerCharacterInternal->ServerRequestPossess_Implementation(GhostPlayerCharacterInternal);
		}

		// --- Enables ghost character input (Input Manage Context) 
		SetManagedInputContextEnabled(true);

		// --- Update ghost character 
		FVector SpawnLocation = UGRSDataAsset::Get().GetSpawnLocation();
		SpawnLocation.X = NewLocation.X;
		GhostPlayerCharacterInternal->SetActorLocation(SpawnLocation);
		GhostPlayerCharacterInternal->SetVisibility(true);

		GhostPlayerCharacterInternal->SetActorLocation(SpawnLocation);
	}
}

// Enables or disables the input context
void UGRSWorldSubSystem::SetManagedInputContextEnabled(bool bEnable)
{
	AMyPlayerController* PC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();

	TArray<const UMyInputMappingContext*> InputContexts;
	UMyInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
	InputContexts.AddUnique(InputContext);

	if (!bEnable)
	{
		// Remove related input contexts
		PC->RemoveInputContexts(InputContexts);
		return;
	}

	// Remove all previous input contexts managed by Controller
	PC->RemoveInputContexts(InputContexts);

	// Add gameplay context as auto managed by Game State, so it will be enabled everytime the game is in the in-game state
	if (InputContext
		&& InputContext->GetChosenGameStatesBitmask() > 0)
	{
		PC->SetupInputContexts(InputContexts);
	}
}

// Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern)
void UGRSWorldSubSystem::RemoveGhostCharacterFromMap()
{
	AMyPlayerController* PC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	if (!ensureMsgf(PC, TEXT("ASSERT: [%i] %hs:\n'PlayerController' is null!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// coild be null
	if (!GhostPlayerCharacterInternal)
	{
		return;
	}

	// --- Disable ghost character input
	SetManagedInputContextEnabled(false);

	// --- Posses to play character  
	PC->Possess(LocalPlayerCharacterInternal);

	// --- Hide from level
	GhostPlayerCharacterInternal->SetVisibility(false);

	// --- Return to pool character
	if (!PoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
	}
}

// Register the ghost revenge system spot component
void UGRSWorldSubSystem::RegisterSpotComponent(UGhostRevengeSystemSpotComponent* SpotComponent)
{
	if (!ensureMsgf(SpotComponent, TEXT("ASSERT: [%i] %hs:\n'SpotComponent' is null!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	SpotComponentInternal = SpotComponent;
}
