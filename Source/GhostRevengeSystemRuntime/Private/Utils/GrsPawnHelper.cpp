// Copyright (c) Valerii Roteremel & Yevhenii Selivanov

#include "Utils/GrsPawnHelper.h"

// @PR JanSeliv [Coding Standards] - includes mix own-plugin, project, UE in one block, split into groups by blank line plus marker comment per module convention
// @PR JanSeliv [Coding Standards] - AnimInstance, BmrCameraComponent, BmrPlayerNameWidgetComponent, BmrSkeletalMeshComponent, SplineComponent, CharacterMovementComponent never referenced in cpp, remove unused includes, applies across include block
#include "Animation/AnimInstance.h"
#include "Components/BmrCameraComponent.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
// @PR JanSeliv [Coding Standards] - ABmrPlayerState never referenced in cpp, remove unused include, not covered by list above
#include "GameFramework/BmrPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LevelActors/GrsPawn.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "UtilityLibraries/BmrBlueprintFunctionLibrary.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// @PR JanSeliv [Coding Standards] - cpp has reflection in own .h, uncomment UE_INLINE_GENERATED_CPP_BY_NAME, commented-out form disables it. Applies across module
// #include UE_INLINE_GENERATED_CPP_BY_NAME(GrsPawnHelper)

// Set pawn location to available side (left or right)
void UGrsPawnHelper::SetPawnToAvailableSide(AGrsPawn* GrsPawn)
{
	// @PR JanSeliv [Coding Standards] - use checkf with ERROR [%i] %hs message form like line below, not bare check, applies across file
	check(GrsPawn);

	if (!GrsPawn->HasAuthority())
	{
		return;
	}
	const EGRSCharacterSide CharacterSide = UGRSWorldSubSystem::Get().RegisterGhostCharacter(GrsPawn);

	// @PR JanSeliv [Coding Standards] - redundant double-negative, write CharacterSide != EGRSCharacterSide::None not !(==)
	checkf(!(CharacterSide == EGRSCharacterSide::None), TEXT("ERROR: [%i] %hs:\n'CharacterSide' is none!"), __LINE__, __FUNCTION__);

	// @PR JanSeliv [Coding Standards] - FBmrCell stored by value and FBmrCell::CellSize used, missing `#include "Structures/BmrCell.h"`, never rely on transitive
	FBmrCell ActorSpawnLocation;
	// @PR JanSeliv [Coding Standards] - CellSize never reassigned, mark const float. Float divisor needs .f, write 2.f not int 2, applies to every / 2 below
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
	// @PR JanSeliv [Coding Standards] - GetPawn result derefed without null-check, save to var and null-guard before GetActorLocation
	ActorSpawnLocation.Location.Z = UBmrBlueprintFunctionLibrary::GetPawn(GrsPawn->GetPlayerID())->GetActorLocation().Z;
	GrsPawn->SetActorLocation(ActorSpawnLocation);
}

// Checks if Pawn is replicated fully (player state and controller present
bool UGrsPawnHelper::bIsReady(AGrsPawn* GrsPawn)
{
	// @PR JanSeliv [Coding Standards] - bIsReady is Is-func, drop check on GrsPawn, silent return false on null input, never assert in Is-func
	check(GrsPawn);

	if (!GrsPawn->GetController())
	{
		return false;
	}

	if (!GrsPawn->GetPlayerState())
	{
		// @PR JanSeliv [Coding Standards] - remove commented-out dead UE_LOG line, no commented-out code
		// UE_LOG(LogGrs, Verbose, TEXT("GetPlayerState() is not available"), ___FUNCTION___); // ~ Log LogGrs Verbose
		return false;
	}

	return true;
}

// Obtains player state from the provided playerID
// @PR JanSeliv [Coding Standards] - cpp uses elaborated class specifier, GrsPawn.h included so drop class keyword, plain AGrsPawn*, applies across file
// @PR JanSeliv [Coding Standards] - GrsPawn derefed via GetPlayerID without null-check, guard like check(GrsPawn) in sibling funcs, applies across file
APlayerState* UGrsPawnHelper::GetPlayerStateForPlayerID(const class AGrsPawn* GrsPawn)
{
	APlayerState* FoundPlayerState = UBmrBlueprintFunctionLibrary::GetPlayerState(GrsPawn->GetPlayerID());
	// @PR JanSeliv [Coding Standards] - no ensureMsgf in Get-func, on null FoundPlayerState silent return nullptr or LogGrs Verbose, never assert in getter
	if (!ensureMsgf(FoundPlayerState, TEXT("ASSERT: [%i] %hs:\n'FoundPlayerState' failed to obtain from UBmrBlueprintFunctionLibrary::GetPlayerState!"), __LINE__, __FUNCTION__))
	{
		return nullptr;
	}
	return FoundPlayerState;
}

// Obtains bmr pawn from the provided GrsPawn
// @PR JanSeliv [Coding Standards] - ABmrPawn used directly here, missing `#include "Actors/BmrPawn.h"`, never rely on transitive
ABmrPawn* UGrsPawnHelper::GetOwningBmrPawn(class AGrsPawn* GrsPawn)
{
	// @PR JanSeliv [Coding Standards] - redundant local used once, return GetPawn result directly
	ABmrPawn* BmrPawn = UBmrBlueprintFunctionLibrary::GetPawn(GrsPawn->GetPlayerID());
	return BmrPawn;
}
