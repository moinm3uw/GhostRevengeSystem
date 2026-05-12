// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#include "GrsUtils.h"

// Grs
#include "Components/GrsPlayerControllerComponent.h"
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Controllers/BmrPlayerController.h"
#include "GhostRevengeSystemRuntimeModule.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

AGrsPawn* UGrsUtils::GetGhostPlayerCharacter(const UObject* OptionalWorldContext)
{
	class ABmrPlayerController* PlayerController = UBmrBlueprintFunctionLibrary::GetLocalPlayerController();
	AGrsPawn* PlayerCharacter = nullptr;
	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);

	if (PlayerController)
	{
		PlayerCharacter = Cast<AGrsPawn>(PlayerController->GetPawn());
		UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: --- GhostCharacter is %s "), __LINE__, __FUNCTION__, PlayerCharacter ? TEXT("TRUE") : TEXT("FALSE"));
	}

	return PlayerCharacter ? PlayerCharacter : nullptr;
}

// Returns the ghost controller component, nullptr otherwise.
class UGrsPlayerControllerComponent* UGrsUtils::GetControllerComponent(const UObject* OptionalWorldContext)
{
	const class ABmrPlayerController* LocalController = UBmrBlueprintFunctionLibrary::GetLocalPlayerController();
	if (!LocalController)
	{
		return nullptr;
	}

	return LocalController->FindComponentByClass<UGrsPlayerControllerComponent>();
}

//  Calculates the character side from an actor reference
EGRSCharacterSide UGrsUtils::GetCharacterSideFromActor(AActor* Actor)
{
	if (!Actor)
	{
		return EGRSCharacterSide::None;
	}

	const FBmrCell ArenaCenter = UBmrCellUtilsLibrary::GetCenterCellOnLevel();
	const FVector ToArenaDirection = ArenaCenter.Location - Actor->GetActorLocation().GetSafeNormal();
	return ToArenaDirection.X > 0 ? EGRSCharacterSide::Left : EGRSCharacterSide::Right;
}
