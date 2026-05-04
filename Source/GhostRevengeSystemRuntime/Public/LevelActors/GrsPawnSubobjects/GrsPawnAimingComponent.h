#pragma once
#include "Kismet/GameplayStaticsTypes.h"
#include "Structures/BmrCell.h"
#include "UObject/ObjectPtr.h"

class AGrsPawn;

/**
 * Ghost player Aiming visualization and bomb spawn functionality component
 */
struct FGrsPawnAimingComponent
{
	/** Mesh of component. */
	TObjectPtr<class UMeshComponent> MeshComponentInternal = nullptr;

	/** Spline component used to visually display a projectile trajectory path */
	class USplineComponent* ProjectileSplineComponentInternal;

	/** Spline component used to build a projectile trajectory path */
	TArray<class USplineMeshComponent*> SplineMeshArrayInternal;

	/** Aiming sphere used when a player aiming */
	class UStaticMeshComponent* AimingSphereComponent;

	/** Initial setup of spline component */
	void SetupSplineComponent(class AGrsPawn* GrsPawn);

	/** Initiate and activate aiming point */
	void InitAimingSphere();

	/** Add a mesh to the last element of the predict projected path results */
	void AddMeshToEndOfProjectedPath(FVector Location);

	/** Add spline points to the spline component */
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Add spline mesh to spline points */
	void AddSplineMesh(FPredictProjectilePathResult& Result, class AGrsPawn* GrsPawn);

	/** Throw projectile towards the direction of pawn watching, bound to onetime button press */
	void ThrowProjectile(class AGrsPawn* GrsPawn);

	/** Spawn bomb on aiming sphere position. */
	void SpawnBomb(FBmrCell TargetCell, class AGrsPawn* GrsPawn);

	/** Hide spline elements (trajectory) */
	void ClearTrajectorySplines();

	/** Clean up all transient data */
	void PerformCleanUp();
};
