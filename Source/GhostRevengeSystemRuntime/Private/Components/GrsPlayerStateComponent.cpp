// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Components/GrsPlayerStateComponent.h"

// Grs
#include "Data/GRSDataAsset.h"
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
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "GhostRevengeSystemRuntimeModule.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPlayerStateComponent)

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

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: %s "), __LINE__, __FUNCTION__, GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));

	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(GrsGameplayTags::Event::GameFeaturePluginReady, this, &ThisClass::OnInitialize);
}

//  Called as part of MGF(GFP) lifecycle when unload happens
void UGrsPlayerStateComponent::OnUnregister()
{
	Super::OnUnregister();

	UGlobalMessageSubsystem::StopListeningForAllGlobalMessages(this);

	RemoveAppliedReviveGameplayEffect();
	RemoveBombSpawningGameplayEffect();

	if (AppliedBombSpawnEffectHandle.IsValid())
	{
		AppliedBombSpawnEffectHandle.Invalidate();
	}

	ABmrPlayerState* BmrPlayerState = GetCurrentPlayerState();
	if (BmrPlayerState && BmrPlayerState->OnOpponentsKilledNumChanged.IsBound())
	{
		BmrPlayerState->OnOpponentsKilledNumChanged.RemoveDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
	}
}

// Starting point once whole module is ready(loaded) to be initialized
void UGrsPlayerStateComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);
}

// Listen game states to grant revive ability for player character
void UGrsPlayerStateComponent::OnGameStateChanged_Implementation(const struct FGameplayEventData& Payload)
{
	if (GetCurrentPlayerState()->IsABot())
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

		ABmrPlayerState& BmrPlayerState = GetCurrentPlayerStateChecked();
		BmrPlayerState.OnOpponentsKilledNumChanged.AddUniqueDynamic(this, &ThisClass::OnOpponentsKilledNumChanged);
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
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}
	APawn& CurrentPawn = GetCurrentPlayerStateChecked().GetPawnChecked();
	AGrsPawn* GrsPawn = Cast<AGrsPawn>(&CurrentPawn); // --- if no grs pawn means elimination was done by a player not ghost

	if (!GrsPawn || GrsPawn->GetPlayerID() != GetCurrentPlayerStateChecked().GetPlayerId())
	{
		return;
	}

	ABmrPawn* PlayerCharacter = UBmrBlueprintFunctionLibrary::GetPawn(GrsPawn->GetPlayerID());
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

// Apply review ability that will restore regular player character
void UGrsPlayerStateComponent::RevivePlayerCharacter(ABmrPawn* PlayerCharacter)
{
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

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
	ASC->HandleGameplayEvent(UGRSDataAsset::Get().GetReviePlayerCharacterTriggerTag(), &EventData);

	UGRSWorldSubSystem::Get(this).SetRevivedPlayer(PlayerCharacter);
}

// Grant to a player revive GAS effect
void UGrsPlayerStateComponent::GrantPlayerReviveEffect()
{
	if (!GetCurrentPlayerStateChecked().HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ensureMsgf(ASC, TEXT("ASSERT: [%i] %hs:\n 'ASC' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	TSubclassOf<UGameplayEffect> PlayerReviveEffect = UGRSDataAsset::Get().GetPlayerReviveEffect();
	if (ensureMsgf(PlayerReviveEffect, TEXT("ASSERT: [%i] %hs:\n'PlayerDeathEffect' is not set!"), __LINE__, __FUNCTION__))
	{
		ASC->ApplyGameplayEffectToSelf(PlayerReviveEffect.GetDefaultObject(), /*Level*/ 1.f, ASC->MakeEffectContext());
	}
}

// To Remove Revive applied gameplay effect
void UGrsPlayerStateComponent::RemoveAppliedReviveGameplayEffect()
{
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

	TSubclassOf<UGameplayEffect> PlayerReviveEffect = UGRSDataAsset::Get().GetPlayerReviveEffect();
	if (!ensureMsgf(PlayerReviveEffect, TEXT("ASSERT: [%i] %hs:\n'PlayerDeathEffect' is not returned from data asset!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = PlayerReviveEffect;
	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	for (FActiveGameplayEffectHandle Handle : Handles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
		Handle.Invalidate();
	}
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
	TSubclassOf<UGameplayEffect> ExplosionDamageEffect = UGRSDataAsset::Get().GetExplosionDamageEffect();
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