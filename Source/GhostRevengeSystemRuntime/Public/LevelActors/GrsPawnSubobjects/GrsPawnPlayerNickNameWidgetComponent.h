#pragma once
#include "UObject/ObjectPtr.h"

class AGRSPlayerCharacter;

/**
 * 3D widget component that displays the player name above the character
 */
struct FGrsPawnPlayerNickNameWidgetComponent
{
	TObjectPtr<class UBmrPlayerNameWidgetComponent> PlayerName3DWidgetComponent = nullptr;

	/** Initialize 3d widget component for the player name */
	void SetupWidget(class AGRSPlayerCharacter* GrsPawn);

	/** Initialize player name widget (on top of character) */
	void InitializePlayerNameWidget(class AGRSPlayerCharacter* GrsPawn);
};
