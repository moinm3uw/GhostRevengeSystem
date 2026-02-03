// Copyright (c) Yevhenii Selivanov

#include "GhostRevengeUtils.h"

#include "Components/GRSPlayerControllerComponent.h"
#include "Controllers/BmrPlayerController.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"

AGRSPlayerCharacter* UGhostRevengeUtils::GetGhostPlayerCharacter(const UObject* OptionalWorldContext)
{
	class ABmrPlayerController* PlayerController = UBmrBlueprintFunctionLibrary::GetLocalPlayerController();
	AGRSPlayerCharacter* PlayerCharacter = nullptr;
	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- Utils requested return of ghost character"), __LINE__, __FUNCTION__);

	if (PlayerController)
	{
		PlayerCharacter = Cast<AGRSPlayerCharacter>(PlayerController->GetPawn());
		UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- GhostCharacter is %s"), __LINE__, __FUNCTION__, PlayerCharacter ? TEXT("TRUE") : TEXT("FALSE"));
	}

	return PlayerCharacter ? PlayerCharacter : nullptr;
}

// Returns the ghost controller component, nullptr otherwise.
class UGRSPlayerControllerComponent* UGhostRevengeUtils::GetControllerComponent(const UObject* OptionalWorldContext)
{
	const class ABmrPlayerController* LocalController = UBmrBlueprintFunctionLibrary::GetLocalPlayerController();
	if (!LocalController)
	{
		return nullptr;
	}

	return LocalController->FindComponentByClass<UGRSPlayerControllerComponent>();
}
