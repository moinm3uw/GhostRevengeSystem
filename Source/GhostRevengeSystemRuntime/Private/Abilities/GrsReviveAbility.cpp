// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Abilities/GrsReviveAbility.h"
#include "GhostRevengeSystemRuntimeModule.h"

// Bmr
#include "Actors/BmrGeneratedMap.h"
#include "Components/BmrMapComponent.h"
// @PR JanSeliv [Coding Standards] - unused include, BmrMoverComponent never referenced, remove. Applies across file: AbilitySystemComponent.h and AbilitySystemGlobals.h also unused
#include "Components/BmrMoverComponent.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

// @PR JanSeliv [Coding Standards] - uncomment, reflection cpp requires active UE_INLINE_GENERATED_CPP_BY_NAME after includes, no commented-out code. Ref active in neighbor BmrPlayerDeathAbility.cpp
#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsReviveAbility)

// Actually activate ability, do not call this directly
void UGrsReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	// @PR JanSeliv [Coding Standards] - split combined assert per symbol so failure names which is null, only `check(A && B)` in module. Use checkf per symbol like sibling funcs `is null!`
	check(ActorInfo && TriggerEventData);
	// @PR JanSeliv [Coding Standards] - const pointee, AvatarActor only read, passed to GetMapComponent const param, never mutated
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ABmrGeneratedMap::Get().AddToGrid(UBmrMapComponent::GetMapComponent(AvatarActor));
}