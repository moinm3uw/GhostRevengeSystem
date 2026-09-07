// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Components/GrsPlayerStateComponent.h"

#include "Data/GRSDataAsset.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "GrsGameplayTags.h"
#include "LevelActors/GrsPawn.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// MyEditorUtils
#include "Subsystems/GlobalMessageSubsystem.h"

// UE
#include "Abilities/GameplayAbilityTypes.h" // FGameplayEventData
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPlayerStateComponent)

// Sets default values for this component's properties
UGrsPlayerStateComponent::UGrsPlayerStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Returns the player state from attached BmrPlayerState component
ABmrPlayerState* UGrsPlayerStateComponent::GetCurrentPlayerState() const
{
	return Cast<ABmrPlayerState>(GetOwner());
}

ABmrPlayerState& UGrsPlayerStateComponent::GetCurrentPlayerStateChecked() const
{
	ABmrPlayerState* InPlayerState = GetCurrentPlayerState();
	checkf(InPlayerState, TEXT("ERROR: [%i] %hs:\n'InPlayerState' is null!"), __LINE__, __FUNCTION__);
	return *InPlayerState;
}

// Called when the game starts
void UGrsPlayerStateComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

//  Called as part of GFP lifecycle when unload happens
void UGrsPlayerStateComponent::OnUnregister()
{
	Super::OnUnregister();

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	RemoveAppliedReviveGameplayEffect();
	RemoveBombSpawningGameplayEffect();

	AppliedBombSpawnEffectHandle.Invalidate();

	ABmrPlayerState* BmrPlayerState = GetCurrentPlayerState();
	if (BmrPlayerState)
	{
		BmrPlayerState->OnOpponentsKilledNumChanged.RemoveDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
	}

	PreviousGrsPawn = nullptr; // --- reset the pointer as it should apply only once
}

// Starting point once whole module is ready(loaded) to be initialized
void UGrsPlayerStateComponent::OnInitialize_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Listen game states to grant revive ability for player character
void UGrsPlayerStateComponent::OnGameStateChanged_Implementation(const FGameplayEventData& Payload)
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	if (GetCurrentPlayerStateChecked().IsABot())
	{
		return;
	}

	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::GameStarting))
	{
		RemoveBombSpawningGameplayEffect();
		RemoveAppliedReviveGameplayEffect();
	}

	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		ApplyBombSpawningGameplayEffect();
		GrantPlayerReviveEffect();
		PreviousGrsPawn = nullptr; // --- reset the pointer as it should apply only once

		ABmrPlayerState& BmrPlayerStateRef = GetCurrentPlayerStateChecked();
		BmrPlayerStateRef.OnOpponentsKilledNumChanged.AddUniqueDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
	}
}

// Is increased when this player kills an opponent
void UGrsPlayerStateComponent::OnOpponentsKilledNumChanged_Implementation(int32 OpponentsKilledNum)
{
	// --- ignore reset cases
	if (OpponentsKilledNum < 1)
	{
		return;
	}

	TryReviveCharacter(); // --- revive main player character
}

