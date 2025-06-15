// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "SubSystems/GRSWorldSubSystem.h"

#include "PoolManagerSubsystem.h"
#include "Controllers/MyPlayerController.h"
#include "Data/EGRSSpotType.h"
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
			RemoveGhostCharacter();
		}
		break;
	case ECurrentGameState::GameStarting:
		{
			OnInitialize.Broadcast();
			OnInit();
			break;
		}
	default: break;
	}
}

void UGRSWorldSubSystem::OnInit_Implementation()
{
	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

void UGRSWorldSubSystem::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	// Binds the local player state ready event to the handler
	BIND_ON_LOCAL_PLAYER_STATE_READY(this, ThisClass::OnLocalPlayerStateReady);
	LocalPlayerCharacterInternal = PlayerCharacter;
}

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

void UGRSWorldSubSystem::OnEndGameStateChanged_Implementation(EEndGameState EndGameState)
{
	class UHUDWidget* HUD = nullptr;
	// Spawn parameters
	FActorSpawnParameters SpawnParams;

	FVector SpawnLocation = UGRSDataAsset::Get().GetSpawnLocation();
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

void UGRSWorldSubSystem::AddGhostCharacter()
{
	SpawnMapCollisionOnSide();

	//Return to Pool Manager the list of handles which is not needed (if there are any) 
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

void UGRSWorldSubSystem::SpawnMapCollisionOnSide()
{
	// Spawn side collision
	const TSubclassOf<AActor> CollisionsAssetClass = UGeneratedMapDataAsset::Get().GetCollisionsAssetClass();
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(CollisionsAssetClass, UGRSDataAsset::Get().GetCollisionTransform());
	if (SpawnedActor)
	{
		SpawnedActor->SetActorTransform(UGRSDataAsset::Get().GetCollisionTransform());
	}
}

void UGRSWorldSubSystem::OnTakeActorsFromPoolCompleted(const TArray<FPoolObjectData>& CreatedObjects)
{
	// something wrong if there are more than 1 object found
	if (CreatedObjects.Num() > 1)
	{
		return;
	}

	// Spawn ghost character 
	FVector SpawnLocation = UGRSDataAsset::Get().GetSpawnLocation();

	// Setup spawned widget
	for (const FPoolObjectData& CreatedObject : CreatedObjects)
	{
		AGRSPlayerCharacter& GhostPlayerCharacter = CreatedObject.GetChecked<AGRSPlayerCharacter>();
		ensure(&GhostPlayerCharacter);
		GhostPlayerCharacterInternal = GhostPlayerCharacter;

		GhostPlayerCharacter.SetActorLocation(SpawnLocation);
		// Possess the ghost character
		AMyPlayerController* PC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
		PC->Possess(GhostPlayerCharacterInternal);
		SetManagedInputContextEnabled(true);

		GhostPlayerCharacterInternal->SetVisibility(true);
	}
}

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

void UGRSWorldSubSystem::RemoveGhostCharacter()
{
	if (!GhostPlayerCharacterInternal)
	{
		return;
	}

	SetManagedInputContextEnabled(false);

	AMyPlayerController* PC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	PC->Possess(LocalPlayerCharacterInternal);
	GhostPlayerCharacterInternal->SetVisibility(false);

	// Destroying Star Actors
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