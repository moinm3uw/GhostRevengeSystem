#pragma once
#include "Kismet/GameplayStaticsTypes.h"

class AGRSPlayerCharacter;

/**
 * GrsPawnInitializer properties and methods
 */
struct FGrsPawnVisualizer
{
	/** Returns the Skeletal Mesh of ghost revenge character. */
	static class UBmrSkeletalMeshComponent* GetMeshChecked(AGRSPlayerCharacter* GrsPawn);

	/** Set visibility of the player character */
	static void SetVisibility(AGRSPlayerCharacter* GrsPawn, bool Visibility);

	/** Initialize skeletal mesh of the character */
	static void InitializeSkeletalMesh(AGRSPlayerCharacter* GrsPawn);

	/** Configure the movement component of the character */
	static void MovementComponentConfiguration(AGRSPlayerCharacter* GrsPawn);

	/** Set up the capsule component of the character */
	static void InitCapsuleComponent(AGRSPlayerCharacter* GrsPawn);

	/** Set and apply skeletal mesh for ghost player. Copy mesh from current player. */
	static void InitPlayerMesh(AGRSPlayerCharacter* GrsPawn);

	/** Initialize character visual (animation, skins)  once added to the level by utilizing player id */
	static void InitCharacterVisual(AGRSPlayerCharacter* GrsPawn);
};
