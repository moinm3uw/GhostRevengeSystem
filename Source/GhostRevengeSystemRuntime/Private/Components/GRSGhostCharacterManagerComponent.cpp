// Copyright (c) Yevhenii Selivanov


#include "Components/GRSGhostCharacterManagerComponent.h"

#include "Bomber.h"
#include "PoolManagerSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/MyGameStateBase.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"

#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"


// Sets default values for this component's properties
UGRSGhostCharacterManagerComponent::UGRSGhostCharacterManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UGRSGhostCharacterManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
	UGRSWorldSubSystem::Get().OnMainCharacterRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnMainCharacterRemovedFromLevel);
}

// Listen game states to remove ghost character from level
void UGRSGhostCharacterManagerComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::Menu:
		{
			// whenever user returns to main menu all ghost character to be removed from level (cleanup)
			RemoveGhostCharacterFromMap();
			break;
		}
	case ECurrentGameState::GameStarting:
		{
			// --- Only server can spawn character and posses it
			if (GetOwner()->HasAuthority())
			{
				AddGhostCharacter();
			}
			break;
		}
	default: break;
	}
}

// Remove (hide) ghost character from the level. Hides and return character to pool manager (object pooling pattern)
void UGRSGhostCharacterManagerComponent::RemoveGhostCharacterFromMap()
{
	// --- no ghost character, could be null
	AGRSPlayerCharacter* GhostPlayerCharacterInternal = UGRSWorldSubSystem::Get().GetGhostPlayerCharacter();
	if (!GhostPlayerCharacterInternal)
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

	// --- Reset current ghost character
	UGRSWorldSubSystem::Get().ResetGhostCharacter();
}

// Called when the ghost player kills another player and will be swaped with him
void UGRSGhostCharacterManagerComponent::OnGhostEliminatesPlayer()
{
	RemoveGhostCharacterFromMap();
}

// Whenever a main player character is removed from level
void UGRSGhostCharacterManagerComponent::OnMainCharacterRemovedFromLevel_Implementation()
{
	// take over controlls
}

// Add ghost character to the current active game (on level map)
void UGRSGhostCharacterManagerComponent::AddGhostCharacter()
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
		if (UGRSGhostCharacterManagerComponent* This = WeakThis.Get())
		{
			This->OnTakeActorsFromPoolCompleted(CreatedObjects);
		}
	};

	// --- Spawn actor
	UPoolManagerSubsystem::Get().TakeFromPoolArray(PoolActorHandlersInternal, AGRSPlayerCharacter::StaticClass(), 2, OnTakeActorsFromPoolCompleted, ESpawnRequestPriority::High);
}

// Grabs a Ghost Revenge Player Character from the pool manager (Object pooling patter)
void UGRSGhostCharacterManagerComponent::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// --- something wrong if there are less than 1 object found
	if (CreatedObjects.Num() < 1)
	{
		return;
	}

	// --- Setup spawned widget
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGRSPlayerCharacter& GhostCharacter = CreatedObject.GetChecked<AGRSPlayerCharacter>();
		GhostCharacter.Initialize();
		//GhostCharacter.OnGhostPlayerKilled.AddUniqueDynamic(this, &ThisClass::OnGhostEliminatesPlayer);
	}
}
