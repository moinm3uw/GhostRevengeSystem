// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "Components/GRSComponent.h"

#include "Components/MySkeletalMeshComponent.h"
#include "Controllers/MyPlayerController.h"
#include "Data/GRSDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "LevelActors/PlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UI/Widgets/HUDWidget.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

// Sets default values for this component's properties
UGRSComponent::UGRSComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Sets default values for this component's properties
AMyPlayerController* UGRSComponent::GetPlayerController() const
{
	return Cast<AMyPlayerController>(GetOwner());
}

// Returns Player Controller of this component
AMyPlayerController& UGRSComponent::GetPlayerControllerChecked() const
{
	AMyPlayerController* MyPlayerController = GetPlayerController();
	checkf(MyPlayerController, TEXT("%s: 'MyPlayerController' is null"), *FString(__FUNCTION__));
	return *MyPlayerController;
}

// Called when the owning Actor begins play or when the component is created if the Actor has already begun play
void UGRSComponent::BeginPlay()
{
	Super::BeginPlay();

	UGRSWorldSubSystem::Get().OnInitialize.AddUniqueDynamic(this, &ThisClass::OnInitialize);
}

// Clears all transient data created by this component
void UGRSComponent::OnUnregister()
{
	Super::OnUnregister();
}

// Enables or disables the input context
void UGRSComponent::SetManagedInputContextEnabled()
{
	AMyPlayerController& PC = GetPlayerControllerChecked();

	// Remove all previous input contexts managed by Controller
	TArray<const UMyInputMappingContext*> InputContexts;
	UMyInputMappingContext* InputContext = UGRSDataAsset::Get().GetInputContext();
	InputContexts.AddUnique(InputContext);
	PC.RemoveInputContexts(InputContexts);

	// Add gameplay context as auto managed by Game State, so it will be enabled everytime the game is in the in-game state
	if (InputContext
		&& InputContext->GetChosenGameStatesBitmask() > 0)
	{
		PC.SetupInputContexts(InputContexts);
	}
}

// Called when the ghost revenge system is ready loaded (when game transitions to in-game state)
void UGRSComponent::OnInitialize_Implementation()
{
	UGRSWorldSubSystem::Get().RegisterSpotComponent(this);
	BIND_ON_LOCAL_CHARACTER_READY(this, UGRSComponent::OnLocalCharacterReady);
	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
	UE_LOG(LogTemp, Warning, TEXT("UGRSComponent OnRegister"));
	UE_LOG(LogTemp, Warning, TEXT("UGRSDataAsset string %s"), *UGRSDataAsset::Get().GetTestString());
}

// Called when the local player character is spawned, possessed, and replicated
void UGRSComponent::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	// Binds the local player state ready event to the handler
	BIND_ON_LOCAL_PLAYER_STATE_READY(this, ThisClass::OnLocalPlayerStateReady);
}

// Subscribes to the end game state change notification on the player state.
void UGRSComponent::OnLocalPlayerStateReady_Implementation(class AMyPlayerState* PlayerState, int32 CharacterID)
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
void UGRSComponent::OnEndGameStateChanged_Implementation(EEndGameState EndGameState)
{
	class UHUDWidget* HUD = nullptr;
	GhostPlayerCharacter = NewObject<AGRSPlayerCharacter>();
	if (!ensureMsgf(GhostPlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'GhostPlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	// Spawn parameters
	FActorSpawnParameters SpawnParams;

	FVector SpawnLocation = UGRSDataAsset::Get().GetSpawnLocation();
	switch (EndGameState)
	{
	case EEndGameState::Lose:
		{
			// Spawn ghost character 
			GhostPlayerCharacter = GetWorld()->SpawnActor<AGRSPlayerCharacter>(GhostPlayerCharacter->GetClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
			// Posses controller
			SetManagedInputContextEnabled();
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

// Listen game states to switch character skin. 
void UGRSComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::Menu:
		{
			if (GhostPlayerCharacter)
			{
				GhostPlayerCharacter->Destroy();
			}
		}
		break;
	default: break;
	}
}
