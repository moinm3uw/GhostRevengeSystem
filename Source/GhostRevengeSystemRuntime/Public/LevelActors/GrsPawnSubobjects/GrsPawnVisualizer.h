#pragma once
#include "Kismet/GameplayStaticsTypes.h"

class AGrsPawn;

/**
 * GrsPawnInitializer properties and methods
 */
struct FGrsPawnVisualizer
{
	/** Returns the Skeletal Mesh of ghost revenge character. */
	static class UBmrSkeletalMeshComponent* GetMeshChecked(AGrsPawn* GrsPawn);

	/** Set visibility of the player character */
	static void SetVisibility(AGrsPawn* GrsPawn, bool Visibility);

	/** Initialize skeletal mesh of the character */
	static void InitializeSkeletalMesh(AGrsPawn* GrsPawn);

	/** Configure the movement component of the character */
	static void MovementComponentConfiguration(AGrsPawn* GrsPawn);

	/** Set up the capsule component of the character */
	static void InitCapsuleComponent(AGrsPawn* GrsPawn);

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player. */
	static void InitPlayerMesh(AGrsPawn* GrsPawn);

	/** Initialize character visual (animation, skins)  once added to the level by utilizing player id */
	static void InitCharacterVisual(AGrsPawn* GrsPawn);
};
