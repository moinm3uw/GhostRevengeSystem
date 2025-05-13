// Copyright (c) Valerii Rotermel & Yevhenii Selivanov


#include "SubSystems/GRSWorldSubSystem.h"

#include "Components/GRSComponent.h"
#include "Data/EGRSSpotType.h"
#include "Data/MyPrimaryDataAsset.h"
#include "Data/GRSDataAsset.h"
#include "Engine/Engine.h"
#include "GameFramework/MyGameStateBase.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(GRSWorldSubSystem)

// Returns this Subsystem, is checked and will crash if it can't be obtained
UGRSWorldSubSystem& UGRSWorldSubSystem::Get()
{
	const UWorld* World = UUtilsLibrary::GetPlayWorld();
	checkf(World, TEXT("%s: 'World' is null"), *FString(__FUNCTION__));
	UGRSWorldSubSystem* ThisSubsystem = World->GetSubsystem<ThisClass>();
	checkf(ThisSubsystem, TEXT("%s: 'ProgressionSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

// Returns this Subsystem, is checked and will crash if it can't be obtained
UGRSWorldSubSystem& UGRSWorldSubSystem::Get(const UObject& WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObjectChecked(&WorldContextObject);
	checkf(World, TEXT("%s: 'World' is null"), *FString(__FUNCTION__));
	UGRSWorldSubSystem* ThisSubsystem = World->GetSubsystem<ThisClass>();
	checkf(ThisSubsystem, TEXT("%s: 'ProgressionSubsystem' is null"), *FString(__FUNCTION__));
	return *ThisSubsystem;
}

// Returns the data asset that contains all the assets of Ghost Revenge System game feature
const UGRSDataAsset* UGRSWorldSubSystem::GetGRSDataAsset() const
{
	return UMyPrimaryDataAsset::GetOrLoadOnce(DataAssetInternal);
}

// Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors
void UGRSWorldSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Listen game states to switch character skin.
void UGRSWorldSubSystem::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
	case ECurrentGameState::GameStarting:
		OnInitialize.Broadcast();
		break;
	default:
		break;
	}
}

// Returns available FName for spot component
FName UGRSWorldSubSystem::GetSpotName()
{
	FName SpotName = FName("");
	if (SpotComponentsMapInternal.IsEmpty())
	{
		UEnum* EnumPtr = StaticEnum<EGRSSpotType>();
		const FString ContextString = EnumPtr->GetNameStringByValue(TO_FLAG(EGRSSpotType::Left));
		SpotName = *ContextString;
		return SpotName;
	}

	UEnum* EnumPtr = StaticEnum<EGRSSpotType>();
	const FString ContextString = EnumPtr->GetNameStringByValue(TO_FLAG(EGRSSpotType::Right));
	SpotName = *ContextString;
	return SpotName;
}

// Register the ghost revenge system spot component
void UGRSWorldSubSystem::RegisterSpotComponent(UGRSComponent* MyComponent)
{
	if (!ensureMsgf(MyComponent, TEXT("ASSERT: [%i] %hs:\n'MyComponent' is null!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	SpotComponentsMapInternal.Add(MyComponent->GetSpotName(), MyComponent);
}
