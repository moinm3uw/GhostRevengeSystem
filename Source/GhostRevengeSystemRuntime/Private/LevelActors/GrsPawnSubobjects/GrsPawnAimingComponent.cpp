#include "LevelActors/GrsPawnSubobjects/GrsPawnAimingComponent.h"

#include "Components/MeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/GRSDataAsset.h"
#include "LevelActors/GrsPawn.h"
#include "Subsystems/GlobalMessageSubsystem.h"
#include "UtilityLibraries/BmrCellUtilsLibrary.h"

// Initial setup of spline component
void FGrsPawnAimingComponent::SetupSplineComponent(class AGrsPawn* GrsPawn)
{
	check(GrsPawn);

	// --- setup spline component
	ProjectileSplineComponentInternal = GrsPawn->CreateDefaultSubobject<USplineComponent>(TEXT("ProjectileSplineComponent"));
	ProjectileSplineComponentInternal->AttachToComponent(MeshComponentInternal, FAttachmentTransformRules::KeepRelativeTransform);

	AimingSphereComponent = GrsPawn->CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereComp"));
}

// Initiate and activate aiming point
void FGrsPawnAimingComponent::InitAimingSphere()
{
	checkf(AimingSphereComponent, TEXT("ERROR: [%i] %hs:\n'AimingSphereComponent' is null!"), __LINE__, __FUNCTION__);

	AimingSphereComponent->SetStaticMesh(UGRSDataAsset::Get().GetProjectileMesh());
	AimingSphereComponent->SetMaterial(0, UGRSDataAsset::Get().GetAimingMaterial());
	AimingSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimingSphereComponent->SetVisibility(false);
}

//  Add a mesh to the last element of the predict projected path results
void FGrsPawnAimingComponent::AddMeshToEndOfProjectedPath(FVector Location)
{
	AimingSphereComponent->SetVisibility(true);
	AimingSphereComponent->SetWorldLocation(Location);
}

// Add spline points to the spline component
void FGrsPawnAimingComponent::AddSplinePoints(FPredictProjectilePathResult& Result)
{
	ClearTrajectorySplines();

	for (int32 i = 0; i < Result.PathData.Num(); i++)
	{
		FVector SplinePoint = Result.PathData[i].Location;
		ProjectileSplineComponentInternal->AddSplinePointAtIndex(SplinePoint, i, ESplineCoordinateSpace::World);
		ProjectileSplineComponentInternal->Mobility = EComponentMobility::Static;
	}

	ProjectileSplineComponentInternal->SetSplinePointType(Result.PathData.Num() - 1, ESplinePointType::CurveClamped, true);
	ProjectileSplineComponentInternal->UpdateSpline();
}

// Add spline mesh to spline points
void FGrsPawnAimingComponent::AddSplineMesh(FPredictProjectilePathResult& Result, AGrsPawn* GrsPawn)
{
	for (int32 i = 0; i < ProjectileSplineComponentInternal->GetNumberOfSplinePoints() - 2; i++)
	{
		// Create and attach the spline mesh component
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(GrsPawn);
		SplineMesh->AttachToComponent(ProjectileSplineComponentInternal, FAttachmentTransformRules::KeepRelativeTransform);
		SplineMesh->ForwardAxis = ESplineMeshAxis::Z;
		SplineMesh->Mobility = EComponentMobility::Static;
		SplineMesh->SetStartScale(UGRSDataAsset::Get().GetTrajectoryMeshScale());
		SplineMesh->SetEndScale(UGRSDataAsset::Get().GetTrajectoryMeshScale());

		// Set mesh and material
		SplineMesh->SetStaticMesh(UGRSDataAsset::Get().GetChargeMesh());
		SplineMesh->SetMaterial(0, UGRSDataAsset::Get().GetTrajectoryMaterial());
		FVector TangentStart = ProjectileSplineComponentInternal->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
		FVector TangentEnd = ProjectileSplineComponentInternal->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

		// Set start and end
		SplineMesh->SetStartAndEnd(Result.PathData[i].Location, TangentStart, Result.PathData[i + 1].Location, TangentEnd);
		// Register the component so it appears in the game
		SplineMesh->RegisterComponent();

		SplineMeshArrayInternal.AddUnique(SplineMesh);
	}
}

// Throw projectile towards the direction of pawn watching, bound to onetime button press
void FGrsPawnAimingComponent::ThrowProjectile(class AGrsPawn* GrsPawn)
{
	//--- Calculate Cell to spawn bomb
	FBmrCell CurrentCell;
	CurrentCell.Location = AimingSphereComponent->GetComponentLocation();

	//--- hide aiming sphere from ui
	AimingSphereComponent->SetVisibility(false);

	SpawnBomb(CurrentCell, GrsPawn);

	FVector ThrowDirection = GrsPawn->GetActorForwardVector() + FVector(5, 5, 0.0f);
	ThrowDirection.Normalize();
	FVector LaunchVelocity = ThrowDirection * 100;

	ClearTrajectorySplines();
}

//  Spawn bomb on aiming sphere position
void FGrsPawnAimingComponent::SpawnBomb(FBmrCell TargetCell, class AGrsPawn* GrsPawn)
{
	const FBmrCell& SpawnBombCell = UBmrCellUtilsLibrary::GetNearestFreeCell(TargetCell);

	// Activate bomb ability
	FGameplayEventData EventData;
	EventData.EventTag = UGRSDataAsset::Get().GetTriggerBombTag();
	EventData.Instigator = GrsPawn;
	EventData.EventMagnitude = UBmrCellUtilsLibrary::GetIndexByCellOnLevel(SpawnBombCell);
	UGlobalMessageSubsystem::BroadcastGlobalMessage(EventData);
}

// Hide spline elements (trajectory)
void FGrsPawnAimingComponent::ClearTrajectorySplines()
{
	for (USplineMeshComponent* SplineMeshComponent : SplineMeshArrayInternal)
	{
		SplineMeshComponent->DestroyComponent();
	}

	SplineMeshArrayInternal.Empty();
	ProjectileSplineComponentInternal->ClearSplinePoints();
}

//  Clean up all transient data
void FGrsPawnAimingComponent::PerformCleanUp()
{
	// Components created via CreateDefaultSubobject must NOT cleanup, they are defaults this actor:
	// ProjectileSplineComponentInternal, AimingSphereComponent, PlayerName3DWidgetComponentInternal
	
	if (AimingSphereComponent)
	{
		AimingSphereComponent->EmptyOverrideMaterials();
	}
	if (MeshComponentInternal)
	{
		MeshComponentInternal->DestroyComponent();
		MeshComponentInternal = nullptr;
	}
}