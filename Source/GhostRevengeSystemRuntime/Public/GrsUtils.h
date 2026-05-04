// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelActors/GrsPawn.h"

#include "GrsUtils.generated.h"

/**
 *
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API UGrsUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
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
