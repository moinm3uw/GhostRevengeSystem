// Copyright (c) Yevhenii Selivanov

#include "GhostRevengeUtils.h"

#include "Controllers/MyPlayerController.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

AGRSPlayerCharacter* UGhostRevengeUtils::GetGhostPlayerCharacter(const UObject* OptionalWorldContext)
{
	class AMyPlayerController* PlayerController = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	AGRSPlayerCharacter* PlayerCharacter = nullptr;
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- Utils requested return of ghost character"), __LINE__, __FUNCTION__);

	if (PlayerController)
	{
		PlayerCharacter = Cast<AGRSPlayerCharacter>(PlayerController->GetPawn());
		UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- GhostCharacter is %s"), __LINE__, __FUNCTION__, PlayerCharacter ? TEXT("TRUE") : TEXT("FALSE"));
	}

	return PlayerCharacter ? PlayerCharacter : nullptr;
}
