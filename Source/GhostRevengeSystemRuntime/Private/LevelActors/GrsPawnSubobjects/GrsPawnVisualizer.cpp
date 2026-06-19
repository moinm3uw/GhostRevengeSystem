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

// Returns the Skeletal Mesh of ghost revenge character
UBmrSkeletalMeshComponent* FGrsPawnVisualizer::GetMeshChecked(AGrsPawn* GrsPawn)
{
	// @PR JanSeliv [Coding Standards] - use checkf with ERROR [%i] %hs message form like checkf lines below, not bare check, applies across file
	check(GrsPawn);

	return CastChecked<UBmrSkeletalMeshComponent>(GrsPawn->GetMesh());
}

// Set visibility of the player character
void FGrsPawnVisualizer::SetVisibility(AGrsPawn* GrsPawn, bool Visibility)
{
	check(GrsPawn);

	// @PR JanSeliv [Coding Standards] - GetMesh() derefed without null-check, route through GetMeshChecked like other funcs
	GrsPawn->GetMesh()->SetVisibility(Visibility, true);
}

//  Initialize skeletal mesh of the character
void FGrsPawnVisualizer::InitializeSkeletalMesh(AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	// Initialize skeletal mesh
	USkeletalMeshComponent* SkeletalMeshComponent = GrsPawn->GetMesh();
	checkf(SkeletalMeshComponent, TEXT("ERROR: [%i] %hs:\n'SkeletalMeshComponent' is null!"), __LINE__, __FUNCTION__);
	// @PR JanSeliv [Coding Standards] - float components need .f, write 0.f not 0 in FVector/FRotator, applies across file
	static const FVector MeshRelativeLocation(0, 0, -90.f);
	SkeletalMeshComponent->SetRelativeLocation_Direct(MeshRelativeLocation);
	static const FRotator MeshRelativeRotation(0, -90.f, 0);
	SkeletalMeshComponent->SetRelativeRotation_Direct(MeshRelativeRotation);
	// @PR JanSeliv [Coding Standards] - UCollisionProfile used here and below, missing `#include "Engine/CollisionProfile.h"`, relies on transitive. Add to UE group like GrsBombProjectile.cpp
	SkeletalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Enable all lighting channels, so it's clearly visible in the dark
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/ true, /*bChannel1*/ true, /*bChannel2*/ true);
}

// Configure the movement component of the character
void FGrsPawnVisualizer::MovementComponentConfiguration(AGrsPawn* GrsPawn)
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
void FGrsPawnVisualizer::InitCapsuleComponent(AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	UE_LOG(LogGrs, Verbose, TEXT("[%i] %hs: "), __LINE__, __FUNCTION__);
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

// Set and apply skeletal mesh for ghost player. Copy mesh from current player
void FGrsPawnVisualizer::InitPlayerMesh(AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	// @PR JanSeliv [Coding Standards] - const pointee, PlayerCharacter only read via const getters, never reassigned or mutated. Applies across file: same in InitCharacterVisual
	ABmrPawn* PlayerCharacter = UGrsPawnHelper::GetOwningBmrPawn(GrsPawn);
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
	// @PR JanSeliv [Coding Standards] - redundant `FGrsPawnVisualizer::` self-qualifier calling own static from own member, call GetMeshChecked directly, applies across file
	FGrsPawnVisualizer::GetMeshChecked(GrsPawn)->InitSkeletalMesh(MeshData);
}

// Initialize character visual (animation, skins)  once added to the level by utilizing player id
void FGrsPawnVisualizer::InitCharacterVisual(AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	ABmrPawn* PlayerCharacter = UGrsPawnHelper::GetOwningBmrPawn(GrsPawn);
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);

	if (USkeletalMeshComponent* MeshComp = GrsPawn->GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UBmrPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	// @PR JanSeliv [Coding Standards] - address-of ref-returning GetMeshComponentChecked into pointer, bind as ref `UBmrSkeletalMeshComponent&` with Ref suffix, not pointer
	const UBmrSkeletalMeshComponent* MainCharacterMeshComponent = &PlayerCharacter->GetMeshComponentChecked();
	// @PR JanSeliv [Conding Standards] - redundant ensureMsgf, address-of GetMeshComponentChecked ref never null, drop guard. Same for GetMeshChecked result below, applies across file. Keep local var referenced as `UBmrSkeletalMeshComponent& MainCharacterMeshCompRef = ...`
	if (!ensureMsgf(MainCharacterMeshComponent, TEXT("ASSERT: [%i] %hs:\n'MainCharacterMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	const FName CurrentSkinRowName = MainCharacterMeshComponent->GetAppliedSkinRowName();

	UBmrSkeletalMeshComponent* CurrentMeshComponent = FGrsPawnVisualizer::GetMeshChecked(GrsPawn);
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitSkeletalMesh(MainCharacterMeshComponent->GetMeshData());
	CurrentMeshComponent->ApplySkinByRowName(CurrentSkinRowName);
}