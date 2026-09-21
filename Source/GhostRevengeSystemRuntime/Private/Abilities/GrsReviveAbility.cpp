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
	if (AvatarActor->HasAuthority())
	{
		UBmrMapComponent* MapComponent = UBmrMapComponent::GetMapComponent(AvatarActor);
		if (!ensureMsgf(MapComponent, TEXT("ASSERT: [%i] %hs:\n Activated ability to an actor that is not on the map - 'MapComponent' is not set!"), __LINE__, __FUNCTION__))
		{
			return;
		}

		MapComponent->SetCell(FBmrCell::InvalidCell); // @todo temporary fix to force ABmrGeneratedMap::AddToGrid to treat it as a fresh add till a new approach with tags will be implemented
		ABmrGeneratedMap::Get().AddToGrid(MapComponent);
	}
}