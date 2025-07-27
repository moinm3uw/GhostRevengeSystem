// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "GhostRevengeSystemComponent.h"
#include "PoolManagerSubsystem.h"
#include "Data/GRSDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"

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

// Returns owner (main) player character
APlayerCharacter* UGhostRevengeSystemComponent::GetMainPlayerCharacter() const
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwner());

	return PlayerCharacter ? PlayerCharacter : nullptr;
}

// Called when the game starts
void UGhostRevengeSystemComponent::BeginPlay()
{
	Super::BeginPlay();

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
	case ECurrentGameState::GameStarting:
		{
			// required since the characters added on game load, but at that point it might not have all local players connected
			RegisterMainCharacter();
			break;
		}
	default: break;
	}
}

// Register into world subsystem main character
void UGhostRevengeSystemComponent::RegisterMainCharacter()
{
	if (AMyGameStateBase::GetCurrentGameState() == ECurrentGameState::GameStarting || AMyGameStateBase::GetCurrentGameState() == ECurrentGameState::InGame)
	{
		APlayerCharacter* PlayerCharacter = GetMainPlayerCharacter();
		if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__))
		{
			return;
		}

		if (!PlayerCharacter->IsPlayerControlled())
		{
			return;
		}

		UGRSWorldSubSystem::Get().RegisterMainCharacter(PlayerCharacter);
	}
}

// Called when this level actor is reconstructed or added on the Generated Map, on both server and clients.
void UGhostRevengeSystemComponent::OnAddedToLevel_Implementation(class UMapComponent* MapComponent)
{
	RegisterMainCharacter();
}

// Called right before owner actor going to remove from the Generated Map
void UGhostRevengeSystemComponent::OnPostRemovedFromLevel_Implementation(class UMapComponent* MapComponent, UObject* DestroyCauser)
{
	// --- ignore if not in game
	if (AMyGameStateBase::GetCurrentGameState() != ECurrentGameState::InGame)
	{
		return;
	}

	if (MapComponent == UMapComponent::GetMapComponent(GetOwner()))
	{
		UGRSWorldSubSystem::Get().MainCharacterRemovedFromLevel(MapComponent);
	}
}
