// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

// Grs
#include "Utils/GrsPawnHelper.h"

#include "Components/GrsPlayerStateComponent.h"
#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsPawn.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "GameFramework/BmrGameState.h"
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

	// --- keep already occupied side if this ghost is placed again, otherwise its own side would be treated as occupied by another ghost
	UGrsPlayerStateComponent& GrsPlayerStateComponent = GrsPawn->GetGrsPlayerStateComponentChecked();
	EGRSCharacterSide CharacterSide = GrsPlayerStateComponent.GetGhostSide();
	if (CharacterSide == EGRSCharacterSide::None)
	{
		CharacterSide = FindAvailableGhostSide();
	}

	checkf(CharacterSide != EGRSCharacterSide::None, TEXT("ERROR: [%i] %hs:\n'CharacterSide' is none!"), __LINE__, __FUNCTION__);
	GrsPlayerStateComponent.SetGhostSide(CharacterSide);

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

// Returns the first side of the map that is not occupied by a ghost of any player
EGRSCharacterSide UGrsPawnHelper::FindAvailableGhostSide()
{
	// side is stored on the player state of each ghost, so occupied sides are collected from all ghosts
	bool bIsLeftSideOccupied = false;
	bool bIsRightSideOccupied = false;
	for (const APlayerState* PlayerState : ABmrGameState::Get().PlayerArray)
	{
		// --- personally I am not sure about this, however the main idea was to remove obligatory to manage sides from subsystem.
		// --- otherwise we will cache something back on the subsystem (e.g. map<PlayerID,EGrsCharacterSide>, TArray ghostPlayersSpawned)
		// --- alternative is to fetch by type from pool manager all objects (requires pool manager extension) but I am not sure what we win here (iterate through components vs iterate through types in pool manager)
		const UGrsPlayerStateComponent* GrsPlayerStateComponent = PlayerState ? PlayerState->FindComponentByClass<UGrsPlayerStateComponent>() : nullptr;
		const EGRSCharacterSide OccupiedSide = GrsPlayerStateComponent ? GrsPlayerStateComponent->GetGhostSide() : EGRSCharacterSide::None;
		if (OccupiedSide == EGRSCharacterSide::Left)
		{
			bIsLeftSideOccupied = true;
		}
		else if (OccupiedSide == EGRSCharacterSide::Right)
		{
			bIsRightSideOccupied = true;
		}
	}

	if (!bIsLeftSideOccupied)
	{
		return EGRSCharacterSide::Left;
	}

	if (!bIsRightSideOccupied)
	{
		return EGRSCharacterSide::Right;
	}

	return EGRSCharacterSide::None;
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
		UE_LOG(LogGrs, Verbose, TEXT("GetPlayerState() is not available [%i] %hs: "), __LINE__, __FUNCTION__);
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
		UE_LOG(LogGrs, Verbose, TEXT("FoundPlayerState failed to obtain from UBmrBlueprintFunctionLibrary::GetPlayerState! [%i] %hs"), __LINE__, __FUNCTION__);
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
