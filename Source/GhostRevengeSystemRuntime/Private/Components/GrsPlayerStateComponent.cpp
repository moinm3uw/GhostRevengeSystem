// Copyright (c) Yevhenii Selivanov

#include "Components/GrsPlayerStateComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrPawn.h"
#include "Data/GRSDataAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/BmrGameState.h"
#include "GameFramework/BmrPlayerState.h"
#include "GrsGameplayTags.h"
#include "Structures/BmrGameStateTag.h"
#include "Structures/BmrGameplayTags.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

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

ABmrPlayerState* UGrsPlayerStateComponent::GetCurrentPlayerStateChecked() const
{
	ABmrPlayerState* InPlayerState = GetCurrentPlayerState();
	checkf(InPlayerState, TEXT("ERROR: [%i] %hs:\n'InPlayerState' is null!"), __LINE__, __FUNCTION__);
	return InPlayerState;
}

//  Checks if player state has authority
bool UGrsPlayerStateComponent::HasAuthority()
{
	return GetCurrentPlayerStateChecked()->HasAuthority();
}

// Called when the game starts
void UGrsPlayerStateComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("UGrsPlayerStateComponent::BeginPlay  --- %s - %s"), *this->GetName(), GetOwner()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"));
	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.RegisterPlayerStateComponent(this);
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

	UGRSWorldSubSystem& WorldSubsystem = UGRSWorldSubSystem::Get();
	WorldSubsystem.UnRegisterPlayerStateComponent(this);
}

// Starting point once whole module is ready(loaded) to be initialized
void UGrsPlayerStateComponent::OnInitialize(const struct FGameplayEventData& Payload)
{
	UGlobalMessageSubsystem::CallOrStartListeningForGlobalMessage(BmrGameplayTags::Event::GameState_Changed, this, &ThisClass::OnGameStateChanged);

	ApplyBombSpawningGameplayEffect();
}

// Listen game states to grant revive ability for player character
void UGrsPlayerStateComponent::OnGameStateChanged_Implementation(const struct FGameplayEventData& Payload)
{
	if (GetCurrentPlayerState()->IsABot())
	{
		return;
	}

	if (!Payload.InstigatorTags.HasTag(FBmrGameStateTag::InGame))
	{
		RemoveAppliedReviveGameplayEffect();
	}

	if (Payload.InstigatorTags.HasTag(FBmrGameStateTag::GameStarting))
	{
		GrantPlayerReviveEffect();
	}
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
	if (!HasAuthority())
	{
		return;
	}

	const ABmrGameState& GameState = ABmrGameState::Get();
	if (!PlayerCharacter || !GameState.HasMatchingGameplayTag(FBmrGameStateTag::InGame))
	{
		return;
	}

	if (PlayerCharacter->GetPlayerState() != Cast<APlayerState>(GetCurrentPlayerStateChecked()))
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

	UGRSWorldSubSystem::Get().SetRevivedPlayer(PlayerCharacter);
}

// Grant to a player revive GAS effect
void UGrsPlayerStateComponent::GrantPlayerReviveEffect()
{
	if (HasAuthority())
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
	if (HasAuthority())
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

// Spawn bomb on target location
void UGrsPlayerStateComponent::UseSpawnBomb(FBmrCell TargetCell, const AActor* TargetInstigator)
{
}

//  To apply explosion (bomb spawning) gameplay effect
void UGrsPlayerStateComponent::ApplyBombSpawningGameplayEffect()
{
	if (!HasAuthority())
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
	if (!HasAuthority())
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