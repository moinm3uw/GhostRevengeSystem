// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GrsUtils.generated.h"

enum class EGRSCharacterSide : uint8;

/**
 * 
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// @PR JanSeliv [Coding Standards] - magic literal 1048576.0f repeated 3x, extract to named constexpr axis max then build FVector from it
	/** It is farthest possible location where deactivated actors are placed, is the maximum integer-like coordinates supported by UE network (for standard ReplicatedMovement actors) with 20 bits per axis */
	inline static const FVector MaxPos = {1048576.0f, 1048576.0f, 1048576.0f};

	/** Returns the ghost character */
	UFUNCTION(BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get Ghost Player Character", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class AGrsPawn* GetGhostPlayerCharacter(const UObject* OptionalWorldContext = nullptr);

	// @PR JanSeliv [Coding Standards] - DisplayName "Get BMR Player Controller (Local)" mislabels return, func returns UGrsPlayerControllerComponent ghost controller component, rename to "Get Ghost Controller Component (Local)" to match GetControllerComponent
	/** Returns the ghost controller component, nullptr otherwise. */
	// @PR JanSeliv [Coding Standards] - redundant specifier pair, BlueprintPure already implies callable, drop BlueprintCallable, use BlueprintPure alone like sibling GetGhostPlayerCharacter
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[GhostRevengeSystem]", DisplayName = "Get BMR Player Controller (Local)", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static class UGrsPlayerControllerComponent* GetControllerComponent(const UObject* OptionalWorldContext = nullptr);

	// @PR JanSeliv [Coding Standards] - const pointee, Actor only read for GetActorLocation, never mutated
	// @PR JanSeliv [Coding Standards] - pure getter missing BlueprintPure, add it like sibling GetGhostPlayerCharacter and every other module getter, no side effects
	// @PR JanSeliv [Coding Standards] - bare AActor* relies on transitive GrsPawn.h include, after removing that include AActor undeclared, forward declare with elaborated `class AActor* Actor` like neighbor GRSWorldSubSystem.h AddCollisionActor
	/** Calculates the character side from an actor reference */
	UFUNCTION(BlueprintCallable, Category = "[GhostRevengeSystem]")
	static EGRSCharacterSide GetCharacterSideFromActor(AActor* Actor);
};
