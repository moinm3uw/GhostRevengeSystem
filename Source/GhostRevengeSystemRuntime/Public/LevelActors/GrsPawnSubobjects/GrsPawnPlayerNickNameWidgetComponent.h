#pragma once
#include "UObject/ObjectPtr.h"

class AGrsPawn;

/**
 * 3D widget component that displays the player name above the character
 */
struct FGrsPawnPlayerNickNameWidgetComponent
{
	TObjectPtr<class UBmrPlayerNameWidgetComponent> PlayerName3DWidgetComponent = nullptr;

	/** Initialize 3d widget component for the player name */
	void SetupWidget(class AGrsPawn* GrsPawn);

	/** Initialize player name widget (on top of character) */
	void InitializePlayerNameWidget(class AGrsPawn* GrsPawn);
};
