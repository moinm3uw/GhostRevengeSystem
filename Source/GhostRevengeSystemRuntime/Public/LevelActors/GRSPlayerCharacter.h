// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Components/MapComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "LevelActors/PlayerCharacter.h"
#include "GRSPlayerCharacter.generated.h"

/**
 * Ghost Players (only for players, no AI) whose goal is to perform revenge as ghost (spawned on side of map).
 * Copy the died player mesh and skin.
 */
UCLASS()
class GHOSTREVENGESYSTEMRUNTIME_API AGRSPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/** Sets default values for this character's properties */
	AGRSPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	/*********************************************************************************************
	 * Mesh and Initialization
	 **********************************************************************************************/
protected:
	/** The static mesh nameplate (background material of the player name). */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Nameplate Mesh Component"))
	TObjectPtr<class UStaticMeshComponent> NameplateMeshInternal = nullptr;

	/** 3D widget component that displays the player name above the character. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<class UWidgetComponent> PlayerName3DWidgetComponentInternal = nullptr;

	/** Mesh of component. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Mesh Component"))
	TObjectPtr<class UMeshComponent> MeshComponentInternal = nullptr;

	/** Set default character parameters such as bCanEverTick, bStartWithTickEnabled, replication etc. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetDefaultParams();

	/** Initialize skeletal mesh of the character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitializeSkeletalMesh();

	/** Initialize the nameplate mesh component (background material of the player name) */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitializeNameplateMeshComponent();

	/** Setup name plate material */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitializeNamePlateMaterial();

	/** Initialize 3D widget component for the player name */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void Initialize3DWidgetComponent();

	/** Set nickname */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitPlayerNickName();

	/** Configure the movement component of the character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void MovementComponentConfiguration();

	/** Set up the capsule component of the character */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetupCapsuleComponent();

public:
	friend class UMyCheatManager;

	/** Called when an instance of this class is placed (in editor) or spawned */
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	
	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** Called to bind functionality to input */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Clean up the character */
	void PerfromCleanUp();

	/** Set visibility of the player character */
	void SetVisibility(bool Visibility);

	/** Returns the Skeletal Mesh of ghost revenge character. */
	UMySkeletalMeshComponent& GetMeshChecked() const;

	/** Possess a player */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void TryPossessController();

	/** Set and apply default skeletal mesh for this player.
	 * @param bForcePlayerSkin If true, will force the bot to change own skin to look like a player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetDefaultPlayerMeshData(bool bForcePlayerSkin = false);

	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void MovePlayer(const FInputActionValue& ActionValue);

	/** Updates new player name on a 3D widget component. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void SetNicknameOnNameplate(FName NewName);

	/** Returns own character ID, e.g: 0, 1, 2, 3 */
	UFUNCTION(BlueprintPure, Category = "C++")
	int32 GetPlayerId() const;

	/*********************************************************************************************
	 * Hold To Charge and aim
	 **********************************************************************************************/
protected:
	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	class USplineComponent* ProjectileSplineComponentInternal;

	UPROPERTY()
	TObjectPtr<class AGRSBombProjectile> BombProjectileInternal = nullptr;

	UPROPERTY()
	float CurrentHoldTimeInternal = 0.0f;

	/** Spline component used for show the projectile trajectory path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TArray<class USplineMeshComponent*> SplineMeshArrayInternal;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* SphereComp;

public:
	/** Hold button to increase trajectory on max level achieved throw projectile */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ChargeBomb(const FInputActionValue& ActionValue);

	/** Add and update visual representation of charging (aiming) progress as trajectory */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ShowVisualTrajectory();

	/** Spawn and send projectile */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ThrowProjectile(const FInputActionValue& ActionValue);

	/** Hide spline elements (trajectory) */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void ClearTrajectorySplines();

	/** Configure PredictProjectilePath settings and get result */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void PredictProjectilePath(FPredictProjectilePathResult& PredictResult);

	/** Add a mesh to the last element of the predict Projectile path results */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddMeshToEndProjectilePath(FPredictProjectilePathResult& Result);

	/** Add spline points to the spline component */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddSplinePoints(FPredictProjectilePathResult& Result);

	/** Add spline mesh to spline points */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void AddSplineMesh(FPredictProjectilePathResult& Result);
	
	virtual void OnRep_Controller() override;
};
