// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "GrsUtils.h"

// Grs
#include "Components/GrsPlayerControllerComponent.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Controllers/BmrPlayerController.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsUtils)

AGrsPawn* UGrsUtils::GetGhostPlayerCharacter(const UObject* OptionalWorldContext)
{
	ABmrPlayerController* PlayerController = UBmrBlueprintFunctionLibrary::GetLocalPlayerController();
	AGrsPawn* PlayerCharacter = nullptr;
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (PlayerController)
	{
		PlayerCharacter = Cast<AGrsPawn>(PlayerController->GetPawn());
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: --- GhostCharacter is %s "), __LINE__, __FUNCTION__, PlayerCharacter ? TEXT("TRUE") : TEXT("FALSE"));
	}

	// @PR JanSeliv [Coding Standards] - redundant ternary, PlayerCharacter ? PlayerCharacter : nullptr equals return PlayerCharacter
	return PlayerCharacter ? PlayerCharacter : nullptr;
}

//  Calculates the character side from an actor reference
EGRSCharacterSide UGrsUtils::GetCharacterSideFromActor(const AActor* Actor)
{
	if (!Actor)
	{
		return EGRSCharacterSide::None;
	}

	const FBmrCell ArenaCenter = UBmrCellUtilsLibrary::GetCenterCellOnLevel();
	// @PR JanSeliv [Coding Standards] - Actor->GetActorLocation dereferences AActor, include "GameFramework/Actor.h" in cpp, never rely on transitive include
	/* @PR JanSeliv [Potential Bug] - GetSafeNormal binds to GetActorLocation only, normalizes actor position not direction, side collapses to sign(ArenaCenter.X), same for left and right ghost, both throw same way.
	 * Parenthesize before normalize: (ArenaCenter.Location - Actor->GetActorLocation()).GetSafeNormal() */
	const FVector ToArenaDirection = ArenaCenter.Location - Actor->GetActorLocation().GetSafeNormal();
	// @PR JanSeliv [Coding Standards] - float compared to bare int literal, write `> 0.0f` use `.f` for floats
	return ToArenaDirection.X > 0 ? EGRSCharacterSide::Left : EGRSCharacterSide::Right;
}
