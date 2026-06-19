// Copyright (c) Valerii Rotermel & Yevhenii Selivanov

#pragma once

// UE
// @PR JanSeliv [Coding Standards] - unused include, no GameplayStaticsTypes type in header, remove
#include "Kismet/GameplayStaticsTypes.h"

class AGrsPawn;

/**
 * GrsPawnInitializer properties and methods
 */
struct FGrsPawnVisualizer
{
	// @PR JanSeliv [Coding Standards] - GrsPawn only read via const GetMesh(), mark const AGrsPawn* like GetOwningBmrPawn convention in GrsPawnHelper.h
	/** Returns the Skeletal Mesh of ghost revenge character. */
	static class UBmrSkeletalMeshComponent* GetMeshChecked(AGrsPawn* GrsPawn);

	/* @PR JanSeliv [Coding Standards] - mark const AGrsPawn*, GrsPawn only read via const GetMesh(), not mutated, like
	 * GetOwningBmrPawn convention. Same for InitializeSkeletalMesh, MovementComponentConfiguration, InitCapsuleComponent, applies across file */
	/** Set visibility of the player character */
	// @PR JanSeliv [Coding Standards] - bool param needs b prefix, rename Visibility to bVisibility per module bVisibility/bEnable
	static void SetVisibility(AGrsPawn* GrsPawn, bool Visibility);

	/** Initialize skeletal mesh of the character */
	static void InitializeSkeletalMesh(AGrsPawn* GrsPawn);

	// @PR JanSeliv [Coding Standards] - action func noun-first, rename verb-first ConfigureMovementComponent like sibling SetVisibility/InitCapsuleComponent
	/** Configure the movement component of the character */
	static void MovementComponentConfiguration(AGrsPawn* GrsPawn);

	/** Set up the capsule component of the character */
	static void InitCapsuleComponent(AGrsPawn* GrsPawn);

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player. */
	static void InitPlayerMesh(AGrsPawn* GrsPawn);

	/** Initialize character visual (animation, skins)  once added to the level by utilizing player id */
	static void InitCharacterVisual(AGrsPawn* GrsPawn);
};
