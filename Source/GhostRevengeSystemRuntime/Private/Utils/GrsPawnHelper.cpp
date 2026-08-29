// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

// Grs
#include "Utils/GrsPawnHelper.h"

#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsPawn.h"
#include "SubSystems/GRSWorldSubSystem.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "GameFramework/BmrPlayerState.h"
#include "Structures/BmrCell.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// UE
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawnHelper)

// Set pawn location to available side (left or right)
void UGrsPawnHelper::SetPawnToAvailableSide(AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null!"), __LINE__, __FUNCTION__);

	if (!GrsPawn->HasAuthority())
	{
		return;
	}
	const EGRSCharacterSide CharacterSide = UGRSWorldSubSystem::Get().RegisterGhostCharacter(GrsPawn);

	checkf(CharacterSide != EGRSCharacterSide::None, TEXT("ERROR: [%i] %hs:\n'CharacterSide' is none!"), __LINE__, __FUNCTION__);

	FBmrCell ActorSpawnLocation;
	const float CellSize = FBmrCell::CellSize + (FBmrCell::CellSize / 2.0f);

	if (CharacterSide == EGRSCharacterSide::Left)
	{
		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopLeft);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X - CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2.0f); // temporary, debug row
	}
	else if (CharacterSide == EGRSCharacterSide::Right)
	{
		ActorSpawnLocation = UBmrCellUtilsLibrary::GetCellByCornerOnLevel(EBmrGridCorner::TopRight);
		ActorSpawnLocation.Location.X = ActorSpawnLocation.Location.X + CellSize;
		ActorSpawnLocation.Location.Y = ActorSpawnLocation.Location.Y + (CellSize / 2.0f); // temporary, debug row
	}

	// Match the Z axis to what we have on the level
	ABmrPawn* BmrPawn = UBmrBlueprintFunctionLibrary::GetPawn(GrsPawn->GetPlayerID());
	if (!ensureMsgf(BmrPawn, TEXT("ASSERT: [%i] %hs:\n'BmrPawn' failed to obtain from playerID during setting pawn to a side"), __LINE__, __FUNCTION__))
	{
		return;
	}

	ActorSpawnLocation.Location.Z = BmrPawn->GetActorLocation().Z;
	GrsPawn->SetActorLocation(ActorSpawnLocation);
}

// Checks if Pawn is replicated fully (player state and controller present
bool UGrsPawnHelper::IsReady(const AGrsPawn* GrsPawn)
{
	if (!GrsPawn)
	{
		return false;
	}

	if (!GrsPawn->GetController())
	{
		return false;
	}

	if (!GrsPawn->GetPlayerState())
	{
		UE_LOG(LogGrs, Verbose, TEXT("GetPlayerState() is not available"), __FUNCTION__); // ~ Log LogGrs Verbose
		return false;
	}

	return true;
}

// Obtains player state from the provided playerID
APlayerState* UGrsPawnHelper::GetPlayerStateForPlayerID(const AGrsPawn* GrsPawn)
{
	if (!ensureMsgf(GrsPawn, TEXT("ASSERT: [%i] %hs:\n'GrsPawn' is null "), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	APlayerState* FoundPlayerState = Cast<APlayerState>(UBmrBlueprintFunctionLibrary::GetPlayerState(GrsPawn->GetPlayerID()));
	if (!FoundPlayerState)
	{
		UE_LOG(LogGrs, Verbose, TEXT("FoundPlayerState failed to obtain from UBmrBlueprintFunctionLibrary::GetPlayerState!"), __FUNCTION__); // ~ Log LogGrs Verbose
		return nullptr;
	}

	return FoundPlayerState;
}

// Obtains bmr pawn from the provided GrsPawn
ABmrPawn* UGrsPawnHelper::GetOwningBmrPawn(const AGrsPawn* GrsPawn)
{
	if (!ensureMsgf(GrsPawn, TEXT("ASSERT: [%i] %hs:\n'GrsPawn' is null "), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}

	return UBmrBlueprintFunctionLibrary::GetPawn(GrsPawn->GetPlayerID());
}
