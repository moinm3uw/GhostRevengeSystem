

#include "LevelActors/GrsPawnSubobjects/GrsPawnSubobject.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimInstance.h"
#include "Bomber.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/BmrSkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GrsPlayerStateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Data/GRSDataAsset.h"
#include "DataAssets/BmrPlayerDataAsset.h"
#include "DataRegistries/BmrPlayerRow.h"
#include "DataRegistries/BmrPlayerSkinRow.h"
#include "GameFramework/BmrPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LevelActors/GRSPlayerCharacter.h"
#include "SubSystems/GRSWorldSubSystem.h"
#include "Subsystems/GlobalMessageSubsystem.h"

// Returns the Skeletal Mesh of ghost revenge character
UBmrSkeletalMeshComponent* FGrsPawnVisualizer::GetMeshChecked(AGRSPlayerCharacter* GrsPawn)
{
	check(GrsPawn);

	return CastChecked<UBmrSkeletalMeshComponent>(GrsPawn->GetMesh());
}

// Set visibility of the player character
void FGrsPawnVisualizer::SetVisibility(AGRSPlayerCharacter* GrsPawn, bool Visibility)
{
	check(GrsPawn);

	GrsPawn->GetMesh()->SetVisibility(Visibility, true);
}

//  Initialize skeletal mesh of the character
void FGrsPawnVisualizer::InitializeSkeletalMesh(class AGRSPlayerCharacter* GrsPawn)
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
void FGrsPawnVisualizer::MovementComponentConfiguration(class AGRSPlayerCharacter* GrsPawn)
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
void FGrsPawnVisualizer::InitCapsuleComponent(class AGRSPlayerCharacter* GrsPawn)
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

// Set and apply skeletal mesh for ghost player. Copy mesh from current player
void FGrsPawnVisualizer::InitPlayerMesh(AGRSPlayerCharacter* GrsPawn)
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
	FGrsPawnVisualizer::GetMeshChecked(GrsPawn)->InitSkeletalMesh(MeshData);
}

// Initialize character visual (animation, skins)  once added to the level by utilizing player id
void FGrsPawnVisualizer::InitCharacterVisual(AGRSPlayerCharacter* GrsPawn)
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

	UBmrSkeletalMeshComponent* CurrentMeshComponent = FGrsPawnVisualizer::GetMeshChecked(GrsPawn);
	if (!ensureMsgf(CurrentMeshComponent, TEXT("ASSERT: [%i] %hs:\n'CurrentMeshComponent' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}
	CurrentMeshComponent->InitSkeletalMesh(MainCharacterMeshComponent->GetMeshData());
	CurrentMeshComponent->ApplySkinByRowName(CurrentSkinRowName);
}