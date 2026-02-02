// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "SubSystems/GRSWorldSubSystem.h"

#include "Actors/BmrPawn.h"
#include "Data/GRSDataAsset.h"
#include "Data/MyPrimaryDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/BmrGameState.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/BmrGlobalEventsSubsystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

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

	CharacterMangerComponent = nullptr;
	CollisionMangerComponent = nullptr;

	GhostCharacterLeftSide = nullptr;
	GhostCharacterRightSide = nullptr;

	LastActivatedGhostCharacter = nullptr;

	LeftSideCollisionInternal = nullptr;
	RightSideCollisionInternal = nullptr;
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

	Init(); // try to initialize
}

// Register collision manager component used to track if all components loaded and MGF ready to initialize
void UGRSWorldSubSystem::RegisterCollisionManagerComponent(class UGhostRevengeCollisionComponent* NewCollisionManagerComponent)
{
	if (NewCollisionManagerComponent && NewCollisionManagerComponent != CollisionMangerComponent)
	{
		CollisionMangerComponent = NewCollisionManagerComponent;
	}

	Init(); // try to initialize
}

// Register ghost character
void UGRSWorldSubSystem::RegisterGhostCharacter(class AGRSPlayerCharacter* GhostPlayerCharacter)
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

void UGRSWorldSubSystem::SetLastActivatedGhostCharacter(AGRSPlayerCharacter* GhostCharacter)
{
	if (!GhostCharacter || GhostCharacter == LastActivatedGhostCharacter)
	{
		return;
	}

	LastActivatedGhostCharacter = GhostCharacter;
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
void UGRSWorldSubSystem::OnLocalPawnReady_Implementation(class ABmrPawn* PlayerCharacter, int32 CharacterID)
{ :D 
	UE_LOG(LogTemp, Log, TEXT("UGRSWorldSubSystem::OnLocalCharacterReady_Implementation  --- %s"), *this->GetName());

	Init(); // try to initialize

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Checks if all components present and invokes initialization
void UGRSWorldSubSystem::Init()
{
	if (CharacterMangerComponent && CollisionMangerComponent)
	{
		if (OnInitialize.IsBound())
		{
			OnInitialize.Broadcast();
			OnInitialize.Clear();
		}
	}
}

// Return a ghost character
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
	else if (!RightSideCollisionInternal)
	{
		RightSideCollisionInternal = Actor;
	}
}

// Returns TRUE if collision are spawned
bool UGRSWorldSubSystem::IsCollisionsSpawned()
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
	if (LeftSideCollisionInternal)
	{
		return LeftSideCollisionInternal;
	}

	return nullptr;
}

// Returns right side spawned collision or nullptr
class AActor* UGRSWorldSubSystem::GetRightCollisionActor()
{
	if (RightSideCollisionInternal)
	{
		return RightSideCollisionInternal;
	}

	return nullptr;
}

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(EBmrCurrentGameState CurrentGameState) {}

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