// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

// Grs
#include "LevelActors/GrsPawnSubobjects/GrsPawnVisualizer.h"

#include "GhostRevengeSystemRuntimeModule.h" // LogGrs
#include "LevelActors/GrsPawn.h"
#include "Utils/GrsPawnHelper.h"

// Bmr
#include "Actors/BmrPawn.h"
#include "Bomber.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "DataRegistries/BmrPlayerRow.h"
#include "DataRegistries/BmrPlayerSkinRow.h"
#include "Structures/BmrMeshData.h"

// UE
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/CollisionProfile.h"

// Returns the Skeletal Mesh of ghost revenge character
UBmrSkeletalMeshComponent* FGrsPawnVisualizer::GetMeshChecked(const AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null during try to obtain a mesh!"), __LINE__, __FUNCTION__);
	return CastChecked<UBmrSkeletalMeshComponent>(GrsPawn->GetMesh());
}

// Set visibility of the player character
void FGrsPawnVisualizer::SetVisibility(const AGrsPawn* GrsPawn, bool bVisibility)
{
	GetMeshChecked(GrsPawn)->SetVisibility(bVisibility, true);
}

//  Initialize skeletal mesh of the character
void FGrsPawnVisualizer::InitializeSkeletalMesh(const AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null!"), __LINE__, __FUNCTION__);

	// Initialize skeletal mesh
	USkeletalMeshComponent* SkeletalMeshComponent = GrsPawn->GetMesh();
	checkf(SkeletalMeshComponent, TEXT("ERROR: [%i] %hs:\n'SkeletalMeshComponent' is null!"), __LINE__, __FUNCTION__);
	static const FVector MeshRelativeLocation(0.0f, 0.0f, -90.0f);
	SkeletalMeshComponent->SetRelativeLocation_Direct(MeshRelativeLocation);
	static const FRotator MeshRelativeRotation(0.0f, -90.0f, 0.0f);
	SkeletalMeshComponent->SetRelativeRotation_Direct(MeshRelativeRotation);
	SkeletalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Enable all lighting channels, so it's clearly visible in the dark
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/ true, /*bChannel1*/ true, /*bChannel2*/ true);
}

// Configure the movement component of the character
void FGrsPawnVisualizer::ConfigureMovementComponent(const AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null!"), __LINE__, __FUNCTION__);

	if (UCharacterMovementComponent* MovementComponent = GrsPawn->GetCharacterMovement())
	{
		// Rotate player by movement
		MovementComponent->bOrientRotationToMovement = true;
		static const FRotator RotationRate(0.0f, 540.0f, 0.0f);
		MovementComponent->RotationRate = RotationRate;

		// Do not push out clients from collision
		MovementComponent->MaxDepenetrationWithGeometryAsProxy = 0.0f;
	}
}

// Set up the capsule component of the character
void FGrsPawnVisualizer::InitCapsuleComponent(const AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null!"), __LINE__, __FUNCTION__);

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
	if (UCapsuleComponent* RootCapsuleComponent = GrsPawn->GetCapsuleComponent())
	{
		// Setup collision to allow overlap players with each other, but block all other actors
		RootCapsuleComponent->CanCharacterStepUpOn = ECB_Yes;
		RootCapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		RootCapsuleComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		// RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player0, ECR_Overlap);
		// RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player1, ECR_Overlap);
		// RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player2, ECR_Overlap);
		// RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player3, ECR_Overlap);

		RootCapsuleComponent->SetIsReplicated(true);
	}
}

// Set and apply skeletal mesh for ghost player. Copy mesh from current player
void FGrsPawnVisualizer::InitPlayerMesh(const AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null!"), __LINE__, __FUNCTION__);
	
	const ABmrPawn* PlayerCharacter = UGrsPawnHelper::GetOwningBmrPawn(GrsPawn);
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

// Initialize character visual (animation, skins)  once added to the level by utilizing player id
void FGrsPawnVisualizer::InitCharacterVisual(const AGrsPawn* GrsPawn)
{
	checkf(GrsPawn, TEXT("ERROR: [%i] %hs:\n'GrsPawn' is null!"), __LINE__, __FUNCTION__);

	const ABmrPawn* PlayerCharacter = UGrsPawnHelper::GetOwningBmrPawn(GrsPawn);
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (USkeletalMeshComponent* MeshComp = GrsPawn->GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UBmrPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}
	
	const UBmrSkeletalMeshComponent& MainCharacterMeshComponentRef = PlayerCharacter->GetMeshComponentChecked();
	// @PR JanSeliv [Conding Standards] - redundant ensureMsgf, address-of GetMeshComponentChecked ref never null, drop guard. Same for GetMeshChecked result below, applies across file. Keep local var referenced as `UBmrSkeletalMeshComponent& MainCharacterMeshCompRef = ...`
	if (!ensureMsgf(&MainCharacterMeshComponentRef, TEXT("ASSERT: [%i] %hs:\n'MainCharacterMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	const FName CurrentSkinRowName = MainCharacterMeshComponentRef.GetAppliedSkinRowName();

	UBmrSkeletalMeshComponent* CurrentMeshComponent = GetMeshChecked(GrsPawn);
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitSkeletalMesh(MainCharacterMeshComponentRef.GetMeshData());
	CurrentMeshComponent->ApplySkinByRowName(CurrentSkinRowName);
}