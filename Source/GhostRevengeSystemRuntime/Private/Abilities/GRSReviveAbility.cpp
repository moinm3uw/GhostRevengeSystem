// Copyright (c) Yevhenii Selivanov

#include "Abilities/GRSReviveAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/BmrMoverComponent.h"
#include "Components/MapComponent.h"
#include "Data/GRSDataAsset.h"
#include "GeneratedMap.h"
#include "LevelActors/PlayerCharacter.h"

// Actually activate ability, do not call this directly
void UGRSReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	check(ActorInfo && TriggerEventData);
	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	AGeneratedMap::Get().AddToGrid(UMapComponent::GetMapComponent(AvatarActor));
}