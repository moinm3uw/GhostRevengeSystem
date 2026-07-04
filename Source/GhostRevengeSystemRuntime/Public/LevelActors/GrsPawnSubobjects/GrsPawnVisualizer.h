// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE

class AGrsPawn;

/**
 * GrsPawnInitializer properties and methods
 */
struct FGrsPawnVisualizer
{
	/** Returns the Skeletal Mesh of ghost revenge character. */
	static class UBmrSkeletalMeshComponent* GetMeshChecked(const AGrsPawn* GrsPawn);

	/** Set visibility of the player character */
	static void SetVisibility(const AGrsPawn* GrsPawn, bool bVisibility);

	/** Initialize skeletal mesh of the character */
	static void InitializeSkeletalMesh(const AGrsPawn* GrsPawn);

	/** Configure the movement component of the character */
	static void ConfigureMovementComponent(const AGrsPawn* GrsPawn);

	/** Set up the capsule component of the character */
	static void InitCapsuleComponent(const AGrsPawn* GrsPawn);

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player. */
	static void InitPlayerMesh(const AGrsPawn* GrsPawn);

	/** Initialize character visual (animation, skins)  once added to the level by utilizing player id */
	static void InitCharacterVisual(const AGrsPawn* GrsPawn);
};
