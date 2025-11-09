// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"

#include "GRSReviveAbility.generated.h"

/**
 * Handles player revive after ghost kills a player.
 * Ability is triggered by the BmrGameplayTags::Event::Player_Revive event, where:
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGRSReviveAbility : public UGameplayAbility
{
	GENERATED_BODY()
	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Actually activate ability, do not call this directly. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
