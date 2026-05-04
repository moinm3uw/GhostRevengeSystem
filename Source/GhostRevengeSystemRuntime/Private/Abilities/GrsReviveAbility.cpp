// Copyright (c) Yevhenii Selivanov

#include "Abilities/GrsReviveAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Actors/BmrGeneratedMap.h"
#include "Components/BmrMapComponent.h"
#include "Components/BmrMoverComponent.h"

// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsReviveAbility)

// Actually activate ability, do not call this directly
void UGrsReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- GRS Activate triggered"), __LINE__, __FUNCTION__);
	check(ActorInfo && TriggerEventData);
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ABmrGeneratedMap::Get().AddToGrid(UBmrMapComponent::GetMapComponent(AvatarActor));
}