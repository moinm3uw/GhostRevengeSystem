// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// Grs
#include "LevelActors/GrsPawn.h"

// UE
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GrsUtils.generated.h"

/**
 *
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** It is farthest possible location where deactivated actors are placed, is the maximum integer-like coordinates supported by UE network (for standard ReplicatedMovement actors) with 20 bits per axis for */
	inline static const FVector MaxPos = {1048576.0f, 1048576.0f, 1048576.0f};

	/** Returns the ghost character */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get Ghost Player Character", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class AGrsPawn* GetGhostPlayerCharacter(const UObject* OptionalWorldContext = nullptr);

	/** Returns the ghost controller component, nullptr otherwise. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get BMR Player Controller (Local)", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class UGrsPlayerControllerComponent* GetControllerComponent(const UObject* OptionalWorldContext = nullptr);

	/** Calculates the character side from an actor reference */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static EGRSCharacterSide GetCharacterSideFromActor(AActor* Actor);
};
