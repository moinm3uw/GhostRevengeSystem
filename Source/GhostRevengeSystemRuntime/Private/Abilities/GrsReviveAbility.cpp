// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "Abilities/GrsReviveAbility.h"

// Bmr
#include "Actors/BmrGeneratedMap.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrMoverComponent.h"

// UE
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GhostRevengeSystemRuntimeModule.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsReviveAbility)

// Actually activate ability, do not call this directly
void UGrsReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	check(ActorInfo && TriggerEventData);
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ABmrGeneratedMap::Get().AddToGrid(UBmrMapComponent::GetMapComponent(AvatarActor));
}