// Tries to revive main player character when a ghost eliminates an enemy on level including elimination of bots
void UGrsPlayerStateComponent::TryReviveCharacter()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s)"), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	const ABmrPlayerState& PlayerStateRef = GetCurrentPlayerStateChecked();
	if (!PlayerStateRef.HasAuthority()
	    || !PreviousGrsPawn // pawn could be not set (killing a bot)
	    || PreviousGrsPawn->GetPlayerID() != PlayerStateRef.GetPlayerId())
	{
		return;
	}

	ABmrPawn* PlayerCharacter = UBmrBlueprintFunctionLibrary::GetPawn(PreviousGrsPawn->GetPlayerID());
	if (!ensureMsgf(PlayerCharacter, TEXT("ASSERT: [%i] %hs:\n'PlayerCharacter' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	RevivePlayerCharacter(PlayerCharacter);
}

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* UGrsPlayerStateComponent::GetAbilitySystemComponent() const
{
	const ABmrPlayerState* InPlayerState = GetCurrentPlayerState();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

/*********************************************************************************************
 * Revive ability
 **********************************************************************************************/

//  Assign previous GrsPawn reference to track an elimination done by GrsPawn
void UGrsPlayerStateComponent::AssignPreviousGrsPawn(AGrsPawn* NewGrsPawn)
{
	if (!ensureMsgf(NewGrsPawn, TEXT("ASSERT: [%i] %hs:\n 'NewGrsPawn' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	PreviousGrsPawn = NewGrsPawn;
}

// Apply a revive ability that will restore regular player character
void UGrsPlayerStateComponent::RevivePlayerCharacter(ABmrPawn* PlayerCharacter)
{
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) Activate to: %s"), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *GetNameSafe(PlayerCharacter));

	const ABmrGameState& GameState = ABmrGameState::Get();
	if (!PlayerCharacter || !GameState.HasMatchingGameplayTag(FBmrGameStateTag::InGame))
	{
		return;
	}

	// --- Activate revive ability if player was NOT revived previously
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ensureMsgf(ASC, TEXT("ASSERT: [%i] %hs:\n 'ASC' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(PlayerCharacter->GetActorLocation());
	ASC->HandleGameplayEvent(UGRSDataAsset::Get().GetRevivePlayerCharacterTriggerTag(), &EventData);
	PreviousGrsPawn = nullptr; // --- reset the pointer as it should apply only once
	UGRSWorldSubSystem::Get().SetRevivedPlayer(PlayerCharacter);
}

// Grant to a player revive GAS effect
void UGrsPlayerStateComponent::GrantPlayerReviveEffect()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) STARTED "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ensureMsgf(ASC, TEXT("ASSERT: [%i] %hs:\n 'ASC' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> PlayerReviveEffect = UGRSDataAsset::Get().GetPlayerReviveEffectClass();
	if (ensureMsgf(PlayerReviveEffect, TEXT("ASSERT: [%i] %hs:\n'PlayerReviveEffect' is not set!"), __LINE__, __FUNCTION__))
	{
		ASC->ApplyGameplayEffectToSelf(PlayerReviveEffect.GetDefaultObject(), /*Level*/ 1.0f, ASC->MakeEffectContext());
	}

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) Applied revive gameplay effect to self  "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
}

// To Remove Revive applied gameplay effect
void UGrsPlayerStateComponent::RemoveAppliedReviveGameplayEffect()
{
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) STARTED  "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

	// Actor has ASC: apply effect through GAS
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> PlayerReviveEffect = UGRSDataAsset::Get().GetPlayerReviveEffectClass();
	if (!ensureMsgf(PlayerReviveEffect, TEXT("ASSERT: [%i] %hs:\n'PlayerReviveEffect' is not returned from data asset!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = PlayerReviveEffect;
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	Handles.Empty();

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: (%s) Removed revive gameplay effect "), __LINE__, __FUNCTION__, GetCurrentPlayerStateChecked().HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
}

/*********************************************************************************************
 * Bomb spawning ability that automatically explodes after a certain time
 **********************************************************************************************/

//  To apply explosion (bomb spawning) gameplay effect
void UGrsPlayerStateComponent::ApplyBombSpawningGameplayEffect()
{
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

	// Actor has ASC: apply damage effect through GAS
	if (AppliedBombSpawnEffectHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	const TSubclassOf<UGameplayEffect> ExplosionDamageEffect = UGRSDataAsset::Get().GetExplosionDamageEffectClass();
	if (!ensureMsgf(ExplosionDamageEffect, TEXT("ASSERT: [%i] %hs:\n'ExplosionDamageEffect' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	AppliedBombSpawnEffectHandle = ASC->ApplyGameplayEffectToSelf(ExplosionDamageEffect.GetDefaultObject(), /*Level*/ 1.f, ASC->MakeEffectContext());
}

// To Remove applied explosion (bomb spawning) gameplay effect
void UGrsPlayerStateComponent::RemoveBombSpawningGameplayEffect()
{
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !AppliedBombSpawnEffectHandle.IsValid())
	{
		return;
	}

	ASC->RemoveActiveGameplayEffect(AppliedBombSpawnEffectHandle);
	AppliedBombSpawnEffectHandle.Invalidate();
}