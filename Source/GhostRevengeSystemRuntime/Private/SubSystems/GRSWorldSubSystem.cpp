// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

#include "GhostRevengeSystemComponent.h"
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

// Add spawned collision actors to be cached
void UGRSWorldSubSystem::AddCollisionActor(class AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	if (!LeftSideCollisionInternal)
	{
		LeftSideCollisionInternal = Actor;
	}
	else
	{
		RightSideCollisionInternal = Actor;
	}
}

// Returns TRUE if collision are spawned
bool UGRSWorldSubSystem::IsCollisionSpawned()
{
	if (LeftSideCollisionInternal && RightSideCollisionInternal)
	{
		return true;
	}

	return false;
}

// Returns left side spawned collision or nullptr
AActor* UGRSWorldSubSystem::GetLeftCollisionActor()
{
	if (!IsCollisionSpawned())
	{
		return nullptr;
	}

	return LeftSideCollisionInternal;
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSWorldSubSystem::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	OnInitialize.Broadcast();
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState) {}

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

			OnMainCharacterRemovedFromLevel.Broadcast();
		}
	}
}

// Register ghost player character spawned
void UGRSWorldSubSystem::RegisterGhostCharacter(AGRSPlayerCharacter* PlayerCharacter)
{
	if (PlayerCharacter)
	{
		GhostPlayerCharacterInternal = PlayerCharacter;
	}
}

// Reset ghost player character
void UGRSWorldSubSystem::ResetGhostCharacter()
{
	if (GhostPlayerCharacterInternal)
	{
		GhostPlayerCharacterInternal = nullptr;
	}
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
