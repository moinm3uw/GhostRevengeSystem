// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

#include "Data/GRSDataAsset.h"
#include "Data/MyPrimaryDataAsset.h"
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

// Register character manager component
void UGRSWorldSubSystem::RegisterCharacterManagerComponent(class UGRSGhostCharacterManagerComponent* CharacterManagerComponent)
{
	if (CharacterManagerComponent && CharacterManagerComponent != CharacterMangerComponent)
	{
		CharacterMangerComponent = CharacterManagerComponent;
	}
}

// Register ghost character
void UGRSWorldSubSystem::RegisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter)
{
	checkf(GhostPlayerCharacter, TEXT("ERROR: [%i] %hs:\n'GhostPlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (!GhostCharacterLeftSide)
	{
		GhostCharacterLeftSide = GhostPlayerCharacter;
	}
	else if (!GhostCharacterRightSide)
	{
		GhostCharacterRightSide = GhostPlayerCharacter;
	}
}

// Returns the side of the ghost character (left, or right)
EGRSCharacterSide UGRSWorldSubSystem::GetGhostPlayerCharacterSide(AGRSPlayerCharacter* PlayerCharacter)
{
	if (PlayerCharacter == nullptr)
	{
		return EGRSCharacterSide::None;
	}

	if (GhostCharacterLeftSide == PlayerCharacter)
	{
		return EGRSCharacterSide::Left;
	}

	if (GhostCharacterRightSide == PlayerCharacter)
	{
		return EGRSCharacterSide::Right;
	}

	return EGRSCharacterSide::None;
}

// Broadcasts the activation of ghost character, can be called from outside
void UGRSWorldSubSystem::ActivateGhostCharacter(APlayerCharacter* PlayerCharacter)
{
	if (!GhostCharacterLeftSide->GetController())
	{
		GhostCharacterLeftSide->ActivateCharacter(PlayerCharacter);
	}
	else if (!GhostCharacterRightSide->GetController())
	{
		GhostCharacterRightSide->ActivateCharacter(PlayerCharacter);
	}
}

// Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors
void UGRSWorldSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

// Return a ghost character.
class AGRSPlayerCharacter* UGRSWorldSubSystem::GetGhostPlayerCharacter()
{
	return nullptr;
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
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
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
