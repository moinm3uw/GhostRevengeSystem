// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

#include "Utils/GrsPawnHelper.h"

#include "Animation/AnimInstance.h"
#include "Components/BmrPlayerArrowStartComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GrsPlayerStateComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "DataRegistries/BmrPlayerRow.h"
#include "DataRegistries/BmrPlayerSkinRow.h"
#include "GameFramework/BmrPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// Set pawn location to available side (left or right)
void UGrsPawnHelper::SetPawnToAvailableSide(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	if (!GrsPawn->HasAuthority())
	{
		return;
	}
	EGRSCharacterSide CharacterSide = UGRSWorldSubSystem::Get().RegisterGhostCharacter(GrsPawn);

	checkf(!(CharacterSide == EGRSCharacterSide::None), TEXT("ERROR: [%i] %hs:\n'CharacterSide' is none!"), __LINE__, __FUNCTION__);

	FBmrCell ActorSpawnLocation;
	float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2);

	if (CharacterSide == EGRSCharacterSide::Left)
	{
		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
	}
	else if (CharacterSide == EGRSCharacterSide::Right)
	{
		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2); // temporary, debug row
	}

	// Match the Z axis to what we have on the level
	ActorSpawnLocation.Location.Z = 100.0f;
	GrsPawn->SetActorLocation(ActorSpawnLocation);
}

// Checks if Pawn is replicated fully (player state and controller present
bool UGrsPawnHelper::bIsReady(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	if (!GrsPawn->GetController())
	{
		return false;
	}

	if (!GrsPawn->GetPlayerState())
	{
		// UE_LOG(LogGrs, Verbose, TEXT("GetPlayerState() is not available"), ___FUNCTION___); // ~ Log LogGrs Verbose
		return false;
	}

	return true;
}
