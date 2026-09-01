// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "Abilities/GrsReviveAbility.h"

#include "GhostRevengeSystemRuntimeModule.h" // LogGrs

// Bmr
#include "Actors/BmrGeneratedMap.h"
#include "Components/BmrMapComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsReviveAbility)

// Actually activate ability, do not call this directly
void UGrsReviveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	checkf(ActorInfo, TEXT("ERROR: [%i] %hs:\n'ActorInfo' is null!"), __LINE__, __FUNCTION__);
	checkf(TriggerEventData, TEXT("ERROR: [%i] %hs:\n'TriggerEventData' is null!"), __LINE__, __FUNCTION__);

	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	ABmrGeneratedMap::Get().AddToGrid(UBmrMapComponent::GetMapComponent(AvatarActor));
}