// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Components/GrsHUDComponent.h"

#include "GhostRevengeSystemRuntimeModule.h" // LogGrs

// Bmr
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "UI/Widgets/BmrHUDWidget.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsHUDComponent)

/*********************************************************************************************
 * Lifecycle
 **********************************************************************************************/

// Sets default values for this component's properties
UGrsHUDComponent::UGrsHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(false);
}

// Called when the game starts
void UGrsHUDComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	// Binds to local character ready to guarantee that the player state is initialized
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::Player_LocalPawnReady, this, &ThisClass::OnLocalPawnReady);
}

// Clears all transient data created by this component.
void UGrsHUDComponent::OnUnregister()
{
	Super::OnUnregister();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	// --- GFP is unloading, so the HUD has to be restored back
	UBmrHUDWidget* BmrHUD = UBmrBlueprintFunctionLibrary::GetHUDWidget(this);
	if (BmrHUD)
	{
		BmrHUD->SetVisibility(ESlateVisibility::Visible);
	}

	ABmrPlayerState* PlayerState = UBmrBlueprintFunctionLibrary::GetLocalPlayerState();
	if (!PlayerState)
	{
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: 'PlayerState' is null! "), __LINE__, __FUNCTION__);
		return;
	}

	PlayerState->OnEndGameStateChanged.RemoveDynamic(this, &ThisClass::OnEndGameStateChanged);
}

/*********************************************************************************************
 * Main functionality
 **********************************************************************************************/

// Is called when local player character is ready to guarantee that the player state is initialized required for the OnEndGameStateChanged subscription
void UGrsHUDComponent::OnLocalPawnReady_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	const APawn* Pawn = Cast<APawn>(Payload.Instigator.Get());
	ABmrPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<ABmrPlayerState>() : nullptr;
	checkf(PlayerState, TEXT("ERROR: [%i] %hs:\n'PlayerState' is null!"), __LINE__, __FUNCTION__);
	PlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Listen game states to restore the HUD back once the match is over
void UGrsHUDComponent::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (!Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		bool bShowHUDEndResult = true;
		ChangeHUDEndResultVisibility(bShowHUDEndResult);
	}
}

// Listen end game states of the local player to hide the HUD while they are playing as a ghost
void UGrsHUDComponent::OnEndGameStateChanged_Implementation(EBmrEndGameState EndGameState)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (EndGameState == EBmrEndGameState::Lose || EndGameState == EBmrEndGameState::HonorLoss)
	{
		bool bShowHUDEndResult = false;
		ChangeHUDEndResultVisibility(bShowHUDEndResult);
	}
}

//  Changes the Bmr HUD visibility
void UGrsHUDComponent::ChangeHUDEndResultVisibility(bool bVisibility)
{
	const FName ResultTextBlockName = TEXT("RESULT");
	UTextBlock* ResultTextBlock = GetTextBlockToHide(ResultTextBlockName);
	if (!ensureMsgf(ResultTextBlock, TEXT("ASSERT: [%i] %hs:\n'ResultTextBlock' with name %s is not found in the BmrHUD !"), __LINE__, __FUNCTION__, *ResultTextBlockName.ToString()))
	{
		return;
	}

	ESlateVisibility NewVisibility = bVisibility ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	ResultTextBlock->SetVisibility(NewVisibility);
}

// Find and return a textblock element responsible for the end game result
UTextBlock* UGrsHUDComponent::GetTextBlockToHide(FName ResultTextBlockName)
{
	UBmrHUDWidget* BmrHUD = UBmrBlueprintFunctionLibrary::GetHUDWidget(this);
	if (!ensureMsgf(BmrHUD, TEXT("ASSERT: [%i] %hs:\n'BmrHUD' is not valid!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	/* @PR JanSeliv [Architecture] - Wrap entire hack as separate function, marked as @TODO for JanSeliv.
	 * @TODO JanSeliv: widget tree is walked by hardcoded 'RESULT' name, while core already exposes UBmrMVVM_GameViewModel::SetEndGameStateVisibility for it.
	 * Blocked: there is no supported way to obtain that view model by class, UMVVMViewModelCollectionObject::FindViewModelInstance requires the instance name. */
	UTextBlock* FoundTextBlock = nullptr;
	TArray<UWidget*> AllWidgets;
	BmrHUD->WidgetTree->GetAllWidgets(AllWidgets);
	
	for (UWidget* Widget : AllWidgets)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			if (TextBlock->GetName() == ResultTextBlockName)
			{
				FoundTextBlock = TextBlock;
			}
		}
	}

	return FoundTextBlock;
}
