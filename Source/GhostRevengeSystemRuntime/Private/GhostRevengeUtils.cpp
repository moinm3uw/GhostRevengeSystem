// Copyright (c) Yevhenii Selivanov

#include "GhostRevengeUtils.h"

#include "Controllers/MyPlayerController.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

AGRSPlayerCharacter* UGhostRevengeUtils::GetGhostPlayerCharacter(const UObject* OptionalWorldContext)
{
	class AMyPlayerController* PlayerController = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	AGRSPlayerCharacter* PlayerCharacter = nullptr;
	if (PlayerController)
	{
		PlayerCharacter = Cast<AGRSPlayerCharacter>(PlayerController->GetPawn());
	}

	return PlayerCharacter ? PlayerCharacter : nullptr;
}
