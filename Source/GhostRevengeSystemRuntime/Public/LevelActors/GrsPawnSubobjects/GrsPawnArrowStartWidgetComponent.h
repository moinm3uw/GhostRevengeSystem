#pragma once
#include "UObject/ObjectPtr.h"

class AGrsPawn;

/**
 * 3D Static mesh component that displays the arrow above the local player during match start.
 */
struct FGrsPawnArrowStartWidgetComponent
{
	/** Static mesh component that displays the arrow above the local player during match start. */
	TObjectPtr<class UBmrPlayerArrowStartComponent> PlayerArrowStartComponent = nullptr;

	/** Initialize 3D player arrow widget component that appears on top of character when player start to control it */
	void InitArrowStartWidgetComponent(class AGrsPawn* GrsPawn);

	/** Set visibility of the arrow on top of player character */
	void SetArrowEnabled(bool bVisibility);
};
