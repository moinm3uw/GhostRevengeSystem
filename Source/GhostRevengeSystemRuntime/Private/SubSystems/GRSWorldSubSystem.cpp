// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

// GRS
#include "Data/GRSDataAsset.h"
#include "LevelActors/GRSPlayerCharacter.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Data/MyPrimaryDataAsset.h"
#include "GameFramework/BmrGameState.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/BmrGameplayMessageSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// UE
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Structures/BmrGameStateTag.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSWorldSubSystem)

/*********************************************************************************************
 * Subsystem's Lifecycle
 **********************************************************************************************/

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

// Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors
void UGRSWorldSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

// Is called to initialize the world subsystem. It's a BeginPlay logic for the GRS module
void UGRSWorldSubSystem::OnWorldSubSystemInitialize_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("UGRSWorldSubSystem BeginPlay OnWorldSubSystemInitialize_Implementation --- %s"), *this->GetName());
	BIND_ON_LOCAL_PAWN_READY(this, ThisClass::OnLocalPawnReady);
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSWorldSubSystem::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogTemp, Log, TEXT("UGRSWorldSubSystem::OnLocalCharacterReady_Implementation  --- %s"), *this->GetName());

	TryInit(); // try to initialize

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Checks if all components present and invokes initialization
void UGRSWorldSubSystem::TryInit()
{
	// --- check if managers have characters and collisions spawned if not - broadcast, yes -> return
	if (CharacterManagerComponent && CollisionMangerComponent)
	{
		if (OnInitialize.IsBound())
		{
			OnInitialize.Broadcast();
			OnInitialize.Clear();
		}
	}
}

// Clears all transient data created by this subsystem.
void UGRSWorldSubSystem::Deinitialize()
{
	PerformCleanUp();
	Super::Deinitialize();
}

// Cleanup used on unloading module to remove properties that should not be available by other objects.
void UGRSWorldSubSystem::PerformCleanUp()
{
	if (OnInitialize.IsBound())
	{
		OnInitialize.Clear();
	}

	UMyPrimaryDataAsset::ResetDataAsset(DataAssetInternal);

	UnregisterCharacterManagerComponent();
	UnregisterCollisionManagerComponent();
	ClearGhostCharacters();
	ClearCollisions();
}

/*********************************************************************************************
 * Data asset
 **********************************************************************************************/
// Returns the data asset that contains all the assets of Ghost Revenge System game feature
const UGRSDataAsset* UGRSWorldSubSystem::GetGRSDataAsset() const
{
	return UMyPrimaryDataAsset::GetOrLoadOnce(DataAssetInternal);
}

/*********************************************************************************************
 * Side Collisions actors
 **********************************************************************************************/

// Register collision manager component used to track if all components loaded and MGF ready to initialize
void UGRSWorldSubSystem::RegisterCollisionManagerComponent(UGhostRevengeCollisionComponent* NewCollisionManagerComponent)
{
	if (NewCollisionManagerComponent && NewCollisionManagerComponent != CollisionMangerComponent)
	{
		CollisionMangerComponent = NewCollisionManagerComponent;
	}

	TryInit(); // try to initialize
}

// Add spawned collision actors to be cached
void UGRSWorldSubSystem::AddCollisionActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	if (!LeftSideCollision)
	{
		LeftSideCollision = Actor;
	}
	else if (!RightSideCollision)
	{
		RightSideCollision = Actor;
	}
}

// Returns TRUE if collision are spawned
bool UGRSWorldSubSystem::IsCollisionsSpawned()
{
	if (LeftSideCollision && RightSideCollision)
	{
		return true;
	}

	return false;
}

// Clears cached collision manager component
void UGRSWorldSubSystem::UnregisterCollisionManagerComponent()
{
	CollisionMangerComponent = nullptr;
}

// Clear cached collisions
void UGRSWorldSubSystem::ClearCollisions()
{
	if (LeftSideCollision)
	{
		LeftSideCollision->Destroy();
	}

	if (RightSideCollision)
	{
		RightSideCollision->Destroy();
	}

	LeftSideCollision = nullptr;
	RightSideCollision = nullptr;
}

/*********************************************************************************************
 * Ghost Characters
 **********************************************************************************************/

// Register character manager component
void UGRSWorldSubSystem::RegisterCharacterManagerComponent(UGRSGhostCharacterManagerComponent* NewCharacterManagerComponent)
{
	if (NewCharacterManagerComponent && NewCharacterManagerComponent != CharacterManagerComponent)
	{
		CharacterManagerComponent = NewCharacterManagerComponent;
	}

	TryInit();
}

// Register ghost character
void UGRSWorldSubSystem::RegisterGhostCharacter(AGRSPlayerCharacter* GhostPlayerCharacter)
{
	checkf(GhostPlayerCharacter, TEXT("ERROR: [%i] %hs:\n'GhostPlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	FBmrCell ActorSpawnLocation;
	float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

	if (!GhostCharacterLeftSide)
	{
		GhostCharacterLeftSide = GhostPlayerCharacter;

		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
	}
	else if (!GhostCharacterRightSide)
	{
		GhostCharacterRightSide = GhostPlayerCharacter;

		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
	}

	// Match the Z axis to what we have on the level
	ActorSpawnLocation.Location.Z = 100.0f;
	GhostPlayerCharacter->SetActorLocation(ActorSpawnLocation);
}

// Returns currently available ghost character or nullptr if there is no available ghosts.
AGRSPlayerCharacter* UGRSWorldSubSystem::GetAvailableGhostCharacter()
{
	if (!GhostCharacterLeftSide->GetController())
	{
		return GhostCharacterLeftSide;
	}

	if (!GhostCharacterRightSide->GetController())
	{
		return GhostCharacterRightSide;
	}

	return nullptr;
}

// Clears cached character manager component
void UGRSWorldSubSystem::UnregisterCharacterManagerComponent()
{
	CharacterManagerComponent = nullptr;
}

// Clear cached ghost character by reference
void UGRSWorldSubSystem::UnregisterGhostCharacter(AGRSPlayerCharacter* GhostPlayerCharacter)
{
	if (!GhostPlayerCharacter)
	{
		return;
	}

	if (GhostCharacterLeftSide == GhostPlayerCharacter)
	{
		GhostCharacterLeftSide = nullptr;

		return;
	}

	if (GhostCharacterRightSide == GhostPlayerCharacter)
	{
		GhostCharacterRightSide->Destroy();
		GhostCharacterRightSide = nullptr;
	}
}

// Clear cached ghost character references
void UGRSWorldSubSystem::ClearGhostCharacters()
{
	if (GhostCharacterLeftSide)
	{
		GhostCharacterLeftSide->Destroy();
		GhostCharacterLeftSide = nullptr;
	}

	if (GhostCharacterRightSide)
	{
		GhostCharacterRightSide->Destroy();
		GhostCharacterRightSide = nullptr;
	}
}

/*********************************************************************************************
 * Treasury (temp)
 **********************************************************************************************/

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	/** EngGameState:
//HUD = UWidgetsSubsystem::Get().GetWidgetByTag();
if (!ensureMsgf(HUD, TEXT("ASSERT: [%i] %hs:\n'HUD' is not valid!"), __LINE__, __FUNCTION__))
{
	break;
}
HUD->SetVisibility(ESlateVisibility::Collapsed);
PlayerStateInternal->SetCharacterDead(false);
PlayerStateInternal->SetOpponentKilledNum(0);
PlayerStateInternal->SetEndGameState(EBmrEndGameState::None);
*/
}
