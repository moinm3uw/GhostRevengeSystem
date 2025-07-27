// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

#include "GhostRevengeSystemComponent.h"
#include "PoolManagerSubsystem.h"
#include "Data/MyPrimaryDataAsset.h"
#include "Data/GRSDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/MyGameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "LevelActors/PlayerCharacter.h"
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

	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSWorldSubSystem::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	OnInitialize.Broadcast();
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::Menu:
		{
			// whenever user returns to main menu all ghost character to be removed from level (cleanup)
			RemoveGhostCharacterFromMap();
			break;
		}

	default: break;
	}
}

// Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern)
void UGRSWorldSubSystem::RemoveGhostCharacterFromMap()
{
	// --- no ghost character, could be null
	if (!GhostPlayerCharacterInternal)
	{
		return;
	}

	if (!ensureMsgf(MainPlayerCharacterInternal, TEXT("ASSERT: [%i] %hs:\n'MainPlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	GhostPlayerCharacterInternal->UnPossessController();

	// --- Return to pool character
	if (!PoolActorHandlersInternal.IsEmpty())
	{
		UPoolManagerSubsystem::Get().ReturnToPoolArray(PoolActorHandlersInternal);
		PoolActorHandlersInternal.Empty();
	}

	GhostPlayerCharacterInternal = nullptr;
}

// Register main player character from ghost revenge system spot component
void UGRSWorldSubSystem::RegisterMainCharacter(APlayerCharacter* PlayerCharacter)
{
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__) 
		|| !PlayerCharacter->IsPlayerControlled())
	{
		return;
	}

	// store locally controller player data 
	if (PlayerCharacter->IsLocallyControlled())
	{
		MainPlayerCharacterInternal = PlayerCharacter;
		MainPlayerCharacterSpawnLocationInternal = PlayerCharacter->GetActorLocation();
	}

	if (PlayerCharacter->HasAuthority())
	{
		MainPlayerCharacterArrayInternal.Add(PlayerCharacter);
	}
}

//  When a main player character was removed from level spawn a new ghost character
void UGRSWorldSubSystem::MainCharacterRemovedFromLevel(UMapComponent* MapComponent)
{
	// spawn ghost character
	if (!MapComponent)
	{
		return;
	}

	for (TObjectPtr PlayerCharacter : MainPlayerCharacterArrayInternal)
	{
		if (MapComponent == UMapComponent::GetMapComponent(PlayerCharacter))
		{
			APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->Controller);
			bool bHasAuth = PlayerController->HasAuthority();
			if (!bHasAuth)
			{
				return;
			}

			// --- Spawn ghost character
			AddGhostCharacter();
		}
	}
}

// Add ghost character to the current active game (on level map)
void UGRSWorldSubSystem::AddGhostCharacter_Implementation()
{
	APlayerController* PlayerController = Cast<APlayerController>(MainPlayerCharacterInternal->GetController());

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
		if (UGRSWorldSubSystem* This = WeakThis.Get())
		{
			This->OnTakeActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(PoolActorHandlersInternal, UGRSWorldSubSystem::StaticClass(), 1, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGRSWorldSubSystem::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// --- something wrong if there are more than 1 object found
	if (CreatedObjects.Num() > 1)
	{
		return;
	}

	// --- Setup spawned widget
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGRSPlayerCharacter& GhostCharacter = CreatedObject.GetChecked<AGRSPlayerCharacter>();
		GhostPlayerCharacterInternal = GhostCharacter;
		GhostPlayerCharacterInternal->OnGhostPlayerKilled.AddUniqueDynamic(this, &ThisClass::OnGhostEliminatesPlayer);
	}
}

// Called when the ghost player kills another player and will be swaped with him
void UGRSWorldSubSystem::OnGhostEliminatesPlayer()
{
	RemoveGhostCharacterFromMap();
}

APlayerCharacter* UGRSWorldSubSystem::GetMainPlayerCharacter()
{
	return MainPlayerCharacterInternal ? MainPlayerCharacterInternal : nullptr;
}


/** EngGameState:
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
