// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelActors/GRSPlayerCharacter.h"

#include "GhostRevengeUtils.generated.h"

/**
 *
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGhostRevengeUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the ghost character */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get Ghost Player Character", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class AGRSPlayerCharacter* GetGhostPlayerCharacter(const UObject* OptionalWorldContext = nullptr);

	/** Returns the ghost controller component, nullptr otherwise. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get BMR Player Controller (Local)", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class UGRSPlayerControllerComponent* GetControllerComponent(const UObject* OptionalWorldContext = nullptr);
	
	/** Calculates the character side from an actor reference */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static EGRSCharacterSide GetCharacterSideFromActor(AActor* Actor);
};
