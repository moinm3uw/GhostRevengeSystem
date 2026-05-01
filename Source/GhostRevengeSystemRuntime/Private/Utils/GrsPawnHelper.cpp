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

//  Initialize skeletal mesh of the character
void UGrsPawnHelper::InitializeSkeletalMesh(class AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	// Initialize skeletal mesh
	USkeletalMeshComponent* SkeletalMeshComponent = GrsPawn->GetMesh();
	checkf(SkeletalMeshComponent, TEXT("ERROR: [%i] %hs:\n'SkeletalMeshComponent' is null!"), __LINE__, __FUNCTION__);
	static const FVector MeshRelativeLocation(0, 0, -90.f);
	SkeletalMeshComponent->SetRelativeLocation_Direct(MeshRelativeLocation);
	static const FRotator MeshRelativeRotation(0, -90.f, 0);
	SkeletalMeshComponent->SetRelativeRotation_Direct(MeshRelativeRotation);
	SkeletalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Enable all lighting channels, so it's clearly visible in the dark
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/ true, /*bChannel1*/ true, /*bChannel2*/ true);
}

// Configure the movement component of the character
void UGrsPawnHelper::MovementComponentConfiguration(class AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	if (UCharacterMovementComponent* MovementComponent = GrsPawn->GetCharacterMovement())
	{
		// Rotate player by movement
		MovementComponent->bOrientRotationToMovement = true;
		static const FRotator RotationRate(0.f, 540.f, 0.f);
		MovementComponent->RotationRate = RotationRate;

		// Do not push out clients from collision
		MovementComponent->MaxDepenetrationWithGeometryAsProxy = 0.f;
	}
}

// Set up the capsule component of the character
void UGrsPawnHelper::SetupCapsuleComponent(class AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	UE_LOG(LogTemp, Log, TEXT("[%i] %hs: --- PerformCleanUp"), __LINE__, __FUNCTION__);
	if (UCapsuleComponent* RootCapsuleComponent = GrsPawn->GetCapsuleComponent())
	{
		// Setup collision to allow overlap players with each other, but block all other actors
		RootCapsuleComponent->CanCharacterStepUpOn = ECB_Yes;
		RootCapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		RootCapsuleComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player0, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player1, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player2, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player3, ECR_Overlap);

		RootCapsuleComponent->SetIsReplicated(true);
	}
}

// Returns the Skeletal Mesh of ghost revenge character
UBmrSkeletalMeshComponent* UGrsPawnHelper::GetMeshChecked(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	return CastChecked<UBmrSkeletalMeshComponent>(GrsPawn->GetMesh());
}

// Set visibility of the player character
void UGrsPawnHelper::SetVisibility(AGRSPlayerCharacter* GrsPawn, bool Visibility)
{
	check(GrsPawn);

	GrsPawn->GetMesh()->SetVisibility(Visibility, true);
}

// Set visibility of the arrow on top of player character
void UGrsPawnHelper::SetArrowEnabled(AGRSPlayerCharacter* GrsPawn, bool bVisibility)
{
	check(GrsPawn);

	GrsPawn->GetPlayerArrowStartWidgetComponent()->SetArrowEnabled(bVisibility);
}

// Initialize character visual (animation, skins)  once added to the level by utilizing player id
void UGrsPawnHelper::SetCharacterVisual(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	ABmrPawn* PlayerCharacter = &UGRSWorldSubSystem::Get().GetPlayerStateComponent(GrsPawn->GetPlayerID())->GetCurrentPlayerStateChecked()->GetPawnChecked();
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (USkeletalMeshComponent* MeshComp = GrsPawn->GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UBmrPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	const UBmrSkeletalMeshComponent* MainCharacterMeshComponent = &PlayerCharacter->GetMeshComponentChecked();
	if (!ensureMsgf(MainCharacterMeshComponent, TEXT("ASSERT: [%i] %hs:\n'MainCharacterMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	const FName CurrentSkinRowName = MainCharacterMeshComponent->GetAppliedSkinRowName();

	UBmrSkeletalMeshComponent* CurrentMeshComponent = GetMeshChecked(GrsPawn);
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitSkeletalMesh(MainCharacterMeshComponent->GetMeshData());
	CurrentMeshComponent->ApplySkinByRowName(CurrentSkinRowName);
}

// Set and apply skeletal mesh for ghost player. Copy mesh from current player
void UGrsPawnHelper::InitPlayerMesh(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	const ABmrPawn* PlayerCharacter = &UGRSWorldSubSystem::Get().GetPlayerStateComponent(GrsPawn->GetPlayerID())->GetCurrentPlayerStateChecked()->GetPawnChecked();
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	const FBmrPlayerRow* Row = FBmrPlayerRow::GetFirstRow();
	const FName RowName = FBmrPlayerRow::GetFirstRowName();
	if (!ensureMsgf(Row, TEXT("ASSERT: [%i] %hs:\n'Row' is not found!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	FBmrMeshData MeshData = FBmrMeshData::Empty;
	MeshData.RowName = RowName;
	MeshData.SkinRowName = FBmrPlayerSkinRow::GetSkinRowName(Row->PlayerTag, PlayerCharacter->GetPlayerId());
	GetMeshChecked(GrsPawn)->InitSkeletalMesh(MeshData);
}

// Set side for this pawn (left or right)
void UGrsPawnHelper::SetPawnSide(AGRSPlayerCharacter* GrsPawn)
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

// Refresh the pawn visuals
void UGrsPawnHelper::RefreshPawn(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	// --- Clear splines
	ClearTrajectorySplines(GrsPawn);
	SetVisibility(GrsPawn, true);
	SetArrowEnabled(GrsPawn, true);
	GrsPawn->GetAimingSphereComponent()->SetVisibility(true);
}

// Hide spline elements (trajectory)
void UGrsPawnHelper::ClearTrajectorySplines(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	for (USplineMeshComponent* SplineMeshComponent : GrsPawn->GetSplineMeshArray())
	{
		SplineMeshComponent->DestroyComponent();
	}

	GrsPawn->GetSplineMeshArray().Empty();
	GrsPawn->GetSplineComponent()->ClearSplinePoints();
}

// Initialize player name widget (on top of character)
void UGrsPawnHelper::InitializePlayerNameWidget(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	ABmrPlayerState* MyPlayerState = UGRSWorldSubSystem::Get().GetPlayerStateComponent(GrsPawn->GetPlayerID())->GetCurrentPlayerStateChecked();
	class UBmrPlayerNameWidgetComponent* GrsPlayerName3DWidgetComponent = GrsPawn->GetPlayerName3DWidgetComponent();
	if (!ensureMsgf(MyPlayerState, TEXT("ASSERT: [%i] %hs:\n'MyPlayerState' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(GrsPlayerName3DWidgetComponent, TEXT("ASSERT: [%i] %hs:\n'GrsPlayerName3DWidgetComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	GrsPlayerName3DWidgetComponent->Init(MyPlayerState);
}