// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GrsUtils.generated.h"

enum class EGRSCharacterSide : uint8;

/**
 * Useful GFP wide utils methods and data.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The maximum integer-like coordinates supported by UE network (for standard ReplicatedMovement actors) with 20 bits per axis */
	static constexpr float MaxCoordinatesSupportedByUENetworking = 1048576.0f;

	/** It is farthest possible location where deactivated actors are placed */
	inline static const FVector MaxPos = FVector(MaxCoordinatesSupportedByUENetworking);

	/** Returns the ghost character */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get Ghost Player Character", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class AGrsPawn* GetGhostPlayerCharacter(const UObject* OptionalWorldContext = nullptr);

	/** Returns the pointer to the ghost controller component, nullptr if local player controller is not initialized yet.
	 * Is picked by own input actions as context that owns their input callbacks. */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get Ghost Controller Component (Local)", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class UGrsPlayerControllerComponent* GetControllerComponent(const UObject* OptionalWorldContext = nullptr);

	/** Calculates the character side from an actor reference */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]")
	static EGRSCharacterSide GetCharacterSideFromActor(const class AActor* Actor);
};
