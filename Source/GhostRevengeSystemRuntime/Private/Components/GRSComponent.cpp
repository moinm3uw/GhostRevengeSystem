// Copyright (c) Yevhenii Selivanov


#include "Components/GRSComponent.h"

#include "GameFramework/MyPlayerState.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UI/Widgets/HUDWidget.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

// Sets default values for this component's properties
UGRSComponent::UGRSComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Called when a component is registered, after Scene is set, but before CreateRenderState_Concurrent or OnCreatePhysicsState are called
void UGRSComponent::OnRegister()
{
	Super::OnRegister();

	BIND_ON_LOCAL_CHARACTER_READY(this, UGRSComponent::OnLocalCharacterReady);
	UE_LOG(LogTemp, Warning, TEXT("UGRSComponent OnRegister"));
}

// Clears all transient data created by this component
void UGRSComponent::OnUnregister()
{
	Super::OnUnregister();
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
	PlayerStateInternal = PlayerState;
	PlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);
}

// Called when the end game state was changed to recalculate progression according to endgame (win, loss etc.) 
void UGRSComponent::OnEndGameStateChanged_Implementation(EEndGameState EndGameState)
{
	class UHUDWidget* HUD  = nullptr;
	switch (EndGameState)
	{
	case EEndGameState::Lose:
		//HUD = UWidgetsSubsystem::Get().GetWidgetByTag();
		if (!ensureMsgf(HUD, TEXT("ASSERT: [%i] %hs:\n'HUD' is not valid!"), __LINE__, __FUNCTION__))
		{
			break;
		}
		HUD->SetVisibility(ESlateVisibility::Collapsed);
		PlayerStateInternal->SetCharacterDead(false);
		PlayerStateInternal->SetOpponentKilledNum(0);
		PlayerStateInternal->SetEndGameState(EEndGameState::None);
		break;

	default: break;
	}
}
